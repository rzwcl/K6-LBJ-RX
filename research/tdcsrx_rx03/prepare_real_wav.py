#!/usr/bin/env python3
"""Prepare the pinned 457.550 MHz field WAV for the recovered TDCSRX DSP probe."""

import array
import hashlib
import math
import struct
import sys
import wave
from pathlib import Path

EXPECTED_GIT_BLOB_SHA1 = "8fb409ef803a98c17834fa230410c4579b030a83"
INPUT_RATE = 22050
OUTPUT_RATE = 9600
CUTOFF_HZ = 4200.0
FILTER_RADIUS = 32
TARGET_P999 = 1100.0


def git_blob_sha1(data: bytes) -> str:
    header = b"blob " + str(len(data)).encode("ascii") + b"\0"
    return hashlib.sha1(header + data).hexdigest()


def resample_windowed_sinc(samples, input_rate, output_rate):
    """Band-limited arbitrary-ratio resampling, using a 64-tap Hann-like window."""
    output_count = int(round(len(samples) * output_rate / input_rate))
    cutoff = min(CUTOFF_HZ, output_rate * 0.46) / input_rate
    output = [0.0] * output_count

    for out_index in range(output_count):
        position = out_index * input_rate / output_rate
        center = int(math.floor(position))
        weighted_sum = 0.0
        weight_sum = 0.0
        first = max(0, center - FILTER_RADIUS)
        last = min(len(samples) - 1, center + FILTER_RADIUS)

        for source_index in range(first, last + 1):
            distance = source_index - position
            if abs(distance) > FILTER_RADIUS:
                continue
            if abs(distance) < 1.0e-12:
                sinc = 2.0 * cutoff
            else:
                sinc = math.sin(2.0 * math.pi * cutoff * distance) / (math.pi * distance)
            window = 0.5 + 0.5 * math.cos(math.pi * distance / (FILTER_RADIUS + 1))
            weight = sinc * window
            weighted_sum += samples[source_index] * weight
            weight_sum += weight

        output[out_index] = weighted_sum / weight_sum if abs(weight_sum) > 1.0e-15 else 0.0

    return output


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: prepare_real_wav.py INPUT.wav OUTPUT_ADC.bin")

    source = Path(sys.argv[1])
    target = Path(sys.argv[2])
    raw = source.read_bytes()
    actual_sha = git_blob_sha1(raw)
    if actual_sha != EXPECTED_GIT_BLOB_SHA1:
        raise SystemExit(
            "refusing unexpected sample: git blob SHA1 "
            f"{actual_sha} != {EXPECTED_GIT_BLOB_SHA1}"
        )

    with wave.open(str(source), "rb") as wav:
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        sample_rate = wav.getframerate()
        frames = wav.getnframes()
        if channels != 1 or sample_width != 2 or sample_rate != INPUT_RATE:
            raise SystemExit(
                f"unexpected WAV format: channels={channels}, width={sample_width}, "
                f"rate={sample_rate}"
            )
        pcm_bytes = wav.readframes(frames)

    pcm_array = array.array("h")
    pcm_array.frombytes(pcm_bytes)
    if sys.byteorder != "little":
        pcm_array.byteswap()
    pcm = [float(value) for value in pcm_array]

    # Centre the recording at zero before resampling. The TDCSRX target ADC
    # input is represented as unsigned 12-bit samples with a 2048 midpoint.
    dc = sum(pcm) / max(1, len(pcm))
    pcm = [value - dc for value in pcm]
    resampled = resample_windowed_sinc(pcm, sample_rate, OUTPUT_RATE)

    magnitudes = sorted(abs(value) for value in resampled)
    p999 = magnitudes[min(len(magnitudes) - 1, int((len(magnitudes) - 1) * 0.999))]
    if p999 < 1.0:
        raise SystemExit("recording is silent or too quiet to scale")

    scale = TARGET_P999 / p999
    adc = []
    for value in resampled:
        sample = int(round(2048.0 + value * scale))
        adc.append(max(0, min(4095, sample)))

    target.write_bytes(struct.pack("<" + "H" * len(adc), *adc))
    rms = math.sqrt(sum(value * value for value in resampled) / max(1, len(resampled)))
    print(f"source_git_blob_sha1={actual_sha}")
    print(f"source_samples={len(pcm)} source_rate={sample_rate} duration={len(pcm)/sample_rate:.6f}s")
    print(f"resampled_samples={len(adc)} output_rate={OUTPUT_RATE}")
    print(f"source_dc={dc:.3f} resampled_rms={rms:.3f} p99.9={p999:.3f} scale={scale:.6f}")
    print(f"adc_min={min(adc)} adc_max={max(adc)} output={target}")


if __name__ == "__main__":
    main()
