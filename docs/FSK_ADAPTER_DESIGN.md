# BK4819 direct-FSK 1200 adapter (candidate integration)

## Receive path

The adapter now connects the LOSEHU BK4819 interrupt/FIFO path to the MCU POCSAG decoder:

1. `RADIO_SetupRegisters(true)` brings up the normal RX chain and existing radio front end.
2. `LBJ_RFStart()` switches the modem to direct FSK 1200, programs a 4-byte POCSAG sync word, and enables RX sync/FIFO/full/finished interrupts.
3. The first hardware-detected sync starts a fresh software POCSAG bitstream. The sync itself is injected back into the software decoder because the packet modem consumes it as the frame delimiter.
4. On FIFO almost-full, the adapter reads the configured four 16-bit words from `REG_5F`. As in LOSEHU's existing `solve_sign()` path, it emits the low byte first and then the high byte; each byte is passed to POCSAG MSB-first.
5. `REG_0B[7]` marks negative sync polarity. The same polarity is normalized before the bytes reach the decoder.
6. The interrupt handler only drains the FIFO and queues bits. BCH correction and business parsing run in `LBJ_AppProcess()` in the main loop.

## Register settings used

| Register | Value | Purpose |
|---|---:|---|
| `REG_70` | `0x00E0` | Enable FSK/TONE2 gain path |
| `REG_72` | `0x3065` | Existing LOSEHU 1200-baud tone/FSK rate setting |
| `REG_58` | `0x00C1` | Direct FSK 1.2K RX, FSK enabled; not FFSK 1200/1800 |
| `REG_5A` | `0x7CD2` | First two bytes of POCSAG sync |
| `REG_5B` | `0x15D8` | Last two bytes of POCSAG sync |
| `REG_5C` | `0x5625` | LOSEHU-known FSK setting with CRC disabled |
| `REG_5D` | `0xFFE0` | 2048-byte data field (`length - 1` in the 11-bit field) |
| `REG_5E` | `0x0204` | RX FIFO almost-full threshold = 4 words |
| `REG_59` | `0x10F8` | RX enabled, 16-byte alternating preamble, 4-byte sync, no scrambler |

The 2048-byte data field is exactly 256 blocks of 8 bytes, matching the four-word FIFO drain threshold; there should be no residual partial FIFO block at the configured end. After each data-field completion, the adapter clears TX/RX FIFOs with RX disabled and then re-enables RX, following the same clear/re-arm ordering used by LOSEHU's existing FSK receive code.

## Important RF limitations (not yet bench-verified)

- 2048 data bytes is the maximum value representable by the BK4819v3 11-bit data-length field. At 1200 bit/s this is about 13.65 seconds of payload. It is a long capture window, not proof that every LBJ burst will fit or that the modem behaves correctly across multiple bursts inside that window.
- The first POCSAG sync is hardware-detected; periodic POCSAG batch-sync words later in the 2048-byte payload are handled by the software decoder.
- The byte order and polarity handling follow the existing LOSEHU FSK FIFO code, but the exact combination of BK4819 modem output, POCSAG line coding, byte order, and negative-polarity behavior has not been confirmed with a live K6 capture.
- The design assumes the BK4819 packet receiver can recognize the alternating POCSAG preamble and configured four-byte sync, then emit the following data field continuously. This must be checked against recorded IQ/audio or an injected test signal.
- The radio's current RF filter/bandwidth, RSSI sensitivity, and AGC response at 821.2375 MHz still require testing on the actual modified K6.

Do not treat this as a verified receiver or flash this build until a K6 bench test confirms sync acquisition, bit orientation, both polarities, page completion, and reacquisition after the 2048-byte window.
