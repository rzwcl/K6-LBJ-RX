# TDCSRX RX 0.3 — verified binary-to-C path and K6 interface status

## Source and provenance

Published application: troilus/lbjradioweb/archive/apps/TDCSRX-cn.v5.5.6.app

- FAP1 file size: 4,140 bytes
- 64-byte header + 4,076-byte code section
- Application: TDCS RX 0.3, ABI/API 1/3
- Link VMA: 0x20000280
- Git blob SHA-1: af2142508e75b9ef2d2d2c4963be33ace5ee8523
- Code CRC32: 0x640D979A

No matching original author C/H source has been found. The C implementation in this folder is a reconstruction from the published binary, not the original source.

## Address convention and call graph

Offsets below are relative to the code section immediately after the FAP1 header. Runtime address = 0x20000280 + code offset.

| Code offset | Runtime address | Function |
|---:|---:|---|
| 0x0164 | 0x200003E4 | 32-bit SWAR popcount for sync distance |
| 0x0198 | 0x20000418 | I/Q magnitude and asymmetric baseline helper |
| 0x01D4 | 0x20000454 | BCH(26,16) syndrome / polynomial remainder |
| 0x0204 | 0x20000484 | one-bit BCH correction |
| 0x0594 | 0x20000814 | original 40-byte application-record callback |
| 0x061C | 0x2000089C | 45-byte frame parser |
| 0x0800 | 0x20000A80 | sync acquisition, polarity, 26-bit word and frame collector |
| 0x0950 | 0x20000BD0 | 9.6 ksample/s audio DSP |
| 0x0D0A | 0x20000F8A | sample loop calling DSP |

Main call chain:
0x0D0A -> 0x0950 -> 0x0800 -> (0x0204 BCH, 0x061C frame parser) -> 0x0594 callback.

## Original audio input and detector

The original sample loop reads ADC register 0x40012450, masks to 12 bits and calls 0x0950 once every 5,000 cycles from a 48 MHz timebase:

48,000,000 / 5,000 = 9,600 samples/s.

At 1,200 bit/s, one bit spans eight samples. The original ADC path is not directly reusable on the K6: the app targets the F4HWN/PY32F071 environment, while the pinned LOSEHU DP32G030 K6 GPIO map uses PA4 as keyboard column 1 and does not expose that pin as the required SARADC input.

The exact DSP is reconstructed in tdcsrx_recovered_dsp.c/.h:

- DC tracker: dc += (((sample << 4) - dc) >> 6), then input = sample - (dc >> 4).
- Biquad: y = (0x140A*(x[n]-x[n-2]) + 0x3246*y[n-1] - 6125*y[n-2]) >> 14.
- 1,200 Hz I/Q table at code offset 0x0E27:
  [127,90,0,-90,-127,-90,0,90], with Q selected at (phase+6)&7.
- 1,800 Hz I/Q table at code offset 0x0E2F:
  [127,49,-90,-117,0,117,90,-49,-127,-49,90,117,0,-117,-90,49], with Q selected at (phase+4)&15.
- Magnitude helper at 0x0198:
  max(abs(I),abs(Q)) + (min(abs(I),abs(Q)) >> 2) + (min(abs(I),abs(Q)) >> 3).
- Baseline rises by (magnitude-baseline)>>5 with a minimum of 16; falling branch is baseline-(baseline>>10). Both baselines initialise to 16.

## Two independent discriminator and collector paths

The original binary has two decision lanes operating on the same pair of 1,200/1,800 Hz I/Q magnitudes. The lanes are not different RF frequencies.

| Lane | Parent state offsets | Initial weights | Collector state |
|---|---|---|---|
| A | +0x70..+0x7B | weight1200=2, weight1800=3 | parent +0x88 |
| B | +0x7C..+0x87 | weight1200=3, weight1800=2 | parent +0xC8 |

Each path computes cross-normalized terms and subtracts them:

diff = (mag1200 * smooth1800 * weight1200) - (mag1800 * smooth1200 * weight1800).

Each lane has its own previous-difference sign, phase accumulator, clock-recovery state and 26-bit frame collector. The phase accumulator advances by 0x2000 per sample. At a sign transition it is nudged one quarter of the way toward 0x9000. On reaching 0x10000 it subtracts one bit period and emits a bit to that lane's collector.

## C-versus-original Thumb validation

tools/emulate_tdcsrx_dsp.py downloads the exact published FAP1, checks its Git blob SHA-1 and code CRC32, then executes the original Thumb routine at 0x0950 using Unicorn. It does not reimplement the DSP in Python. It intercepts the original collector entry only to record the bit and return to the original sample routine.

Pinned field recording:

- rzwcl/450-TAx2/2026-10-11_17-30-46_audio_457550000Hz.wav
- Git blob SHA-1: 8fb409ef803a98c17834fa230410c4579b030a83
- 22,050 Hz mono signed 16-bit PCM, duration 1.696961 s
- resampled to 16,291 unsigned 12-bit samples at 9,600 samples/s.

The original Thumb and C implementation were compared after every input sample. Results:

| State / output | C versus Thumb |
|---|---|
| ADC sample | exact |
| filtered output | exact |
| 1,200 Hz I/Q accumulators | exact |
| 1,800 Hz I/Q accumulators | exact |
| both smoothed baselines | exact |
| both sample phases | exact |
| both previous discriminator differences | exact |
| both bit-phase accumulators | exact |
| lane A emitted bits | 1,988 / 1,988, zero differences |
| lane B emitted bits | 1,865 / 1,865, zero differences |

