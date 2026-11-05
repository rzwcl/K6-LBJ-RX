#!/usr/bin/env python3
"""Read-only analysis of a K6 EEPROM backup for safe APP-slot planning."""
from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

EXPECTED_EEPROM_SIZES = (0x80000, 0x100000)
CANDIDATE_BASE = 0x2A400
SLOT_SIZE = 0x2000
SLOT_COUNT = 8
PAGE_SIZE = 0x1000


def ranges(data: bytes, start: int, end: int, page: int = PAGE_SIZE):
    out = []
    for pos in range(start, min(end, len(data)), page):
        block = data[pos:min(pos + page, end, len(data))]
        counts = Counter(block)
        out.append({
            "start": f"0x{pos:05X}",
            "end_exclusive": f"0x{pos + len(block):05X}",
            "length": len(block),
            "ff_bytes": counts.get(0xFF, 0),
            "00_bytes": counts.get(0x00, 0),
            "non_ff_bytes": len(block) - counts.get(0xFF, 0),
            "unique_values": len(counts),
            "dominant_byte": f"0x{counts.most_common(1)[0][0]:02X}",
            "dominant_count": counts.most_common(1)[0][1],
        })
    return out


def slot_report(data: bytes):
    slots = []
    for index in range(SLOT_COUNT):
        start = CANDIDATE_BASE + index * SLOT_SIZE
        end = start + SLOT_SIZE
        if end > len(data):
            slots.append({
                "slot": index,
                "start": f"0x{start:05X}",
                "end_exclusive": f"0x{end:05X}",
                "present": False,
            })
            continue
        block = data[start:end]
        counts = Counter(block)
        slots.append({
            "slot": index,
            "start": f"0x{start:05X}",
            "end_exclusive": f"0x{end:05X}",
            "present": True,
            "length": len(block),
            "ff_bytes": counts.get(0xFF, 0),
            "00_bytes": counts.get(0x00, 0),
            "non_ff_bytes": len(block) - counts.get(0xFF, 0),
            "ff_percent": round(100.0 * counts.get(0xFF, 0) / len(block), 3),
            "unique_values": len(counts),
            "first_64_hex": block[:64].hex(" "),
            "last_64_hex": block[-64:].hex(" "),
        })
    return slots


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--file", required=True, type=Path)
    p.add_argument("--report", required=True, type=Path)
    args = p.parse_args()

    raw = args.file.read_bytes()
    size = len(raw)
    candidate_end = CANDIDATE_BASE + SLOT_SIZE * SLOT_COUNT

    result = {
        "file": args.file.name,
        "size_bytes": size,
        "size_hex": f"0x{size:X}",
        "expected_512KiB": size == 0x80000,
        "expected_1MiB": size == 0x100000,
        "sha256": hashlib.sha256(raw).hexdigest(),
        "crc32": f"0x{__import__('zlib').crc32(raw) & 0xFFFFFFFF:08X}",
        "global": {
            "ff_bytes": raw.count(0xFF),
            "00_bytes": raw.count(0x00),
            "non_ff_bytes": size - raw.count(0xFF),
            "unique_values": len(set(raw)),
        },
        "known_layout_checks": {
            "bootloader_catalogue_start_0x40000": raw[0x40000:0x41000].hex(" ") if size >= 0x41000 else None,
            "stage_b_start_0x41000": raw[0x41000:0x44000].hex(" ") if size >= 0x44000 else None,
            "firmware_slot_area_0x44000_0x80000": (
                {
                    "length": len(raw[0x44000:0x80000]),
                    "ff_bytes": raw[0x44000:0x80000].count(0xFF),
                    "non_ff_bytes": len(raw[0x44000:0x80000]) - raw[0x44000:0x80000].count(0xFF),
                } if size >= 0x80000 else None
            ),
        },
        "candidate_app_zone": {
            "start": f"0x{CANDIDATE_BASE:05X}",
            "end_exclusive": f"0x{candidate_end:05X}",
            "length": SLOT_SIZE * SLOT_COUNT,
            "fully_present": candidate_end <= size,
            "slots": slot_report(raw),
            "pages_4k": ranges(raw, CANDIDATE_BASE, candidate_end),
        },
        "reference_ranges_4k": {
            "chinese_pinyin_region_0x20000_0x2A400": ranges(raw, 0x20000, 0x2A400),
            "gap_0x2A400_0x3C210": ranges(raw, 0x2A400, 0x3C210),
            "si4732_region_0x3C210_0x40000": ranges(raw, 0x3C210, 0x40000),
            "bootloader_and_multiboot_0x40000_0x80000": ranges(raw, 0x40000, min(0x80000, size)),
        },
        "first_256_hex": raw[:256].hex(" "),
    }

    # A range is considered bytewise erased only if every byte is 0xFF.
    # This is evidence about the backup contents, not proof that the radio
    # firmware has no custom meaning assigned to that region.
    result["candidate_app_zone"]["all_ff"] = (
        candidate_end <= size and all(b == 0xFF for b in raw[CANDIDATE_BASE:candidate_end])
    )
    result["candidate_app_zone"]["safe_to_write"] = False
    result["candidate_app_zone"]["reason"] = (
        "Read-only content analysis cannot establish semantic ownership; "
        "requires correlating the exact firmware build's EEPROM map and user data."
    )

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print("EEPROM BACKUP READ-ONLY ANALYSIS")
    print(f"File: {args.file.name}")
    print(f"Size: {size} bytes (0x{size:X})")
    print(f"SHA256: {result['sha256']}")
    print(f"CRC32: {result['crc32']}")
    print(f"Global FF: {result['global']['ff_bytes']}/{size}")
    print(f"Candidate zone: 0x{CANDIDATE_BASE:05X}..0x{candidate_end:05X}")
    print(f"Fully present: {result['candidate_app_zone']['fully_present']}")
    print(f"All 8 slots erased (all FF): {result['candidate_app_zone']['all_ff']}")
    for slot in result["candidate_app_zone"]["slots"]:
        if not slot["present"]:
            print(f"Slot {slot['slot']}: outside backup")
        else:
            print(
                f"Slot {slot['slot']} {slot['start']}..{slot['end_exclusive']}: "
                f"FF {slot['ff_bytes']}/{slot['length']} ({slot['ff_percent']}%), "
                f"non-FF {slot['non_ff_bytes']}"
            )
    print(f"Report: {args.report}")
    print("NOTE: this report never authorizes writing to any EEPROM address.")


if __name__ == "__main__":
    main()
