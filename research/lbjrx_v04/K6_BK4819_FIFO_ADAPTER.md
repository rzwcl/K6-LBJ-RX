# LBJRX 0.4 → K6 BK4819 direct-FSK/FIFO adapter plan

Status: **original bit decoder host-tested; FIFO interface design only; not K6 hardware verified**.

This document concerns the public LBJ RX 0.4 source candidate preserved in this repository. It does not describe a complete K6 firmware or an approved register image.

## 1. Original decoder boundary

The preserved upstream files are under [LBJRX-RX04](../original_apps/LBJRX-RX04/). The portable decoder API is:

- lbj_rx_bit(lbj_rx_t *r, uint8_t bit) — feed one serial demodulated bit;
- lbj_emit_msg(const lbj_rx_t *r, const char *bcd, uint16_t len) — application callback when a message is complete.

The public RX 0.4 decoder source explicitly specifies POCSAG codewords MSB-first. It rolls a 32-bit shift register until it finds frame sync 0x7CD215D8; Hamming distance <=2 locks normal polarity, distance >=30 locks inverted polarity. After lock it collects 32-bit codewords, performs BCH(31,21) correction and even-parity validation, interprets address/function and BCD message words, and hunts the next batch sync. The app callback filters RIC 1233980..1234020.

The decoder is therefore already the right software boundary for the K6 port. **Do not port the original PA4 ADC/DPLL front end to K6**, and do not replace lbj_rx_bit() with the deleted experimental K6 decoder.

## 2. K6 hardware evidence

Pinned base: losehu/uv-k5-firmware-custom, tag 0.13.2,
commit 021656ea13226d349e31acd6eb21079c5bab64c7.

- BK4819_SetupAircopy() selects direct FSK 1.2K RX mode with REG_58 = 0x00C1, but also configures Aircopy CRC and packet length; do not call the whole function unchanged.
- REG_58[12:10] = 000 selects direct FSK 1.2K RX; REG_58[3:1] = 000 selects the direct 1.2K bandwidth; REG_58[0] enables FSK.
- REG_59 controls RX FIFO clearing, FSK RX enable, data inversion, preamble type/length and 2-byte/4-byte sync selection.
- REG_5A/REG_5B contain the hardware sync bytes; REG_5F is the 16-bit FIFO.
- In the current K6 receiver, the generic FSK handler reads REG_5E[2:0] words from REG_5F, appending each word low byte first, then high byte. MDC1200 additionally checks REG_0B[7] for negative sync polarity. These are source observations, not yet proof of POCSAG FIFO serialization.

The current driver’s direct-FSK setup is a reference for modem mode only. A receive-only POCSAG adapter must set its own sync, preamble and FIFO configuration. Keep REG_59[11] (FSK TX enable) clear at all times.

## 3. Candidate sync handoff

POCSAG transmits an alternating preamble before its 32-bit frame sync. The preserved software decoder searches for 0x7CD215D8 itself, so the K6 hardware should not consume that 32-bit protocol sync if we want to preserve the original software decoder’s sync tolerance.

Candidate experiment:

1. Use direct FSK 1.2K receive mode in REG_58.
2. Set the BK4819 hardware sync detector for a 2-byte alternating marker (candidate REG_5A = 0xAAAA, with the exact preamble-type and sync-byte behaviour to be tested).
3. Let the hardware sync detector consume the first 16 alternating preamble bits.
4. On RX-sync, initialise the original POCSAG decoder state and remember REG_0B polarity status.
5. Read FIFO words in exact hardware order, convert bytes to bits in the measured order, and call lbj_rx_bit() for every bit. Do not inject the POCSAG frame sync; it should remain in the FIFO stream for the original decoder to find.
6. Continue across subsequent POCSAG batches and recover cleanly after overflow, RX-finished, weak signal or loss of sync.

For the usual 576-bit POCSAG alternating preamble, a two-byte hardware match leaves 560 alternating bits before the 32-bit POCSAG frame sync. This is the host handoff test added in commit
3326fe2be039f85716bd44369e7c9ba2a45a4f3b.

The test feeds 560 residual alternating bits, then the original frame sync, one address word, message word and idle words into the unmodified lbj_rx_bit() implementation. It checks normal and inverted polarity. It tests the software side only; it does **not** prove that the BK4819 recognises the proposed marker, starts the FIFO at that bit boundary, or serialises the bits in the assumed order.

## 4. FIFO handling still unresolved

Unlike TDCSRX’s bounded 83-byte frame hypothesis, POCSAG is a repeating batch stream. Each POCSAG batch is one 32-bit sync word plus 16 32-bit codewords. The receiver must not treat one FIFO-almost-full interrupt as a complete message.

A production adapter must answer these points from a controlled RF or conducted test:

- Does the BK4819 strip exactly the two-byte alternating hardware sync?
- Are subsequent bits packed MSB-first or LSB-first within each FIFO byte?
- Are FIFO words exposed low-byte-first, matching the current K6 handlers?
- Does the hardware normalise polarity, or must the captured REG_0B status be applied as XOR 0xFF / 0xFFFF?
- Can RX operate as a continuous FSK stream, or does it stop at a programmed REG_5D length and require a safe re-arm between POCSAG batches?
- What is the precise FIFO almost-full threshold/count behaviour, and how are the final 1–7 words drained without overrun?
- Does a sync-loss or overflow event leave the decoder aligned to the next POCSAG sync after it is reset?

The K6 application loop services radio interrupts on a roughly 10 ms cadence. A practical adapter should drain the FIFO in the interrupt service path, not defer reads until a display update. A one-word threshold is a candidate to test, not a chosen production setting.

No REG_5D packet length is prescribed here. A guessed fixed packet length could truncate batches or cause the receiver to stop before the next sync; obtain a raw FIFO capture first.

## 5. Required raw capture

Use a known POCSAG 1200 waveform carrying a known RIC and known message. The K6 must remain receive-only. Record:

- REG_02 IRQ status and event timing;
- REG_0B polarity state at sync;
- every raw 16-bit REG_5F FIFO word in exact read order;
- REG_5E, REG_5D, and the exact REG_58/REG_59/REG_5A/REG_5B values;
- the expected original POCSAG bitstream, including preamble, frame sync, address and message codewords.

That capture must establish byte order, bit order, polarity, exact sync stripping, batch continuity, length behaviour and re-acquisition before a production lbjrx_bk4819.c adapter is written.

## 6. Firmware integration constraints

The final target must be a full independent LOSEHU-based K6 firmware, not an overlay .app. Keep the receive path strictly RX-only, do not add a PTT/TX route, do not write EEPROM, do not change the user's Bootloader, and retain the requested 10-entry RAM history and UI behaviour. No flashable LBJRX-K6 image exists yet.
