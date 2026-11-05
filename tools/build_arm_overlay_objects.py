#!/usr/bin/env python3
"""Compile only the LBJ overlay translation units for Cortex-M0/ARM EABI.

This is an object-level check with local interface stubs, not a full LOSEHU link.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    "FirmwareOverlay/app/lbj_app.c",
    "FirmwareOverlay/app/lbj_pocsag.c",
    "FirmwareOverlay/app/lbj_core.c",
    "FirmwareOverlay/app/lbj_loco.c",
    "FirmwareOverlay/app/lbj_train_type.c",
    "FirmwareOverlay/app/lbj_radio.c",
    "FirmwareOverlay/ui/lbj.c",
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", default="clang", help="Clang executable (default: clang)")
    parser.add_argument("--output", default="build-arm-obj", help="object output directory")
    args = parser.parse_args()

    clang = shutil.which(args.clang)
    if not clang:
        print(f"ERROR: Clang executable not found: {args.clang}", file=sys.stderr)
        return 2

    out_dir = (ROOT / args.output).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    base = [
        clang,
        "--target=arm-none-eabi",
        "-mcpu=cortex-m0",
        "-mthumb",
        "-std=c2x",
        "-fshort-enums",
        "-Os",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ffunction-sections",
        "-fdata-sections",
        f"-I{ROOT / 'build-arm-includes'}",
        f"-I{ROOT / 'tests/stubs'}",
        f"-I{ROOT / 'FirmwareOverlay'}",
        f"-I{ROOT / 'include'}",
    ]

    for relative in SOURCES:
        source = ROOT / relative
        output = out_dir / (source.stem + ".o")
        command = base + ["-c", str(source), "-o", str(output)]
        print("ARM compile:", relative)
        subprocess.run(command, check=True)

    print(f"ARM EABI overlay objects written to: {out_dir}")
    print("NOTE: this does not compile or link the LOSEHU base firmware.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"ERROR: ARM object compile failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode or 1)
