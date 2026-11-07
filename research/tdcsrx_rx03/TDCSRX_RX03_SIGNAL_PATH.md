# TDCSRX RX 0.3 — corrected sample-to-bit path and K6 interface notes

## Source image

Published `TDCSRX-cn.v5.5.6.app`:
- FAP1 file size: 4,140 bytes
- Code section: 4,076 bytes
- App: TDCS RX 0.3, ABI/API 1/3
- Link VMA: `0x20000280`
- Git blob SHA: `af2142508e75b9ef2d2d2c4963be33ace5ee8523`

This is a reverse-engineering report, not original author C source.

## Address convention and corrected call graph

Every offset below is relative to the code section immediately after the 64-byte
FAP1 header. Convert it to runtime address by adding `0x20000280`.

| Code offset | Runtime address | Function |
|---:|---:|---|
| `0x0164` | `0x200003E4` | 32-bit SWAR popcount for sync distance |
| `0x01D4` | `0x20000454` | BCH(26,16) syndrome / polynomial remainder |
| `0x0204` | `0x20000484` | One-bit BCH correction using 26 syndromes |
| `0x061C` | `0x2000089C` | 45-byte application-frame parser |
| `0x0800` | `0x20000A80` | Sync acquisition, polarity, 26-bit words and frame assembly |
| `0x0950` | `0x20000BD0` | 9.6 ksample/s audio DSP |
| `0x0D0A` | `0x20000F8A` | Sample loop calling the DSP |

Call chain:
`0x0D0A -> 0x0950 -> 0x0800 -> (0x0204 BCH, 0x061C frame parser)`.

The old report used runtime targets `0x20000A80` and `0x20000484` as
though they were code offsets. They are not: after subtracting the link VMA,
they are offsets `0x0800` and `0x0204`, respectively.

## Sample input and tone detector

The receive loop reads ADC data register `0x40012450`, masks to 12 bits and
calls `0x0950` once every 5,000 core cycles at 48 MHz:

`48,000,000 / 5,000 = 9,600 samples/s`

At 1,200 bit/s that is 8 samples per bit. The code at `0x0950` updates a DC
estimate, filters the sample, then updates four phase-indexed correlator
accumulators at state offsets `+0x14`, `+0x18`, `+0x1C`, and `+0x20`.

- Table at code offset `0x0E27`: 8 signed coefficients
  `[127,90,0,-90,-127,-90,0,90]`; it also selects `(phase+6)&7`,
  giving I/Q for 1,200 Hz at 9,600 samples/s.
- Table at code offset `0x0E2F`: 16 signed coefficients
  `[127,49,-90,-117,0,117,90,-49,-127,-49,90,117,0,-117,-90,49]`;
  it also selects `(phase+4)&15`, giving I/Q at 1,800 Hz (the third
  harmonic of 600 Hz).

The C module now contains a first-pass translation of the discriminator
scale, smoothing, and decision-state updates. The discriminator subtraction
order was corrected, but a later attempt to interpret the falling-baseline
branch added an extra ASRS #5 incorrectly. Disassembly shows the branch at
0x01D2 targets 0x01C2, skipping the rising branch's ASRS #5; the C code and
numeric test have now been corrected to falling decrement baseline>>10.
A real 457.550 MHz recording is now used for an end-to-end probe; see
[TDCSRX_REVERSE_ENGINEERING.md](TDCSRX_REVERSE_ENGINEERING.md).

## Sync, word and frame path at code offset 0x0800

The collector keeps rolling 32-bit history. It checks normal and inverted
candidates over 31- and 32-bit windows against `0x0DD4259F`, accepting the
best match at no more than two errors. Once synchronized, it normalizes
polarity and collects 26 bits MSB-first per BCH word.

The collector calls:
- `0x0204` for BCH correction; `0x01D4` calculates the 10-bit syndrome
  using the literal polynomial `0x05B9`;
