# TDCSRX RX 0.3 — reverse-engineering notes

## Scope and provenance

This document concerns the published `TDCSRX-cn.v5.5.6.app` executable in
[`troilus/lbjradioweb`](https://github.com/troilus/lbjradioweb/blob/main/archive/apps/TDCSRX-cn.v5.5.6.app).
It is a reverse-engineering reconstruction, **not the author's original C source**.

- Git blob SHA: `af2142508e75b9ef2d2d2c4963be33ace5ee8523`
- File size: 4,140 bytes
- Container: `FAP1`
- Header: 64 bytes
- Application code: 4,076 bytes (`0x0FEC`)
- App name/version: `TDCS RX` / `0.3`
- ABI/API: 1 / 3
- Link VMA: `0x20000280`
- Entry offset: 0
- Assets: none
- Code CRC32 from header: `0x640d979a`

The publisher's public web repository contains the `.app` binaries but no
matching `.c` or `.h` source. Its linked `troilus/k1lbj` repository contains
LBJ code but no TDCSRX implementation. The F4HWN upstream source branches
checked also do not contain TDCSRX. The recovered source in this package is
therefore necessarily reconstructed from the binary.

## Verified execution model

The image is a Thumb Cortex-M0/M0+ overlay application linked at
`0x20000280`, using the F4HWN overlay API v3. It is not a standalone firmware
image and is not directly runnable on the K6.

The entry routine directly configures MCU ADC/DAC registers and BK4819
registers. The binary uses PA4 as the receive-audio ADC input and sets a DAC
mid-scale bias while sampling. The receiver loop uses a 48 MHz core-cycle
clock and increments the next sample deadline by `0x1388` (5,000) cycles:

`48,000,000 / 5,000 = 9,600 samples/s`.

At 1,200 bit/s this is 8 ADC samples per bit. The ADC data register is read at
`0x40012450`; the sample is masked to 12 bits and passed to the DSP routine at
code offset `0x950`.

The initial radio setup writes the following BK4819 register/value pairs:

| Register | Value | Evidence |
|---:|---:|---|
| `0x38` | `0x2A78` | Entry routine literal and `bk_write` call |
| `0x39` | `0x02BA` | Entry routine literal and `bk_write` call |
| `0x43` | `0x3628` | Entry routine literal and `bk_write` call |
| `0x2B` | read-modify-write, AND mask `0xF8FF` | Entry routine |
| `0x73` | read-modify-write, clears bit `0x0010` | Entry routine |
| `0x30` | read-modify-write around zero/control value | Entry routine |

The receiver uses a rolling 32-bit sync register. The sync constant is
`0x0DD4259F`; normal and inverted bit polarities are considered. After sync,
26-bit words are collected MSB-first and converted into 16-bit data words.
The application frame is assembled from those decoded 16-bit words.

## Audio sample path and dual-tone detector recovered

The entry's receive loop calls the routine at code offset `0x950` from the
sample-processing loop at `0xD0A`. The loop reads the ADC data register
`0x40012450`, retains the low 12 bits, and invokes the routine once per
5,000-cycle deadline at a 48 MHz core clock: 9,600 samples/s.

The routine at code offset `0x950` is not merely an unknown opaque DSP block.
Its literal tables and phase arithmetic identify a dual-tone quadrature detector:

- The 8-entry signed coefficient table is at code offset `0xE27`:
  `7F 5A 00 A6 81 A6 00 5A` =
  `[127, 90, 0, -90, -127, -90, 0, 90]`.
  The routine advances an 8-state phase once per sample and also indexes
  `(phase + 6) & 7`, yielding the quadrature component. At 9,600 samples/s,
  one 8-sample cycle is 1,200 Hz.
- The 16-entry signed coefficient table is at code offset `0xE2F`:
  `7F 31 A6 8B 00 75 5A CF 81 CF 5A 75 00 8B A6 31` =
  `[127, 49, -90, -117, 0, 117, 90, -49, -127, -49, 90, 117, 0, -117, -90, 49]`.
  This is a 16-phase, third-harmonic cosine table. The routine also indexes
  `(phase + 4) & 15`, giving its quadrature component. A 16-sample
  fundamental at 9,600 samples/s is 600 Hz; its third harmonic is 1,800 Hz.
- The function maintains four signed accumulators in its state object at
  offsets `+0x14`, `+0x18`, `+0x1C`, and `+0x20`, in addition to the
  phase counters and sample history. Before the phase detectors it updates a
  slow DC estimate and subtracts that estimate from the input sample.
- The downstream path is `0x950` → `0xA80` (bit/word synchronization and
  collection) → `0x484` (26-bit BCH single-bit correction) → `0x61C`
  (45-byte application-frame parser). The call at `0xD0A` is the per-sample
  entry into this chain.

This is strong instruction/table evidence that the app demodulates the 1,200 Hz
and 1,800 Hz FFSK tones with phase-indexed I/Q detectors before bit slicing.
It is a static reconstruction, not yet a cycle-accurate C port. The exact
fixed-point scaling, downstream decision thresholds, both state lanes, and
resynchronization timing still need to be translated and tested before K6
integration.

## Recovered application frame

The original parser at code offset `0x61C` requires a 45-byte frame. Its
argument is a last-byte index (`44`) in the original ABI; this reconstruction
uses a conventional length of 45.

| Frame offset | Size | Rule / meaning |
|---:|---:|---|
| 0 | 1 | Must be `0x0C` |
| 1 | 1 | Must be `0x2B` (43) |
| 2–6 | 5 | Covered by CRC; no fixed values checked by this parser routine |
| 7–9 | 3 | Either `FF FF FF` or `1F 8C 30` |
| 10 | 1 | Must be `0x20` (32 payload bytes) |
| 11–42 | 32 | TDCS payload |
| 43–44 | 2 | CRC-16/XMODEM, big-endian |

CRC is calculated over bytes 0–42, polynomial `0x1021`, initial value zero,
MSB-first, no final xor. The transmitted CRC is `(frame[43] << 8) | frame[44]`.

## Payload layout recovered from the binary

The payload starts at frame offset 11. All multi-byte numbers below are
little-endian unless noted otherwise.

| Payload offset | Size | Meaning | Target handling |
|---:|---:|---|---|
| 0 | 1 | Signal-machine/status selector | Low 4 bits retained |
| 1 | 1 | Signal-kind selector | Low 3 bits retained |
| 2 | 1 | Locomotive role | Low 2 bits retained |
| 3–6 | 4 | Train-class prefix | Retained byte-for-byte |
| 7–9 | 3 | Train number | Unsigned 24-bit LE |
| 10–11 | 2 | Locomotive type | LE16 |
| 12–13 | 2 | Locomotive serial | LE16 |
| 14–16 | 3 | Mileage and flags | Bits 0–21 magnitude; bit 22 direction; bit 23 sign |
| 17–19 | 3 | Speed field | Low 10 bits of LE16(payload[17..18]); byte 19 is not used for the displayed speed |
| 20–21 | 2 | Total weight | LE16 |
| 22–23 | 2 | Calculated length | LE16; display code inserts decimal scale |
| 24 | 1 | Car count | Raw byte |
| 25–26 | 2 | Station number | LE16 |
| 27 | 1 | Section/track number | Raw byte |
| 28 | 1 | Actual route/track | Raw byte |
| 29–31 | 3 | Driver number | Unsigned 24-bit LE |

The target sets its mileage-present flag false when the three mileage bytes are
all `0xFF`. It separately extracts the 22-bit magnitude, sign bit and direction
bit. Other protocol-level mileage sentinels described by independent decoders
need additional target-binary confirmation before treating them as equivalent.

The recovered C parser is in `tdcsrx_recovered_protocol.c/.h`. It preserves the
raw frame and payload alongside the decoded values so remaining differences
can be checked against future evidence.

## Display strings and paging

The executable contains these field labels in GB2312:

`信号机`, `信号种类`, `本补客货`, `车次种类`, `车次`, `机车类型`, `机车号`,
`公里标`, `速度`, `总重`, `计长`, `辆数`, `车站号`, `区段号`, `实际交路`,
`司机号`, `状态`.

The paging table selects six fields per page:

- Page 0: selectors `3, 4, 5, 6, 7, 8`
- Page 1: selectors `9, 10, 11, 12, 13, 14`
- Page 2: selectors `15, 16, 0, 1, 2` (sixth slot unused)

The role strings embedded in the binary are `本务 货`, `本务 客`, `补机 货`,
`补机 客`. Signal/status text includes `始发车`, `编组站`, `出站`, `进站`,
`通过`, `预告`, and `容许`. UI status strings include `km/h`, `dB`, `4BL:`,
`1SPK:`, `ON`, and `OFF`.

## BCH implementation recovered from the literal pool

The BCH routine at code offsets `0x1D4..0x1FC` uses an LDR at `0x1DC`.
Its PC-relative target is the 32-bit literal at code offset `0x200`, whose
bytes are `B9 05 00 00`. Interpreted as a little-endian 32-bit word, the actual
generator polynomial is **`0x000005B9`**, not `0x0000B905`.

The routine shifts the 26-bit codeword through polynomial division from bit 25
down to bit 10, XORing `0x05B9 << (bit - 10)` when the current high bit is set,
then keeps the low 10-bit remainder. The correction routine at `0x204`
precomputes the syndrome for all 26 one-bit masks and finds the matching bit.
The reconstructed implementation now uses `0x05B9` for the original routine.

A host test now verifies all 26 single-bit syndromes are unique, compares the
target operation against the independent `0x05B9` reference for deterministic
test values, and injects each of the 26 possible single-bit errors into several
valid synthetic BCH(26,16) words. Every injected single-bit error is corrected
to the original 16-bit data word. This supersedes the earlier BCH discrepancy
section in the initial recovery notes; that discrepancy was caused by reading
the first two bytes of the literal pool without decoding the full 32-bit
little-endian value.


## Remaining reverse-engineering work

1. Resolve the BCH literal/syndrome discrepancy with another build or a target
   frame vector from a source independent of the user's recordings.
2. Finish documenting the integer DSP routine at `0x950` (filter states,
   tone correlators, timing loop and decision threshold) and the exact byte/bit
   framing around the 26-bit words.
3. Reconstruct all display formatting branches and setting/key behavior from
   routines `0x314`, `0xAD4`, and `0xBF4`.
4. Port the recovered receive chain to the K6 DP32G030/BK4819 platform and the
   existing LOSEHU full-firmware integration. The F4HWN PA4 ADC/DAC setup and
   overlay API are not directly reusable on the K6.

No K6 source files were changed while preparing this reconstruction.