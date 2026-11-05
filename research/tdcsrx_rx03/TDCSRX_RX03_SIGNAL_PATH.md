# TDCSRX RX 0.3 — recovered sample-to-bit signal path

## Source

Published `TDCSRX-cn.v5.5.6.app`; FAP1 code section is 4,076 bytes linked at
`0x20000280`. Disassembly artifact:
[TDCSRX RX 0.3 binary disassembly](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42082752327).

## BCH literal and correction

- Code offset `0x1DC`: `LDR r1, [pc, #0x20]`.
- PC-relative literal target: code offset `0x200`.
- Raw bytes at `0x200`: `B9 05 00 00`.
- Little-endian 32-bit polynomial: `0x000005B9`.
- Code offset `0x1D4`: polynomial remainder routine, bits 25 down through 10.
- Code offset `0x204`: builds all 26 single-bit syndromes and locates a one-bit correction.
- Reconstructed tests demonstrate 26 unique syndromes and successful correction of every single-bit position across six generated codewords.

The earlier `0xB905` anomaly was a literal-pool endianness/width reading error.

## Sample timing and DSP call site

- Code offset `0xCBA`: receive processing loop.
- It uses 5,000 core cycles per sample. At 48 MHz this is 9,600 samples/s.
- It reads `0x40012450` and masks to the low 12 bits.
- Code offset `0xD0A` calls code offset `0x950` once for each sample.
- The DSP routine performs DC tracking/removal before using the phase-indexed tables.

## Two detected FFSK tones

The function at `0x950` references two constant tables via literal pointers:

| Code offset | Runtime pointer | Signed coefficients | Phase |
|---:|---:|---|---:|
| `0xE27` | `0x200010A7` | `[127,90,0,-90,-127,-90,0,90]` | 8 |
| `0xE2F` | `0x200010AF` | `[127,49,-90,-117,0,117,90,-49,-127,-49,90,117,0,-117,-90,49]` | 16 |

The first table is a sampled cosine with 8 phases. The routine selects both
`phase` and `(phase + 6) & 7`, providing a quadrature pair. At 9,600 Hz,
8 samples per cycle correspond to 1,200 Hz.

The second table is a 16-phase cosine at the third harmonic. The routine also
selects `phase` and `(phase + 4) & 15`, giving its quadrature pair. At
9,600 Hz, the 16-sample fundamental is 600 Hz and its third harmonic is
1,800 Hz.

The code advances the 8-phase and 16-phase counters once per ADC sample and
updates four state accumulators at offsets `+0x14`, `+0x18`, `+0x1C`,
and `+0x20`. This is strong evidence for 1,200/1,800 Hz dual-tone I/Q
detection.

## Function chain

```text
ADC 9.6 ksample/s
  -> 0x0950  DC removal + 1200/1800 Hz quadrature detector
  -> 0x0A80  bit/word synchronization and collection
  -> 0x0484  BCH(26,16) syndrome table and single-bit correction
  -> 0x061C  45-byte application-frame parser
```

Evidence for the transitions: the call at `0xD0A` to `0x950`, calls from
`0x950` into `0xA80`, and calls in `0xA80` into `0x484` and `0x61C`.

## Still to reconstruct

- Exact signed fixed-point scaling and state structure for the entire `0x950`
  routine.
- The final discriminator sign/threshold and event cadence from `0x950` into
  `0xA80`.
- Precise bit-polarity tracking, sync reacquisition and frame assembly rules
  inside `0xA80`.
- A C implementation validated against synthetic bitstreams and only then
  adapted to K6 DP32G030 + BK4819.

This report is static reconstruction from the published machine code. It is
not original author C source, not yet a drop-in decoder, and not a complete K6
TDCSRX firmware.