- `0x061C` after enough decoded words have assembled a complete 45-byte
  application frame.

The 45-byte application frame contains a 32-byte business payload and a
big-endian CRC-16/XMODEM. The field layout and header rules are documented in
[`TDCSRX_REVERSE_ENGINEERING.md`](TDCSRX_REVERSE_ENGINEERING.md).

## Exact sample DSP reconstruction from Thumb instructions

The original `0x0950` sample routine is now translated into
`tdcsrx_recovered_dsp.c/.h`. The recovered constants and state layout are:

- Sample rate: 9,600 Hz; one 1,200-baud symbol spans 8 samples.
- DC tracker: `dc += (((sample << 4) - dc) >> 6)`, then input is
  `sample - (dc >> 4)`.
- Biquad:
  `y = (0x140A*(x[n]-x[n-2]) + 0x3246*y[n-1] + 0xFFFFE813*y[n-2]) >> 14`.
- The 1,200 Hz and 1,800 Hz I/Q correlators use the literal tables at code
  offsets `0x0E27` and `0x0E2F`, with an 8-sample sliding history.
- Helper at code offset `0x0198` returns
  `max(abs(I),abs(Q)) + (min(abs(I),abs(Q)) >> 2) + (min(...) >> 3)`.
  Its baseline rises by `(magnitude-baseline)>>5` then clamps to a minimum
  baseline of 16; it falls by `baseline-(baseline>>10)`; the initialized baselines are both 16.
- The discriminator subtract order at code offsets `0x0A54..0x0A60` is:
  `mag1200*smooth1800*weight1200 - mag1800*smooth1200*weight1800`.
  An earlier C translation had this subtraction reversed; it was corrected
  in commit `6ca52dd26db000ab833be7f14fd62d03900f18a9`.
  The two weight pairs are 2:3 and 3:2. Each has its own previous-difference
  sign, phase accumulator, and 26-bit frame collector.
- Each lane advances phase by `0x2000` per sample; on sign changes it nudges
  phase a quarter of the way toward `0x9000`. At `0x10000`, it emits one
  bit to its own collector.
- The original initialized state block is `0x108` bytes. The two collector
  states begin at parent offsets `0x88` and `0xC8`.

All arithmetic in the translation explicitly models 32-bit wrap and ARM
arithmetic right shifts. Despite this instruction-based reconstruction, the
synthetic end-to-end audio test does not produce a valid frame. Treat this as
an incomplete, unverified sample-to-bit translation—not as a proven TDCSRX
receiver. The proposed alternative BK4819 FFSK FIFO handoff and the exact
unknowns to capture are documented in
[K6_BK4819_FIFO_ADAPTER.md](K6_BK4819_FIFO_ADAPTER.md).

## Dual discriminator paths — recovered structure

The DSP does not produce just one software bitstream. It calculates two
parallel discriminators from the same 1,200/1,800 Hz I/Q magnitudes:

| Path | State offsets in parent DSP state | Literal weights | Separate stream state |
|---|---|---|---|
| A | `+0x70..+0x7B` | `0x0302` = (1200:2, 1800:3) | `parent + 0x88` |
| B | `+0x7C..+0x87` | `0x0203` = (1200:3, 1800:2) | `parent + 0xC8` |

Each path forms
`diff = magnitude_1200 * weight_1200 - magnitude_1800 * weight_1800`,
tracks its own previous sign and phase accumulator, advances phase by
`0x2000` per sample, and nudges phase toward `0x9000` by one quarter at a
sign transition. At `0x10000` it emits a bit to its own `0x0800` collector.

This establishes two independent software bit/frame paths, not two RF
frequencies. The reason both swapped-weight paths are enabled is not yet
named by the target binary. The magnitude helper at code offset `0x0198`
and its exact fixed-point behavior still require reconstruction.

The current `tdcsrx_recovered_stream.c` models one collector only; it does
not yet model these two parallel discriminator/collector instances and must
not be described as the complete original stream path.


