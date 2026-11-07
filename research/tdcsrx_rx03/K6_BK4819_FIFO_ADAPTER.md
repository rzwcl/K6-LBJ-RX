# TDCSRX RX 0.3 → K6 BK4819 FFSK/FIFO adapter plan

Status: **design and host-handoff test only; not yet K6 hardware verified**.

This document records the current implementation path for the real TDCSRX RX 0.3 protocol layer. It does not describe a flashable firmware.

## 1. Evidence from the TDCS protocol

The published railway protocol description for the列尾控制台出入库检测 channel specifies:

- 51 bits of alternating 1,0 bit synchronisation;
- a 31-bit frame synchronisation sequence 0x0DD4259F;
- shortened cyclic BCH (26,16) protection, adding 10 parity bits to each 16-bit data word;
- frame CRC-16/XMODEM over the application data.

The same sync value, BCH polynomial 0x05B9, and 45-byte application frame are independently present in the published TDCSRX RX 0.3 executable reconstruction. The protocol reference is [CN202153314U, section 3](https://patents.google.com/patent/CN202153314U/zh); the executable analysis is in [TDCSRX_REVERSE_ENGINEERING.md](TDCSRX_REVERSE_ENGINEERING.md).

The 45-byte application frame is carried as 23 BCH words. The last decoded low byte is padding, so the wire contains:

23 × 26 = 598 BCH bits.

The target collector searches the 31-bit frame sync with a tolerance of up to two bit errors and also accepts inverted polarity.

## 2. Evidence from BK4819 and the pinned K6 firmware

Baseline: losehu/uv-k5-firmware-custom, commit
021656ea13226d349e31acd6eb21079c5bab64c7.

### Register facts

The BK4819v3 register documentation and pinned driver agree on the following:

| Register | Relevant behaviour |
|---|---|
| REG_58[12:10] | RX mode 111 = FFSK 1200/1800 |
| REG_58[9:8] | FSK RX gain |
| REG_58[3:1] | bandwidth 001 = FFSK 1200/1800 |
| REG_58[0] | FSK modem enable |
| REG_59[14] | clear RX FIFO |
| REG_59[12] | enable FSK RX |
| REG_59[11] | enable FSK TX; must remain 0 |
| REG_59[10] | invert received FSK data |
| REG_59[3] | 0 = 2-byte sync, 1 = 4-byte sync |
| REG_5A, REG_5B | sync bytes, high byte first |
| REG_5D | 11-bit FSK data-length field; field value 0x0F represents 16 bytes (length-minus-one encoding) |
| REG_5E[2:0] | RX FIFO almost-full threshold, in 16-bit words; FIFO depth is 8 words |
| REG_5F | 16-bit RX/TX FIFO word |
| REG_02[1] | RX sync event |
| REG_02[12] | FIFO almost-full event |
| REG_02[13] | RX finished event |
| REG_0B[7] | negative-polarity sync found |
| REG_0B[6] | positive-polarity sync found |

The REG_5D length is represented by the low 8 bits in REG_5D[15:8] and the high 3 bits in REG_5D[7:5]. The FSK frame description in the BK4819 guide places the sync word before the data field.

### What the current K6 receiver already proves

In pinned LOSEHU source, CheckRadioInterrupts() handles FSK status in its regular radio-interrupt loop. On an RX-sync event, solve_sign() reads REG_0B and takes bit 7 as the negative-polarity flag. Its MDC path inserts the known sync bytes into its software buffer at that point. On FIFO-almost-full, it reads REG_5F words and appends the low byte before the high byte. Thus the current K6 firmware's protocol handler already assumes the hardware-consumed sync is **not** repeated in FIFO data.

That establishes the general handoff model; it does not yet prove the TDCS bit order or the exact FIFO timing on the user's K6.

The existing enable_msg_rx(true) routine is not reusable as-is: it programs MDC sync bytes 0x7240/0x99A7, MDC frame sizing and other MDC-specific fields. The TDCS adapter must own its FFSK setup and keep TX disabled.

## 3. Candidate TDCS hardware-sync strategy

Using the full 4-byte application sync as the BK4819 hardware sync would hide the actual sync bits from the software receiver and would reduce the value of the recovered software decoder's two-error sync tolerance. The better first hardware experiment is to let the chip lock to the alternating bit-sync pattern, then leave the **actual TDCS frame sync** in the FIFO stream for the software collector.

Candidate setup to test:

1. Select FFSK 1200/1800 receive mode and 1200/1800 receive bandwidth in REG_58.
2. Select a 2-byte hardware sync in REG_59[3].
3. Program the 2-byte hardware sync as an alternating preamble pattern (candidate REG_5A = 0xAAAA; the chip's REG_58[5:4] preamble-type behaviour and both signal polarities still need bench confirmation).
4. Keep scrambling off, RX inversion initially off, RX enabled, and TX enable bit REG_59[11] off.
5. On RX-sync, latch the REG_0B polarity status and initialise the TDCS software collector.
6. Feed the FIFO bytes after the hardware-matched 16 preamble bits into tdcsrx_receiver_feed_bit(). Keep the residual alternating preamble and the original 31-bit 0x0DD4259F frame sync in the stream; do not inject or discard that application sync.
7. Allow the software sync matcher to select normal/inverted polarity and tolerate up to two frame-sync bit errors; then feed the 23 BCH words and validate the 45-byte application CRC.

The framing arithmetic is a useful check on the candidate packet length:

- original bit synchronisation: 51 bits;
- candidate 2-byte hardware match: 16 bits consumed;
- residual alternating preamble: 35 bits;
- application frame sync: 31 bits;
- BCH stream: 23 × 26 = 598 bits;
- total after hardware sync: 35 + 31 + 598 = **664 bits = 83 bytes**.

If the BK4819 RX length counter is confirmed to count exactly the data bytes following its two-byte hardware sync, a first candidate is 83 bytes. Because the documented field is length-minus-one, that candidate encodes as REG_5D = 0x5200. This value is **not yet approved for device use**: FIFO bit packing, the chip's handling of a 31-bit sync after the preamble, and any byte-alignment/padding behaviour must be confirmed by a raw FIFO capture first.

For low-latency draining, a one-word RX threshold is worth testing. The K6 application loop runs every 10 ms and services the BK interrupt-request loop; a FIFO adapter must drain all words reported by repeated almost-full events and account for the final partial word at RX-finished.

## 4. Host test added

Commit b98b059c2c0e36699d71be05d0a178cc41dd8aea adds
test_two_byte_preamble_sync_handoff() to
test_tdcsrx_recovered_receiver.c.

It simulates the post-sync FIFO stream that would remain after a 16-bit hardware match: 35 alternating bits, followed by the 31-bit application sync and 23 BCH words. It checks normal and inverted input, and checks two errors in the application sync.

This test **only validates the software handoff hypothesis**. It does not prove that BK4819 detects 0xAAAA in the user's radio, that the FIFO begins at the assumed bit, or that the FIFO bit order is correct.

Latest test run for that commit: [GitHub Actions run 42340865818](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42340865818). The new preamble-handoff case and the existing protocol/frame tests passed. The overall workflow still failed at the separate synthetic audio-DSP-to-frame diagnostic; that failure remains open and is not hidden.

## 5. Required hardware capture before implementing the K6 adapter

Capture, without transmitting from the K6:

- REG_02 status and the timing of RX-sync/FIFO-almost-full/RX-finished;
- REG_0B at the sync event for normal and inverted signals;
- every raw 16-bit REG_5F word in exact read order;
- raw REG_5E and REG_5D values;
- a known injected FFSK waveform containing the 51-bit preamble, 31-bit sync, 23 known BCH words and valid application CRC.

The capture must answer:
1. whether the FIFO starts after the full two-byte hardware sync;
2. whether bytes arrive low-byte-first, as the current K6 handler assumes;
3. whether bits within each byte are MSB-first or LSB-first;
4. whether polarity is already normalised by hardware or must be XOR-corrected;
5. whether 83 bytes are delivered before RX-finished;
6. whether sync loss and a second frame reacquire cleanly.

Only after those answers are fixed should a production tdcsrx_bk4819.c adapter be added to a complete LOSEHU K6 firmware tree.

## 6. LBJ parallel work

The same hardware question applies to the POCSAG path, but its stream framing differs. The pinned K6 driver documents direct FSK 1.2K mode in REG_58 and reads the same REG_5F FIFO. The original LBJRX software decoder expects a continuous POCSAG bitstream with preamble and sync; do not feed it FIFO words until byte order, bit order, polarity, and hardware sync stripping are measured.

No TX routine or Bootloader modification is part of either receive-only target.
