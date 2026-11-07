# TDCSRX RX 0.3 — reverse-engineering notes

## Scope and provenance

This document concerns the published `TDCSRX-cn.v5.5.6.app` executable in
[`troilus/lbjradioweb`](https://github.com/troilus/lbjradioweb/blob/main/archive/apps/TDCSRX-cn.v5.5.6.app).
It is a static reconstruction from the published binary, **not the author's
original C source**.

- Git blob SHA: `af2142508e75b9ef2d2d2c4963be33ace5ee8523`
- File size: 4,140 bytes; FAP1 header: 64 bytes
- Application code: 4,076 bytes (`0x0FEC`)
- App name/version: `TDCS RX` / `0.3`
- ABI/API: 1 / 3
- Link VMA: `0x20000280`; entry offset: 0
- Assets: none
- Code CRC32 from header: `0x640d979a`

The publisher's public web repository contains the .app binaries but no
matching TDCSRX .c/.h source. The linked `troilus/k1lbj` repository contains
LBJ code but no TDCSRX implementation. The recovered C modules in this
repository are reconstructions from machine code, not recovered author source.

## Address convention — important correction

All offsets below are **file offsets relative to the first byte of the code
section**, immediately after the 64-byte FAP1 header. Runtime address is:

`runtime_address = 0x20000280 + code_offset`

Earlier notes mixed runtime addresses with code offsets. For example,
runtime target `0x20000A80` is **code offset `0x0800`**, not `0x0A80`;
runtime target `0x20000484` is **code offset `0x0204`**, not `0x0484`.
The corrected call chain is listed below.

## Verified execution model

The image is a Thumb Cortex-M0/M0+ overlay app linked at `0x20000280`,
using the F4HWN overlay API v3. It is not a standalone firmware image and
cannot be run directly on K6.

The entry routine directly configures MCU ADC/DAC registers and BK4819
registers. The original app biases PA4 to mid-scale with the MCU DAC, samples
the receive-audio signal on ADC channel 4, and reads the ADC data register at
`0x40012450`. The receive loop increments its next sample deadline by
`0x1388` (5,000) core cycles at 48 MHz:

`48,000,000 / 5,000 = 9,600 samples/s`

At 1,200 bit/s this is 8 samples per bit. Each sample is masked to 12 bits
and passed to the DSP routine at code offset `0x0950`.

Initial BK4819 register setup recovered from the entry routine:

| Register | Value / operation | Evidence |
|---:|---:|---|
| `0x38` | `0x2A78` | Entry routine literal and register-write call |
| `0x39` | `0x02BA` | Entry routine literal and register-write call |
| `0x43` | `0x3628` | Entry routine literal and register-write call |
| `0x2B` | read-modify-write, AND mask `0xF8FF` | Entry routine |
| `0x73` | read-modify-write, clears bit `0x0010` | Entry routine |
| `0x30` | read-modify-write around zero/control value | Entry routine |

## Corrected function map and call chain

| Code offset | Runtime address | Recovered role |
|---:|---:|---|
| `0x0164` | `0x200003E4` | 32-bit SWAR population count used by sync matching |
| `0x01D4` | `0x20000454` | BCH(26,16) polynomial remainder / syndrome |
| `0x0204` | `0x20000484` | Build 26 single-bit syndromes and correct one-bit errors |
| `0x039C` | `0x2000061C` | Parse the 45-byte application frame |
| `0x0800` | `0x20000A80` | Rolling sync acquisition, polarity selection, 26-bit word collection and frame assembly |
| `0x0950` | `0x20000BD0` | Audio sample DSP and dual-tone discriminator |
| `0x0D0A` | `0x20000F8A` | Per-sample call site into `0x0950` |

Observed calls:

1. Code offset `0x0D0A` calls runtime address `0x20000BD0`, which is code offset `0x0950`.
2. The DSP at `0x0950` calls runtime address `0x20000A80`, which is code offset `0x0800`.
3. The collector at `0x0800` calls runtime address `0x20000484`, code offset `0x0204`, for BCH decode.
4. The collector at `0x0800` calls runtime address `0x2000089C`, code offset `0x061C`, when a complete application frame is ready.

This replaces the incorrect `0x950 -> 0xA80 -> 0x484 -> 0x61C` *file-offset*
chain in earlier notes. The numbers `0x20000A80` and `0x20000484` are
runtime addresses, not code offsets.

## Audio sample path and dual-tone detector

The DSP routine at code offset `0x0950` contains two phase-indexed signed
coefficient tables and advances their phase counters on every 9,600 Hz sample:

- The 8-entry table is at code offset `0x0E27`:
  `[127, 90, 0, -90, -127, -90, 0, 90]`.
  It also indexes `(phase + 6) & 7`, giving a quadrature pair. Eight samples
  per cycle at 9,600 samples/s correspond to 1,200 Hz.
- The 16-entry table is at code offset `0x0E2F`:
  `[127, 49, -90, -117, 0, 117, 90, -49, -127, -49, 90, 117, 0, -117, -90, 49]`.
  It also indexes `(phase + 4) & 15`, giving its quadrature pair. A 16-sample
  fundamental is 600 Hz; the third harmonic is 1,800 Hz.
- Four signed correlator accumulators are maintained at state offsets
  `+0x14`, `+0x18`, `+0x1C`, and `+0x20`.
- The routine updates a DC estimate before the phase detectors. At entry,
  its update is consistent with
  `dc += ((sample << 4) - dc) >> 6`, followed by subtraction of
  `dc >> 4` from the current sample.

This is instruction/table evidence for 1,200 Hz and 1,800 Hz FFSK tone
detection. A first-pass C translation of the fixed-point state update,
correlator magnitude helper, dual discriminator and phase event logic now
exists. Its end-to-end synthetic 9.6 ksample/s audio-to-frame test still fails,
so it is not a validated working sample-to-bit decoder.

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
- The discriminator compares cross-normalized tone levels:
  `mag1200*smooth1800*weight1200 - mag1800*smooth1200*weight1800`.
  The two weight pairs are 2:3 and 3:2. Each has its own previous-difference
  sign, phase accumulator, and 26-bit frame collector.
- Each lane advances phase by `0x2000` per sample; on sign changes it nudges
  phase a quarter of the way toward `0x9000`. At `0x10000`, it emits one
  bit to its own collector.
- The original initialized state block is `0x108` bytes. The two collector
  states begin at parent offsets `0x88` and `0xC8`.

All arithmetic in the translation explicitly models 32-bit wrap and ARM
arithmetic right shifts. However, the current deterministic end-to-end test
feeding synthetic 9.6 ksample/s audio through the C translation does **not**
acquire the target sync or deliver a frame. The protocol, BCH, CRC, and
frame-collector tests pass, but the sample-to-bit translation is therefore
still **unverified and not a working decoder**. The latest diagnostic records
0 final-frame callbacks and a large bitstream mismatch; see
[Actions run 42340865818](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42340865818).
Do not use this DSP C path in a K6 firmware until this failure is resolved.
The alternative K6 hardware front-end investigation is documented in
[K6_BK4819_FIFO_ADAPTER.md](K6_BK4819_FIFO_ADAPTER.md).

## Sync acquisition, word collection and frame assembly at code offset 0x0800

The collector maintains a rolling 32-bit history and compares normal and
inverted 31-bit and 32-bit windows. Its state includes:

- `state+0x00`: rolling sync history.
- `state+0x04`: sync-acquired flag.
- `state+0x05`: polarity flag.
- `state+0x08`: 26-bit codeword shift register.
- `state+0x0C`: current codeword bit count.
- `state+0x0D`: decoded 16-bit word count in the current application frame.
- `state+0x0E`: expected 16-bit word count.
- `state+0x0F`: decoded byte position in the frame buffer.
- `state+0x11`: frame byte buffer.

The four sync comparisons are:

1. `popcount(0x0DD4259F ^ (rolling & 0x7FFFFFFF))`
2. `popcount(0x722BDA60 ^ (rolling & 0x7FFFFFFF))`
3. `popcount(0x0DD4259F ^ rolling)`
4. `popcount(0xF22BDA60 ^ rolling)`

The collector keeps the smallest distance with the target's original strict
comparison/tie ordering and acquires sync at 2 or fewer errors. The matching
inverted candidate sets the polarity flag. It then normalizes polarity,
collects 26 bits MSB-first, calls the BCH decoder, and stores each decoded
16-bit word high byte first then low byte in the application frame buffer.

The recovered host module `tdcsrx_recovered_stream.c/.h` reproduces the
observed sync windows, polarity selection, 26-bit collection and BCH
handoff. The complete word-count/45-byte frame callback lifecycle is still
being checked against the target; the host module must not be described as a
complete RF receiver.

## Recovered application frame

The original parser at code offset `0x061C` accepts a 45-byte frame. Its
original argument is the last-byte index (`44`); the reconstructed C API uses
a conventional length of 45.

| Frame offset | Size | Rule / meaning |
|---:|---:|---|
| 0 | 1 | Must be `0x0C` |
| 1 | 1 | Must be `0x2B` |
| 2–6 | 5 | Covered by CRC; no fixed values checked by this parser |
| 7–9 | 3 | Either `FF FF FF` or `1F 8C 30` |
| 10 | 1 | Must be `0x20` (32-byte payload) |
| 11–42 | 32 | TDCS payload |
| 43–44 | 2 | CRC-16/XMODEM, big-endian |

CRC covers bytes 0–42, polynomial `0x1021`, initial value zero, MSB-first,
no final xor. The received CRC is `(frame[43] << 8) | frame[44]`.

## Payload layout recovered from the binary

All multi-byte fields below are little-endian unless noted otherwise.

| Payload offset | Size | Meaning | Target handling |
|---:|---:|---|---|
| 0 | 1 | Signal-machine/status selector | Low 4 bits |
| 1 | 1 | Signal-kind selector | Low 3 bits |
| 2 | 1 | Locomotive role | Low 2 bits |
| 3–6 | 4 | Train-class prefix | Raw bytes |
| 7–9 | 3 | Train number | Unsigned 24-bit LE |
| 10–11 | 2 | Locomotive type | LE16 |
| 12–13 | 2 | Locomotive serial | LE16 |
| 14–16 | 3 | Mileage and flags | Bits 0–21 magnitude; bit 22 direction; bit 23 sign |
| 17–19 | 3 | Speed field | Low 10 bits of LE16(payload[17..18]); byte 19 not used for displayed speed |
| 20–21 | 2 | Total weight | LE16 |
| 22–23 | 2 | Calculated length | LE16; display code applies decimal scaling |
| 24 | 1 | Car count | Raw byte |
| 25–26 | 2 | Station number | LE16 |
| 27 | 1 | Section/track number | Raw byte |
| 28 | 1 | Actual route/track | Raw byte |
| 29–31 | 3 | Driver number | Unsigned 24-bit LE |

The target marks mileage absent when all three mileage bytes are `0xFF`.
Magnitude, sign and direction are extracted separately. Other mileage
sentinels from independent implementations require target-binary confirmation.

## Display and paging evidence

Embedded labels include:

`信号机`, `信号种类`, `本补客货`, `车次种类`, `车次`, `机车类型`,
`机车号`, `公里标`, `速度`, `总重`, `计长`, `辆数`, `车站号`,
`区段号`, `实际交路`, `司机号`, `状态`.

The six-field selector table is:
- Page 0: `3, 4, 5, 6, 7, 8`
- Page 1: `9, 10, 11, 12, 13, 14`
- Page 2: `15, 16, 0, 1, 2` (sixth slot unused)

Embedded role strings are `本务 货`, `本务 客`, `补机 货`, `补机 客`.
Status strings include `始发车`, `编组站`, `出站`, `进站`, `通过`,
`预告`, and `容许`.

## BCH implementation recovered from the literal pool

The syndrome routine at code offsets `0x01D4..0x01FC` loads a 32-bit
literal at code offset `0x0200`. Its bytes are `B9 05 00 00`, hence the
little-endian polynomial is **`0x000005B9`**, not `0x0000B905`.

The routine divides the 26-bit codeword from bit 25 down through bit 10,
XORing `0x05B9 << (bit - 10)` when the current high bit is set, then keeps
the low 10-bit remainder. The correction routine at code offset `0x0204`
precomputes all 26 one-bit syndromes and selects the matching bit. The earlier
BCH discrepancy was an endianness/width reading error and is resolved.

The host test verifies 26 unique single-bit syndromes, compares the recovered
operation with an independent `0x05B9` reference on deterministic data, and
injects each of the 26 possible single-bit errors into synthetic valid words.
This verifies the reconstructed BCH layer, not the full RF sampling chain.

## Two parallel discriminators and collector states

A closer reading of the tail of the DSP routine at `0x0950` shows that the
application maintains **two separate bit decision paths**, both derived from
the same 1,200/1,800 Hz I/Q magnitudes:

- The first path at state offsets `+0x70..+0x7B` uses the 16-bit literal
  `0x0302` stored at offset `+0x78`: weights `(2, 3)`.
- The second path at state offsets `+0x7C..+0x87` uses the literal
  `0x0203` stored at offset `+0x84`: weights `(3, 2)`.

For each path the discriminator is formed as:

`diff = magnitude_1200 * weight_1200 - magnitude_1800 * weight_1800`

Each path has its own previous-difference sign and bit-phase accumulator. The
phase accumulator advances by `0x2000` per sample (eight increments per
1,200-baud bit). At a discriminator sign transition, the phase state is
nudged one quarter of the way toward `0x9000`. When the accumulator reaches
`0x10000`, the code subtracts one bit period and sends the decision bit to
its own collector.

- First decision path calls the collector at code offset `0x0800` with state
  base `parent_state + 0x88`.
- Second decision path calls the same collector at code offset `0x0800`
  with state base `parent_state + 0xC8`.

Thus the original image has two parallel software bit-decision and frame
collector states; the earlier single-`tdcsrx_stream_state_t` host module is
only a partial model. The two paths share the tone correlators but have
independent discriminator/clock recovery and sync/frame state.

The helper called at code offset `0x0198` converts each I/Q accumulator pair
(`+0x14/+0x18` and `+0x1C/+0x20`) into the magnitude values consumed by the
two discriminators. Its integer approximation and asymmetric baseline update
have now been translated into C. The discriminator arithmetic is
`mag1200*smooth1800*weight1200 - mag1800*smooth1200*weight1800`, matching
the multiply/subtract instruction order at code offsets `0x0A54..0x0A60`.
These details are instruction-derived, but the synthetic audio-to-frame test
still fails; they have not yet been validated as a functional decoder.

## K6 BK4819 FIFO front-end research — candidate only

The published railway protocol description in
[CN202153314U section 3](https://patents.google.com/patent/CN202153314U/zh)
specifies a 51-bit alternating bit-sync sequence followed by the 31-bit
0x0DD4259F application sync. This aligns with the TDCSRX executable's
31/32-bit rolling sync search. A candidate K6 handoff is to let BK4819 detect
a two-byte alternating preamble marker, then preserve the remaining 35
preamble bits and the actual 31-bit application sync in the FIFO stream for
the recovered software collector. This preserves its two-bit sync tolerance.

A host test for this handoff was added in commit
b98b059c2c0e36699d71be05d0a178cc41dd8aea. It confirms only that the current
software frame collector can decode a synthetic stream with a 35-bit
alternating prefix before the 31-bit sync. It does not verify that the radio
detects the proposed hardware marker or that FIFO byte/bit ordering matches
the test.

See [K6_BK4819_FIFO_ADAPTER.md](K6_BK4819_FIFO_ADAPTER.md) for the candidate
register setup and required raw FIFO capture.

## K6 porting implications — not yet implemented or bench-verified

The original app's PA4/DAC/ADC sampling path belongs to its F4HWN/PY32F071
environment. It is not directly reusable on the target K6 DP32G030 board:
the LOSEHU K6 GPIO map uses PA4 as keyboard column 1, and the DP32G030 port
multiplex table does not expose SARADC on PA4.

The BK4819 does offer hardware data demodulation and FIFO reception. Its
register map documents:
- direct FSK 1.2K RX as RX mode `000`, bandwidth `000`, FSK enable bit 0 set;
- FFSK 1200/1800 RX as RX mode `111`, bandwidth `001`, FSK enable bit 0 set;
- 4-byte sync configuration in `REG_59`, sync bytes in `REG_5A/REG_5B`;
- receive FIFO words through `REG_5F`, FIFO almost-full threshold in `REG_5E`,
  and an 11-bit receive data-length field in `REG_5D` (maximum 2,048 bytes).

That gives a credible K6 front-end path: use BK4819 FFSK 1200/1800 for TDCSRX
and direct FSK 1.2K for LBJ, then feed the recovered software sync/BCH/frame
logic. Exact FIFO byte/bit ordering, sync stripping, polarity reporting,
window length and re-acquisition behaviour still require a controlled signal
source and K6 bench capture before any firmware can be called functional.

Do not reuse the deleted experimental `FirmwareOverlay` LBJ/TDCS decoders or
the invalid BIN files from the earlier attempt. The correct source bases are
the public LBJRX 0.4 source candidate and this TDCSRX 0.3 binary reconstruction.

## Remaining reverse-engineering work

1. Resolve the still-failing synthetic 9.6 ksample/s sample-to-bit test by
   comparing captured intermediate I/Q, magnitude, baseline, discriminator
   and phase values against instruction-level execution. Do not alter test
   amplitudes merely to make the assertion pass.
2. Finish validating the target's frame word-count logic and callback/reset
   lifecycle, especially malformed-frame recovery.
3. Reconstruct all display formatting branches and settings/key behaviour
   before claiming the original TDCSRX app has been recovered in full.
4. Build a K6-specific BK4819 FFSK receive adapter and validate it with
   synthetic and recorded signals before integration into the full-firmware
   base.

No K6 receiver source code has been changed as part of this reverse-engineering
update.

## Real 457.550 MHz WAV probe (2026-11-07)

A tracked recording in `rzwcl/450-TAx2`,
`2026-10-11_17-30-46_audio_457550000Hz.wav`, was fetched in GitHub Actions
and verified against Git blob SHA-1
`8fb409ef803a98c17834fa230410c4579b030a83`. It is mono 16-bit PCM,
22,050 Hz, 1.696961 s. The independent 48 kHz reference demodulator
reproduces the exact 31-bit sync at bit 1,376 / 1.146667 s, matching the
recording analysis position (~1.146593 s).

The current sample-to-bit C translation was run on a sinc-resampled 9.6 kHz
unsigned-12-bit representation of the same recording. Across ADC scale factors
0.50..2.00 it still delivered zero frames. C bitstream comparison showed
path A's nearest 31-bit sync at Hamming distance 8..10; path B's nearest was
7. The aligned bit error rates were approximately 0.32..0.40.

Separately, an independent 8-sample quadrature-energy detector at the same
9.6 kHz sample rate recovered the complete frame from this ADC file:
sync bit 259, BCH corrections 0, header/padding valid, CRC B27A/B27A,
train X319 and speed 20 km/h. This establishes that the WAV contains enough
information to recover the frame; it is not the original target DSP.

The synthetic DSP-to-frame test also still fails after widening the 1200:1800
input amplitude ratio to 8:1. It produces about 831/825 output symbols for
694 expected bits. In the early alternating preamble, lane A's weighted
difference stays negative and lane B's stays positive for both bit values,
so the two lanes do not form the required complementary data stream. See
[Actions run 42348715488](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42348715488).

Probe and comparison files:
- `test_tdcsrx_real_wav.c`
- `prepare_real_wav.py`
- `compare_real_wav_bits.py`
- workflow `.github/workflows/test-tdcsrx-real-wav.yml`

The next DSP task is to compare the target's per-sample correlator magnitudes,
baseline states and decision signs against the reference waveform around the
known sync. For the synthetic case, trace every one of the 8 sample updates
inside each symbol and log both discriminator sign changes and phase wraps.
Do not tune audio gain further until ring-history and state-update behaviour
has been matched instruction by instruction.
