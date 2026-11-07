# TDCSRX RX 0.3 original Thumb DSP validation — 2026-11-07

## Result in one sentence

The recovered C implementation of the TDCSRX RX 0.3 sample DSP now matches the **original FAP1 Thumb machine code** at every exported per-sample state field for the pinned 457.550 MHz recording. The original executable's fixed 2:3 / 3:2 discriminator weights still do not decode this external sound-card WAV; a separate 1:1 diagnostic override does, so the production weights have deliberately not been changed.

## 1. Exact original binary used

Source: `troilus/lbjradioweb/archive/apps/TDCSRX-cn.v5.5.6.app`

- Git blob SHA-1: `af2142508e75b9ef2d2d2c4963be33ace5ee8523`
- FAP1 code section: 4,076 bytes
- Code link VMA: `0x20000280`
- Code CRC32: `0x640D979A`
- Sample DSP entry: code offset `0x0950` / runtime `0x20000BD0`
- Bit collector entry: code offset `0x0800` / runtime `0x20000A80`

`tools/emulate_tdcsrx_dsp.py` downloads this exact FAP1, verifies its Git blob SHA-1, validates its header and code CRC32, maps the original code into Unicorn's ARM Thumb-M environment, and invokes the original `0x0950` routine once per sample. It does not reimplement the DSP in Python. The only hook is at the original `0x0800` collector entry: it records the emitted bit and returns to the caller so the original sample loop can continue.

## 2. Per-sample C-versus-Thumb comparison

Pinned input recording:

- `rzwcl/450-TAx2/2026-10-11_17-30-46_audio_457550000Hz.wav`
- Git blob SHA-1: `8fb409ef803a98c17834fa230410c4579b030a83`
- 22,050 Hz, mono, signed 16-bit PCM, 1.696961 seconds
- Converted to 16,291 unsigned 12-bit samples at 9,600 samples/s for the target DSP probe.

The C harness exports one state row after each input sample. The Thumb emulator exports the corresponding state row from the original binary. The comparison covers:

| State | C/Thumb result |
|---|---|
| ADC input value | exact |
| Filter output `filter_y1` | exact |
| 1200 Hz I/Q accumulators | exact |
| 1800 Hz I/Q accumulators | exact |
| 1200 / 1800 smoothed baselines | exact |
| 1200 / 1800 sample phases | exact |
| Both discriminator previous-difference values | exact |
| Both discriminator phase accumulators | exact |

Latest successful run before the bitstream-path comparison fix:

- Actions run `42354796193`: [Thumb DSP execution and per-sample state comparison](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42354796193)
- `THUMB_DSP_EXECUTED samples=16291 collector_calls=3853 lane_bits=1988/1865`
- `STATE_COMPARE samples=16291 mismatching_fields=0`
- Every exported field had maximum absolute difference zero.

A follow-up commit updates the emulator's C bit-file lookup to the new explicit `original` / `balanced` profile names. The next CI run must show the two original C bitstreams match the original Thumb-emitted bitstreams exactly as well; do not mark that check complete until its logs say so.

## 3. Real WAV: original weights versus diagnostic-only weights

The independent 48 kHz reference demodulator finds the original 31-bit TDCS frame sync at bit 1,376 / 1.146667 s. A separate 9.6 kHz eight-sample quadrature-energy reference detector also decodes the recording: frame sync at bit 259 in the resampled stream, no BCH corrections required, valid header and padding, CRC `0xB27A`.

### Original binary initialization — unchanged

The original Thumb initializer writes:

- Lane A weights at state `+0x78`: `weight1200=2, weight1800=3`
- Lane B weights at state `+0x84`: `weight1200=3, weight1800=2`

The C implementation retains those exact defaults. With these original weights, the pinned sound-card WAV does not produce a valid frame. Across ADC scale factors 0.50–2.00, the C slicer lanes had nearest sync distances of 7–10 bits and aligned BER around 0.32–0.40; no frame callback was delivered.

### Balanced 1:1 weights — diagnostic experiment only

The real-WAV C harness then runs a second, explicitly labelled diagnostic pass. It changes only the two lane weight pairs to 1:1 **after** normal initialization. The C DSP operations, filter, coefficient tables, I/Q ring history, magnitude helper, smoothing, DPLL, frame collector, BCH, parser and CRC remain unchanged.

At scale 0.50, both C lanes emitted the same bitstream; it aligned with the reference at offset -23 with BER 0.0000. Both lanes decoded the same `X319` frame:

- speed: 20 km/h
- locomotive type: 237 (`0x00ED`)
- locomotive serial: 32 (`0032`)
- CRC: `0xB27A / 0xB27A`
- four valid frame callbacks were observed across the two independently reacquiring lanes.

This proves that the **C frame/BCH/CRC path can decode the field recording when its tone decision threshold is balanced for this recording**. It does not prove that the original binary's default weights decode this recording, nor does it justify changing the original default weights in a K6 port.

Likely boundary to investigate: the field WAV was captured through a sound-card/audio path, whose 1200/1800 Hz amplitude response may differ from the native receiver/ADC path for which the original 2:3 and 3:2 thresholds were chosen. That remains a hypothesis until a native K6 ADC trace or a controlled signal-generator test is available.

## 4. What must remain unchanged

- Keep original binary-derived DSP defaults 2:3 and 3:2.
- Do not replace the DSP with the simple 8-sample reference detector.
- Do not claim the 1:1 diagnostic pass is a faithful reproduction of original RX 0.3.
- Do not use a guessed amplitude correction in production firmware.
- Continue to treat the K6 BK4819 FIFO adapter as unverified until raw `REG_02/0B/5D/5E/5F` capture data establish sync stripping, byte order, bit order, polarity and FIFO length behaviour.

## 5. Next implementation steps

1. Finish the original 40-byte application record ABI map. The host `tdcsrx_message_t` is only a diagnostic structure and must not be passed directly to the recovered original UI callback.
2. Add an explicit conversion from the 45-byte validated frame to the exact 40-byte target application record after all dynamic parser offsets are resolved.
3. Complete the BK4819 FFSK FIFO raw-capture experiment on K6 or with a controllable conducted source.
4. Only after FIFO serialization is known, implement the separate receive-only TDCS adapter in the LOSEHU K6 firmware tree.
5. Keep LBJRX as a separate full-firmware target using the original `lbj_rx_bit()` decoder; do not merge the two receivers or alter the user's Bootloader.

**Status:** original sample-DSP C state translation is now instruction-level matched against the FAP1 on the pinned input. The original app callback ABI is partially recovered. The K6 hardware path, exact 40-byte record conversion, complete K6 firmware and real-radio verification remain unfinished.
