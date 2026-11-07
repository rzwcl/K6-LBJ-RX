#!/usr/bin/env python3
"""Compare recovered TDCSRX sample-DSP decisions with the validated 48 kHz FFSK path."""

import glob
import wave
from pathlib import Path

import numpy as np

SYNC = f"{0x0DD4259F:032b}"[-31:]
RATE = 48000
SAMPLES_PER_BIT = 40


def projection(frequency):
    n = np.arange(SAMPLES_PER_BIT, dtype=np.float64)
    w = 2.0 * np.pi * frequency * n / RATE
    matrix = np.column_stack((np.cos(w), np.sin(w), np.ones(SAMPLES_PER_BIT)))
    q, _ = np.linalg.qr(matrix)
    return q.astype(np.float32)


Q1200 = projection(1200.0)
Q1800 = projection(1800.0)


def load_pcm(path):
    with wave.open(path, "rb") as wav:
        if wav.getframerate() != RATE or wav.getnchannels() != 1 or wav.getsampwidth() != 2:
            raise SystemExit("reference WAV must be 48 kHz mono signed 16-bit PCM")
        raw = wav.readframes(wav.getnframes())
    return np.frombuffer(raw, dtype="<i2")


def reference_bits(samples):
    candidates = []
    for phase in range(SAMPLES_PER_BIT):
        count = (len(samples) - phase) // SAMPLES_PER_BIT
        windows = samples[phase:phase + count * SAMPLES_PER_BIT].reshape(count, SAMPLES_PER_BIT).astype(np.float32)
        p1 = windows @ Q1200
        p0 = windows @ Q1800
        e1 = np.sum(p1 * p1, axis=1)
        e0 = np.sum(p0 * p0, axis=1)
        bits = "".join("1" if value else "0" for value in (e1 > e0))
        pos = bits.find(SYNC)
        inv_pos = bits.find("".join("1" if b == "0" else "0" for b in SYNC))
        if pos >= 0:
            candidates.append((abs(pos / 1200.0 - 1.146593), phase, pos, False, bits))
        if inv_pos >= 0:
            candidates.append((abs(inv_pos / 1200.0 - 1.146593), phase, inv_pos, True, bits))
    if not candidates:
        raise SystemExit("independent 48 kHz reference demodulator did not find exact sync")
    candidates.sort(key=lambda item: item[0])
    _, phase, sync_pos, inverted, bits = candidates[0]
    if inverted:
        bits = "".join("1" if b == "0" else "0" for b in bits)
    print(f"REFERENCE phase={phase} sync_bit={sync_pos} sync_time={sync_pos/1200:.6f}s "
          f"inverted={int(inverted)} bits={len(bits)} ones={bits.count('1')}")
    Path("tdcsrx_reference_48k.bits").write_text(bits + "\n", encoding="ascii")
    return sync_pos, bits


def min_sync_error(bits, expected_pos, radius=100):
    best = (32, -1, "")
    for inverted in (False, True):
        source = bits if not inverted else "".join("1" if b == "0" else "0" for b in bits)
        left = max(0, expected_pos - radius)
        right = min(len(source) - len(SYNC), expected_pos + radius)
        for pos in range(left, right + 1):
            candidate = source[pos:pos + len(SYNC)]
            errors = sum(a != b for a, b in zip(candidate, SYNC))
            if errors < best[0]:
                best = (errors, pos, candidate)
    return best


def compare(c_bits, ref_bits, ref_sync):
    # Align by the frame sync location first; the hardware DSP has its own
    # phase accumulator, so allow a small symbol-count offset.
    best = (1.0, None, None)
    for invert in (False, True):
        source = c_bits if not invert else "".join("1" if b == "0" else "0" for b in c_bits)
        for delta in range(-80, 81):
            c_start = ref_sync + delta
            r_start = ref_sync
            if c_start < 0:
                continue
            length = min(600, len(source) - c_start, len(ref_bits) - r_start)
            if length < 200:
                continue
            errors = sum(a != b for a, b in zip(source[c_start:c_start + length],
                                                ref_bits[r_start:r_start + length]))
            ratio = errors / length
            if ratio < best[0]:
                best = (ratio, delta, invert)
    return best


def main():
    if len(__import__("sys").argv) != 2:
        raise SystemExit("usage: compare_real_wav_bits.py SAMPLE_48K.wav")
    samples = load_pcm(__import__("sys").argv[1])
    ref_sync, ref_bits = reference_bits(samples)
    found = sorted(glob.glob("research/tdcsrx_rx03/tdcsrx_path*_scale_*.bits"))
    if not found:
        raise SystemExit("no exported C discriminator bitstreams found")

    for path in found:
        bits = Path(path).read_text(encoding="ascii").strip()
        hd, pos, got = min_sync_error(bits, ref_sync, radius=120)
        ratio, delta, inverted = compare(bits, ref_bits, ref_sync)
        print(f"C_BITS file={path} length={len(bits)} ones={bits.count('1')} "
              f"sync_min_hd={hd} sync_pos={pos} sync_delta={pos-ref_sync if pos >= 0 else 'NA'} "
              f"ref_ber={ratio:.4f} align_delta={delta} align_inverted={int(bool(inverted))}")
        if pos >= 0:
            print(f"  C_SYNC={got}")
            print(f"  REF_SYNC={SYNC}")


if __name__ == "__main__":
    main()
