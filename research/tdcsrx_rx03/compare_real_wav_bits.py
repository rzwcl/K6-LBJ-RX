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

def projection_9600(frequency):
    n = np.arange(8, dtype=np.float64)
    w = 2.0 * np.pi * frequency * n / 9600.0
    matrix = np.column_stack((np.cos(w), np.sin(w), np.ones(8)))
    q, _ = np.linalg.qr(matrix)
    return q.astype(np.float32)


Q1200_9600 = projection_9600(1200.0)
Q1800_9600 = projection_9600(1800.0)


def polynomial_remainder(value, polynomial=0x05B9):
    degree = polynomial.bit_length()
    while value and value.bit_length() >= degree:
        value ^= polynomial << (value.bit_length() - degree)
    return value


def crc16_xmodem(data):
    crc = 0
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = (((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)) & 0xFFFF
    return crc


def decode_simple_frame(bits, sync_position):
    wire = bits[sync_position + len(SYNC):sync_position + len(SYNC) + 23 * 26]
    if len(wire) != 23 * 26:
        return None

    syndrome_table = {
        polynomial_remainder(1 << bit): bit
        for bit in range(26)
    }
    decoded = bytearray()
    corrected = 0
    uncorrectable = []

    for word_index in range(23):
        start = word_index * 26
        codeword = int(wire[start:start + 26], 2)
        syndrome = polynomial_remainder(codeword)
        if syndrome:
            bit = syndrome_table.get(syndrome)
            if bit is None:
                uncorrectable.append((word_index, syndrome))
            else:
                codeword ^= 1 << bit
                corrected += 1
        decoded.extend((codeword >> 10).to_bytes(2, "big"))

    if uncorrectable:
        return {
            "ok": False,
            "corrected": corrected,
            "uncorrectable": uncorrectable,
        }

    frame = bytes(decoded[:45])
    padding = decoded[45]
    received_crc = int.from_bytes(frame[43:45], "big")
    calculated_crc = crc16_xmodem(frame[:43])
    header_ok = (
        frame[0:2] == b"\x0c\x2b"
        and frame[10] == 0x20
        and (
            frame[7:10] == b"\xff\xff\xff"
            or frame[7:10] == b"\x1f\x8c\x30"
        )
    )
    ok = (
        header_ok and padding == 0
        and received_crc == calculated_crc
    )
    return {
        "ok": ok,
        "corrected": corrected,
        "uncorrectable": [],
        "padding": padding,
        "received_crc": received_crc,
        "calculated_crc": calculated_crc,
        "header_ok": header_ok,
        "frame": frame,
    }


def compare_simple_9600(adc_path, ref_bits):
    raw = np.fromfile(adc_path, dtype="<u2")
    if raw.size == 0 or np.any(raw > 4095):
        raise SystemExit("9.6 kHz ADC file must contain unsigned 12-bit samples in u16le")
    samples = raw.astype(np.float32) - 2048.0
    best = (0.0, None, None, None)
    passing_frames = []

    for phase in range(8):
        count = min((len(samples) - phase) // 8, len(ref_bits))
        if count <= 0:
            continue
        windows = samples[phase:phase + count * 8].reshape(count, 8)
        e1200 = np.sum((windows @ Q1200_9600) ** 2, axis=1)
        e1800 = np.sum((windows @ Q1800_9600) ** 2, axis=1)
        decisions = e1200 > e1800
        bit_text = "".join("1" if value else "0" for value in decisions)
        reference = np.fromiter((bit == "1" for bit in ref_bits[:count]),
                                dtype=np.bool_, count=count)
        accuracy = float(np.mean(decisions == reference))
        if accuracy > best[0]:
            best = (accuracy, phase, int(np.count_nonzero(decisions)), count)

        for candidate_bits, inverted in (
            (bit_text, False),
            (bit_text.translate(str.maketrans("01", "10")), True),
        ):
            sync_position = candidate_bits.find(SYNC)
            if sync_position < 0:
                continue
            result = decode_simple_frame(candidate_bits, sync_position)
            if result is not None:
                print(f"SIMPLE_9600_FRAME phase={phase} sync_bit={sync_position} "
                      f"inverted={int(inverted)} corrections={result['corrected']} "
                      f"uncorrectable={len(result['uncorrectable'])} "
                      f"header_ok={int(result.get('header_ok', False))} "
                      f"crc={result.get('received_crc', 0):04X}/"
                      f"{result.get('calculated_crc', 0):04X} "
                      f"padding={result.get('padding', -1)} "
                      f"pass={int(result['ok'])}")
                if result["ok"]:
                    passing_frames.append((phase, sync_position, result))

    accuracy, phase, ones, count = best
    print(f"SIMPLE_9600_QUADRATURE best_phase={phase} "
          f"bit_agreement={accuracy:.4f} ones={ones}/{count}")
    if passing_frames:
        frame_phase, sync_position, result = passing_frames[0]
        print(f"SIMPLE_9600_FRAME_PASS phase={frame_phase} sync_bit={sync_position} "
              f"crc={result['received_crc']:04X} corrected={result['corrected']} "
              f"frame={result['frame'].hex().upper()}")
    else:
        print("SIMPLE_9600_FRAME_FAIL: no phase produced a BCH/CRC-valid frame")
    print("NOTE: this is an independent 8-sample reference detector, not the recovered TDCSRX DSP.")
    return accuracy


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
    if len(__import__("sys").argv) != 3:
        raise SystemExit("usage: compare_real_wav_bits.py SAMPLE_48K.wav SAMPLE_9600_ADC_U16LE.bin")
    samples = load_pcm(__import__("sys").argv[1])
    ref_sync, ref_bits = reference_bits(samples)
    compare_simple_9600(__import__("sys").argv[2], ref_bits)
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