Latest full direct-execution comparison: Actions run 42369257863. The exported per-sample state had zero mismatching fields and zero maximum absolute difference. The C DSP translation is therefore instruction-level matched to the original binary for this pinned recording.

## Original weights and external sound-card recording caveat

The original initializer writes lane A weights 2:3 and lane B weights 3:2. These recovered defaults remain unchanged in C.

The pinned recording was captured through an external sound-card/audio path. The original 2:3 / 3:2 decision thresholds do not decode this WAV: nearest 31-bit sync distances are 7–10 bits, aligned BER about 0.32–0.40, and no valid frame callback is produced.

A separate diagnostic-only experiment sets both lanes to 1:1 after normal initialization. It does not change the filter, correlators, smoothing, DPLL, collector, BCH, parser or CRC. At input scale 0.50 it decodes X319, speed 20 km/h, locomotive type 237 (0x00ED), serial 32 (0032), CRC B27A, with four frame callbacks across the two independently reacquiring lanes. This is diagnostic evidence about the sound-card path only; it does not justify replacing the original 2:3 / 3:2 defaults in production.

The independent 48 kHz reference detector finds sync at bit 1,376 / 1.146667 s. A separate 9.6 kHz eight-sample quadrature-energy detector also recovers the frame at bit 259, with no BCH correction and valid CRC B27A. These reference detectors confirm the recording contains a recoverable frame; they do not replace the original target DSP.

## Sync, word and frame path

The original collector at code offset 0x0800 maintains rolling 32-bit history. It tests normal and inverted candidates over 31- and 32-bit windows against 0x0DD4259F, accepting a best match with no more than two errors. After lock it normalizes polarity and collects 26 bits MSB-first per BCH word.

The BCH generator polynomial is 0x05B9. The original syndrome routine at 0x01D4 divides the 26-bit codeword from bit 25 down through bit 10; the correction routine at 0x0204 searches the 26 one-bit syndromes.

The parser at 0x061C accepts a 45-byte frame:
- byte 0 = 0x0C, byte 1 = 0x2B;
- bytes 7..9 = FF FF FF or 1F 8C 30;
- byte 10 = 0x20;
- bytes 11..42 = 32-byte payload;
- bytes 43..44 = big-endian CRC-16/XMODEM over bytes 0..42.

The host tests verify normal and inverted streams, sync tolerance, 26-bit BCH collection, single-bit correction, CRC, frame parser and two independent receiver path states.

## Original 40-byte application callback ABI

The original callback at code offset 0x0594 copies exactly 40 bytes to the app's current-record area. This record is not the same as the host diagnostic tdcsrx_message_t, which also contains raw payload, raw frame and CRC diagnostics.

The complete parser output layout is documented in TDCSRX_REVERSE_ENGINEERING.md. A C type tdcsrx_app_record_t and function tdcsrx_build_app_record() now provide a separate callback-compatible 40-byte byte-array ABI. It:
- validates the 45-byte frame through the recovered header/CRC parser;
- copies the exact target frame fields and fixed index-one auxiliary bytes;
- preserves the header-variant, mileage flags and constant index byte;
- zeros the two output bytes +0x26..+0x27 which the original parser leaves untouched, avoiding stale-memory exposure.

tools/emulate_tdcsrx_parser.py executes the original Thumb parser for leading-digit quotients 0, 1, 3, 8 and rejected 9. It independently verifies fixed index-one copies from frame[0x28..0x29] and frame[0x26..0x27], both header variants, and the 40-byte output boundary. The C tests additionally validate the portable record conversion, mileage sentinel, invalid CRC and deterministic zero tail.

## K6 BK4819 FFSK FIFO interface — not yet hardware verified

The pinned LOSEHU K6 base has BK4819 FFSK 1200/1800 RX and FIFO support:
- REG_58 selects FFSK 1200/1800 RX mode and bandwidth;
- REG_59 controls RX enable, FIFO clear, inversion and sync length;
- REG_5A/REG_5B hold sync bytes;
- REG_5D is the receive byte-length field;
- REG_5E controls the FIFO almost-full threshold;
- REG_5F reads 16-bit FIFO words;
- REG_02 reports sync/FIFO/RX-finished events; REG_0B reports detected polarity.

The candidate port replaces the original PA4 ADC front end with BK4819 FFSK output while retaining the recovered software framing/BCH/parser stages. Exact FIFO sync stripping, byte order, bit order, polarity correction, receive length, final partial-word handling and re-acquisition still require raw K6 REG_02/0B/5D/5E/5F capture under a controlled FFSK source.

Do not treat the candidate 83-byte post-sync length or the proposed 0xAAAA two-byte preamble marker as a proven register configuration. They are host handoff hypotheses only. The receive-only adapter has not been implemented or bench-verified.

## Current status

Validated:
- exact FAP1 DSP C translation against original Thumb execution on the pinned WAV;
- original 45-byte frame/BCH/CRC protocol path;
- original 40-byte application-record conversion and host tests;
- independent receiver lane state and sync/collector host tests.

Not yet completed:
- target UI formatter semantics for auxiliary/unknown bytes and settings/key behaviour;
- BK4819 FIFO raw-capture and hardware adapter;
- integration into the full LOSEHU K6 firmware as a separate TDCSRX build;
- native radio reception on the K6.

There is no flashable TDCSRX-K6 firmware yet. Keep TDCSRX and LBJRX as separate full-firmware targets, preserve the user's existing multi-firmware Bootloader, and keep both paths strictly receive-only.
