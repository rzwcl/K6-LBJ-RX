#!/usr/bin/env python3
"""Build a fixed-VMA K6 overlay APP and pack it as a K6A1 EEPROM-slot image.

This is a compile/link/package probe. It does not install to a radio or produce
a main firmware image.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "FirmwareOverlay/apps/app_probe.c"
LINKER_SCRIPT = ROOT / "FirmwareOverlay/apps/k6_app.ld"
INCLUDE_DIR = ROOT / "FirmwareOverlay/apps"
DEFAULT_VMA = 0x20001800
DEFAULT_RAM_LIMIT = 0x1800
DEFAULT_SLOT_SIZE = 0x2000


def run(command: list[str]) -> None:
    print("+", " ".join(str(x) for x in command))
    subprocess.run(command, check=True)


def find_symbol(nm_text: str, symbol: str) -> int:
    for line in nm_text.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[-1] == symbol:
            return int(fields[0], 16)
    raise RuntimeError(f"symbol not found in ELF: {symbol}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, default=ROOT / "build/k6-app-probe")
    parser.add_argument("--gcc", default="arm-none-eabi-gcc")
    parser.add_argument("--objcopy", default="arm-none-eabi-objcopy")
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--name", default="APP PROBE")
    parser.add_argument("--version", default="0.1")
    parser.add_argument("--api-min", type=int, default=1)
    args = parser.parse_args()

    gcc = shutil.which(args.gcc)
    objcopy = shutil.which(args.objcopy)
    nm = shutil.which(args.nm)
    if not gcc or not objcopy or not nm:
        print("ERROR: ARM GCC binutils were not found on PATH", file=sys.stderr)
        return 2

    if not SOURCE.is_file() or not LINKER_SCRIPT.is_file():
        print("ERROR: K6 APP probe source/linker script missing", file=sys.stderr)
        return 2

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    obj = out_dir / "app_probe.o"
    elf = out_dir / "app_probe.elf"
    raw_bin = out_dir / "app_probe.bin"
    map_file = out_dir / "app_probe.map"
    packed = out_dir / "app_probe.k6app"

    compile_flags = [
        gcc,
        "-mcpu=cortex-m0",
        "-mthumb",
        "-std=c11",
        "-Os",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-common",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-fno-stack-protector",
        f"-I{INCLUDE_DIR}",
        "-c",
        str(SOURCE),
        "-o",
        str(obj),
    ]
    run(compile_flags)

    link_flags = [
        gcc,
        "-mcpu=cortex-m0",
        "-mthumb",
        "-nostdlib",
        "-Wl,--gc-sections",
        "-Wl,--no-undefined",
        "-Wl,--build-id=none",
        f"-Wl,-T,{LINKER_SCRIPT}",
        f"-Wl,-Map,{map_file}",
        str(obj),
        "-o",
        str(elf),
    ]
    run(link_flags)

    run([objcopy, "-O", "binary", "--only-section=.text", "--only-section=.data", str(elf), str(raw_bin)])
    nm_text = subprocess.run([nm, "-n", str(elf)], check=True, text=True, capture_output=True).stdout

    from k6_app_image import build_image, parse_image  # local tool module

    entry_symbol = find_symbol(nm_text, "k6_app_main")
    bss_start = find_symbol(nm_text, "__k6_app_bss_start")
    bss_end = find_symbol(nm_text, "__k6_app_bss_end")
    image_end = find_symbol(nm_text, "__k6_app_image_end")

    entry_offset = (entry_symbol & ~1) - DEFAULT_VMA
    bss_size = bss_end - bss_start
    image = raw_bin.read_bytes()
    expected_image_size = image_end - DEFAULT_VMA

    if expected_image_size != len(image):
        raise RuntimeError(
            f"ELF image span ({expected_image_size}) != objcopy BIN size ({len(image)})"
        )
    if bss_start != image_end:
        raise RuntimeError(
            f"BSS begins at 0x{bss_start:08X}, image ends at 0x{image_end:08X}"
        )
    if entry_offset < 0 or entry_offset >= len(image) or (entry_offset & 1):
        raise RuntimeError(f"invalid Thumb entry offset: 0x{entry_offset:X}")

    packed_bytes = build_image(
        image,
        name=args.name,
        version=args.version,
        api_min=args.api_min,
        entry_offset=entry_offset,
        link_vma=DEFAULT_VMA,
        bss_size=bss_size,
        ram_limit=DEFAULT_RAM_LIMIT,
        slot_size=DEFAULT_SLOT_SIZE,
    )
    packed.write_bytes(packed_bytes)
    info = parse_image(packed_bytes)

    report = {
        "kind": "K6A1 APP probe; not a firmware image and not yet installable",
        "name": info.name,
        "version": info.version,
        "abi_major": 1,
        "api_min": info.api_min,
        "link_vma": f"0x{info.link_vma:08X}",
        "entry_offset": info.entry_offset,
        "image_size": info.image_size,
        "bss_size": info.bss_size,
        "image_plus_bss": info.image_size + info.bss_size,
        "overlay_ram_limit": DEFAULT_RAM_LIMIT,
        "slot_file_size": len(packed_bytes),
        "slot_file_limit": DEFAULT_SLOT_SIZE,
        "crc32": f"0x{info.image_crc32:08X}",
        "sha256": hashlib.sha256(packed_bytes).hexdigest(),
        "paths": {
            "elf": str(elf),
            "raw_bin": str(raw_bin),
            "packed": str(packed),
            "map": str(map_file),
        },
    }
    report_path = out_dir / "report.json"
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print("\nK6 APP probe build passed")
    for key in (
        "name", "version", "abi_major", "api_min", "link_vma", "entry_offset",
        "image_size", "bss_size", "image_plus_bss", "overlay_ram_limit",
        "slot_file_size", "slot_file_limit", "crc32", "sha256"
    ):
        print(f"{key}: {report[key]}")
    print(f"report: {report_path}")
    print("NOTE: this is only an APP compile/link/package test; no K6 loader or installer is included.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
