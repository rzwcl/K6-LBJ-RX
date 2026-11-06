# LBJRX RX 0.4 — unmodified source reference

This folder contains source files copied from the public `troilus/k1lbj`
repository at commit `de6f77dd274f62c460b953d4ac639310906c256c`.

Upstream source directory:
https://github.com/troilus/k1lbj/tree/main/App/apps/lbj

The upstream `build.sh` identifies this app as `LBJ RX` version `0.4`,
API minimum `2`, linked at `0x20000280`. The published
`LBJRX-cn.v5.5.6.app` archive reports the same app version and API level.
That is a strong version/source match, but it is **not yet a reproducible
binary match**: this repository has not rebuilt the upstream app with its
original toolchain and compared its code CRC against the archived FAP1 image.

## Files

- `lbj_app.c`: application/UI and original receiver sample path.
- `lbj_dec.c/.h`: POCSAG bitstream decoder and report callback interface.
- `gen_assets.py`: original app asset generator.
- `app.ld`, `build.sh`: original app link/build configuration.

The source files retain their upstream contents and headers. The C files carry
Apache-2.0 headers; the upstream repository also has a GPL-3.0 root license.
Keep those notices when copying/adapting the files. This project-level copy is
for source preservation and porting reference; it is not itself a K6 firmware.

## Important porting boundary

The original app samples its audio input on PA4 through the F4HWN/PY32F071
ADC/DAC path at 19.2 ksample/s (16 samples per 1200-baud bit). That hardware
path must not be copied directly to the K6 DP32G030 board: the pinned LOSEHU
K6 GPIO map uses PA4 as keyboard column 1.

The portable part to preserve is the original bit-decoder/business callback
path beginning at `lbj_rx_bit()`. A K6 version should replace only the
sample/discriminator input with a separately validated BK4819 direct-FSK
receive/FIFO adapter, preserving this decoder's POCSAG sync, BCH/parity,
batch, address, function and message handling. Do not substitute the deleted
experimental `FirmwareOverlay` decoder.

## Original decoder provenance note

`lbj_dec.h` explicitly describes the POCSAG decoder as transcribed from the
author's model/test implementation and Sdr-Is-Fun's
`RTL_SDR_LBJ_RECEIVER`. It is the decoder used by this public LBJ RX 0.4 app
source tree, but it should not be represented as a direct decompilation of an
older LBJRX binary.
