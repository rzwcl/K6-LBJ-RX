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

The exact discriminator scale, smoothing and bit decision threshold are not
yet recovered in a cycle-accurate C form.

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