## K6 front-end path to test

The original app's PA4 ADC method is not directly reusable: it targets the
F4HWN/PY32F071 environment, while the LOSEHU DP32G030 K6 GPIO map uses PA4
as keyboard column 1 and does not expose PA4 as a SARADC channel.

BK4819 hardware can demodulate FFSK 1200/1800 and exposes a receive FIFO:
- `REG_58` RX mode `111` selects FFSK 1200/1800; bandwidth `001`;
- `REG_59` controls RX FIFO clear, FSK RX enable, inversion and sync length;
- `REG_5A/REG_5B` hold the four sync bytes;
- `REG_5D` is an 11-bit byte-length field (maximum 2,048 bytes);
- `REG_5E` selects the RX FIFO almost-full threshold;
- `REG_5F` reads FIFO words.

For TDCSRX the four-byte target sync is `0x0DD4259F`. Replacing only the
analog discriminator with BK4819 FFSK output is the candidate porting strategy;
the original sync/BCH/frame stages remain the reference. This is not yet
verified for FIFO bit ordering, sync stripping or polarity on a K6.

No receiver C code was changed in this documentation update.

## Real audio comparison

The pinned 457.550 MHz field recording has been added as a reproducible CI
input (Git blob SHA-1 8fb409ef803a98c17834fa230410c4579b030a83). The
independent 48 kHz demodulator finds the 31-bit sync at bit 1376 / 1.146667 s.
The reconstructed 9.6 kHz C DSP currently fails on this real recording too:
no complete frame, closest sync distances 7..10 bits, and aligned BER about
0.32..0.40. Further instruction-level sample state tracing is required.


## 2026-11-07 — independent reference success vs recovered DSP failure

The pinned field WAV can be decoded by a separate, deliberately simple 8-sample
quadrature-energy reference detector at 9.6 kHz. It finds sync at bit 259 in the
resampled sample stream and produces a frame with BCH corrections 0, valid
header/padding, and CRC B27A/B27A. This is a reference/control implementation,
not the original TDCSRX DSP.

The reconstructed target C path still does not decode the same WAV. Its two
weighted slicer lanes have substantial bitstream mismatch (sync Hamming distance
7–10, BER around 0.32–0.40) and no frame callbacks. The standalone synthetic
ADC-to-frame diagnostic also still fails after trying a wide 8:1 input tone
amplitude ratio: about 831/825 output symbols for 694 expected bits, and the two
weighted lanes remain on opposite fixed decisions for the early alternating
preamble. See CI run 42348715488 and the detailed report
[TDCSRX_REAL_WAV_DIAGNOSTIC_2026-11-07.md](TDCSRX_REAL_WAV_DIAGNOSTIC_2026-11-07.md).

Do not substitute the reference detector into the target-recovery module or
claim the original sample DSP is complete. The next investigation is a per-sample
instruction-level model of the history ring, I/Q accumulators, 0x0198 baseline
helper, weighted differences, sign transitions, phase correction, and collector
call timing.

## Original application callback ABI (newly recovered)

The caller's third argument to the 0x0950 DSP is a function pointer loaded
from code literal 0x0DB4: value 0x20000815, Thumb entry for code offset
0x0594. The callback receives the parser's compact output record and copies exactly
40 bytes (three 12-byte LDM/STM groups plus one 4-byte LDR/STR) into the
application current-record area at global-object offset +4. It then updates
record counters and invokes the application display hook. The parser's
separate cmp r1,#0x2c checks the last index 44 of the 45-byte input frame; it
does not describe the compact output record size.

The host reconstruction currently passes a generic C callback a
 tdcsrx_message_t that also contains raw payload/frame and CRC diagnostics.
That object is not ABI-compatible with the original 40-byte application
record. Do not treat the host message struct as the original UI record. The
remaining offsets and a portable explicit record-conversion routine are
still to be reconstructed.
