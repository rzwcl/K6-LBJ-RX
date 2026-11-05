#!/usr/bin/env python3
"""Verify and inspect a LOSEHU fw-pack.py packed firmware image."""

from __future__ import annotations

import argparse
import binascii
import json
import re
import struct
from pathlib import Path


def load_obfuscation_key(packer: Path) -> bytes:
    source = packer.read_text(encoding="utf-8")
    match = re.search(r"OBFUSCATION\s*=\s*\[([\s\S]*?)\]", source)
    if not match:
        raise ValueError(f"OBFUSCATION table not found in {packer}")
    values = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))
    if len(values) != 128:
        raise ValueError(f"expected 128 obfuscation bytes, found {len(values)}")
    return values


def inspect_image(image: Path, packer: Path) -> dict:
    packed = image.read_bytes()
    if len(packed) < 0x2012:
        raise ValueError("image is too short to contain LOSEHU header and CRC")

    key = load_obfuscation_key(packer)
    stored_crc = packed[-2] | (packed[-1] << 8)
    calculated_crc = binascii.crc_hqx(packed[:-2], 0)
    if stored_crc != calculated_crc:
        raise ValueError(
            f"CRC-16/XMODEM mismatch: stored=0x{stored_crc:04X}, "
            f"calculated=0x{calculated_crc:04X}"
        )

    decoded = bytes(value ^ key[index % len(key)] for index, value in enumerate(packed[:-2]))
    version_field = decoded[0x2000:0x2010].rstrip(b"\0")
    raw = decoded[:0x2000] + decoded[0x2010:]
    if len(raw) != len(packed) - 18:
        raise ValueError("unexpected packed image layout")

    msp, reset = struct.unpack_from("<II", raw, 0)
    return {
        "file": str(image),
        "packed_bytes": len(packed),
        "raw_firmware_bytes": len(raw),
        "version_field": version_field.decode("ascii", errors="replace"),
        "crc16_xmodem": f"0x{stored_crc:04X}",
        "crc_valid": True,
        "obfuscation_key_bytes": len(key),
        "initial_msp": f"0x{msp:08X}",
        "reset_vector": f"0x{reset:08X}",
        "msp_in_16k_sram": 0x20000000 <= msp <= 0x20004000,
        "reset_vector_in_60k_flash": 0 <= (reset & ~1) < 60 * 1024,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--packer", required=True, type=Path, help="upstream fw-pack.py")
    parser.add_argument("--report", type=Path, help="optional JSON report path")
    args = parser.parse_args()

    report = inspect_image(args.image, args.packer)
    rendered = json.dumps(report, ensure_ascii=False, indent=2)
    print(rendered)
    if args.report:
        args.report.write_text(rendered + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        raise SystemExit(f"ERROR: {exc}")
