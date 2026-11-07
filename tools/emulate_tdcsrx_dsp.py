#!/usr/bin/env python3
"""Execute the original TDCSRX RX 0.3 Thumb DSP and compare it with the C translation.

The emulation calls the original routine at code offset 0x0950 once per ADC
sample. The original collector at 0x0800 is intercepted only to record each
emitted bit and return; all sample filtering, correlator, magnitude, baseline,
dual discriminator and DPLL instructions execute directly from the validated
FAP1 code bytes.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import struct
import urllib.request
import zlib
from pathlib import Path

from unicorn import Uc, UcError, UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_MCLASS, UC_MODE_THUMB
from unicorn.arm_const import (
    UC_ARM_REG_CPSR,
    UC_ARM_REG_LR,
    UC_ARM_REG_PC,
    UC_ARM_REG_R0,
    UC_ARM_REG_R1,
    UC_ARM_REG_R2,
    UC_ARM_REG_SP,
)

SOURCE_URL = (
    "https://raw.githubusercontent.com/troilus/lbjradioweb/main/"
    "archive/apps/TDCSRX-cn.v5.5.6.app"
)
EXPECTED_GIT_BLOB_SHA1 = "af2142508e75b9ef2d2d2c4963be33ace5ee8523"
HEADER = struct.Struct("<4sHBBIIHH16s16sIIHH")

LINK_VMA = 0x20000280
DSP_ENTRY = LINK_VMA + 0x0950
COLLECTOR_ENTRY = LINK_VMA + 0x0800
RETURN_ADDRESS = 0x20003F00
STATE_ADDRESS = 0x20002000
STACK_TOP = 0x20003E00
CALLBACK_POINTER = 0x20003E80

MEMORY_BASE = 0x20000000
MEMORY_SIZE = 0x4000
STATE_SIZE = 0x108

TRACE_COLUMNS = (
    "sample",
    "adc",
    "filter_y1",
    "corr1200_i",
    "corr1200_q",
    "corr1800_i",
    "corr1800_q",
    "smooth1200",
    "smooth1800",
    "phase1200",
    "phase1800",
    "diff_a",
    "phase_a",
    "diff_b",
    "phase_b",
)


def fetch_app() -> bytes:
    request = urllib.request.Request(
        SOURCE_URL,
        headers={"User-Agent": "K6-LBJ-RX-TDCSRX-Thumb-Emulator/1.0"},
    )
    with urllib.request.urlopen(request, timeout=45) as response:
        data = response.read()
    blob = hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()
    if blob != EXPECTED_GIT_BLOB_SHA1:
        raise RuntimeError(f"unexpected FAP1 Git blob SHA1: {blob}")
    if len(data) < HEADER.size:
        raise RuntimeError("truncated FAP1 application")
    fields = HEADER.unpack_from(data)
    magic, _hdr, _abi, _api, code_size, code_crc, _entry, _flags, _name, _version, vma, _caps, asset_size, _asset_crc = fields
    if magic != b"FAP1":
        raise RuntimeError(f"bad FAP1 magic: {magic!r}")
    if len(data) != HEADER.size + code_size + asset_size:
        raise RuntimeError("FAP1 size does not match header")
    code = data[HEADER.size:HEADER.size + code_size]
    actual_crc = zlib.crc32(code) & 0xFFFFFFFF
    if actual_crc != code_crc:
        raise RuntimeError(
            f"FAP1 code CRC mismatch: header={code_crc:08x}, actual={actual_crc:08x}"
        )
    if vma != LINK_VMA or code_size != 4076:
        raise RuntimeError(f"unexpected code layout: vma=0x{vma:08x}, size={code_size}")
    print(f"ORIGINAL_FAP1_PASS blob_sha1={blob} code_crc32={actual_crc:08X} vma={vma:08X}")
    return code


def initialize_original_state(uc: Uc) -> None:
    state = bytearray(STATE_SIZE)
    struct.pack_into("<i", state, 0x00, 0x8000)
    struct.pack_into("<i", state, 0x24, 16)
    struct.pack_into("<i", state, 0x28, 16)
    # The original Thumb initializer stores halfwords 0x0302 at +0x78
    # and 0x0203 at +0x84: [weight1200, weight1800] for each lane.
    struct.pack_into("<H", state, 0x78, 0x0302)
    struct.pack_into("<H", state, 0x84, 0x0203)
    uc.mem_write(STATE_ADDRESS, bytes(state))


def read_s32(uc: Uc, address: int) -> int:
    return struct.unpack("<i", bytes(uc.mem_read(address, 4)))[0]


def read_u8(uc: Uc, address: int) -> int:
    return bytes(uc.mem_read(address, 1))[0]


def capture_state(uc: Uc, sample_index: int, adc: int) -> tuple[int, ...]:
    base = STATE_ADDRESS
    return (
        sample_index,
        adc,
        read_s32(uc, base + 0x0C),
        read_s32(uc, base + 0x14),
        read_s32(uc, base + 0x18),
        read_s32(uc, base + 0x1C),
        read_s32(uc, base + 0x20),
        read_s32(uc, base + 0x24),
        read_s32(uc, base + 0x28),
        read_u8(uc, base + 0x2C),
        read_u8(uc, base + 0x2D),
        read_s32(uc, base + 0x70),
        read_s32(uc, base + 0x74),
        read_s32(uc, base + 0x7C),
        read_s32(uc, base + 0x80),
    )


def compare_c_trace(machine_trace: Path, c_trace: Path) -> int:
    with machine_trace.open("r", newline="", encoding="ascii") as file:
        machine_rows = list(csv.DictReader(file))
    with c_trace.open("r", newline="", encoding="ascii") as file:
        c_rows = list(csv.DictReader(file))

    if len(machine_rows) != len(c_rows):
        print(f"STATE_COMPARE length mismatch: asm={len(machine_rows)} C={len(c_rows)}")
        return 1

    fields = TRACE_COLUMNS[1:]
    mismatch_count = 0
    first_mismatch = None
    max_abs: dict[str, int] = {name: 0 for name in fields}

    for index, (asm, c_row) in enumerate(zip(machine_rows, c_rows)):
        for name in fields:
            av = int(asm[name])
            cv = int(c_row[name])
            delta = abs(av - cv)
            if delta > max_abs[name]:
                max_abs[name] = delta
            if av != cv:
                mismatch_count += 1
                if first_mismatch is None:
                    first_mismatch = (index, name, av, cv)

    print(f"STATE_COMPARE samples={len(machine_rows)} mismatching_fields={mismatch_count}")
    print("STATE_MAX_ABS " + " ".join(f"{k}={v}" for k, v in max_abs.items()))
    if first_mismatch:
        i, name, av, cv = first_mismatch
        print(f"STATE_FIRST_MISMATCH sample={i} field={name} thumb={av} c={cv}")
        left = max(0, i - 2)
        right = min(len(machine_rows), i + 4)
        for row in range(left, right):
            a = machine_rows[row]
            c = c_rows[row]
            print(
                f"  sample={row} adc={a['adc']} "
                f"thumb_filter={a['filter_y1']} c_filter={c['filter_y1']} "
                f"thumb_corr={a['corr1200_i']},{a['corr1200_q']},"
                f"{a['corr1800_i']},{a['corr1800_q']} "
                f"c_corr={c['corr1200_i']},{c['corr1200_q']},"
                f"{c['corr1800_i']},{c['corr1800_q']}"
            )
    else:
        print("STATE_COMPARE_PASS: every exported DSP state field matches the original Thumb code")
    return 0 if mismatch_count == 0 else 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("sample_bin", type=Path)
    parser.add_argument("--trace", type=Path, default=Path("tdcsrx_thumb_trace.csv"))
    parser.add_argument("--c-trace", type=Path, default=Path("research/tdcsrx_rx03/tdcsrx_real_wav_trace.csv"))
    parser.add_argument("--bits-prefix", default="tdcsrx_thumb_path")
    args = parser.parse_args()

    raw = args.sample_bin.read_bytes()
    if len(raw) == 0 or len(raw) % 2:
        raise SystemExit("sample file must contain 16-bit unsigned samples")
    samples = struct.unpack("<" + "H" * (len(raw) // 2), raw)
    if any(sample > 4095 for sample in samples):
        raise SystemExit("ADC samples exceed the 12-bit range")

    code = fetch_app()
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    uc.mem_map(MEMORY_BASE, MEMORY_SIZE)
    uc.mem_write(LINK_VMA, code)
    initialize_original_state(uc)

    emitted: tuple[list[str], list[str]] = ([], [])
    collector_hits = 0
    invalid_collectors = []

    def code_hook(machine: Uc, address: int, _size: int, _user_data) -> None:
        nonlocal collector_hits
        if address != COLLECTOR_ENTRY:
            return

        collector = machine.reg_read(UC_ARM_REG_R0)
        bit = machine.reg_read(UC_ARM_REG_R1) & 1
        if collector == STATE_ADDRESS + 0x88:
            emitted[0].append(str(bit))
        elif collector == STATE_ADDRESS + 0xC8:
            emitted[1].append(str(bit))
        else:
            invalid_collectors.append(collector)

        collector_hits += 1
        return_address = machine.reg_read(UC_ARM_REG_LR) & 0xFFFFFFFE
        machine.reg_write(UC_ARM_REG_PC, return_address)

    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.reg_write(UC_ARM_REG_SP, STACK_TOP)
    uc.reg_write(UC_ARM_REG_CPSR, 0x20)  # Thumb state
    uc.mem_write(RETURN_ADDRESS, b"\x70\x47")  # BX LR; emu_start stops before execution.

    with args.trace.open("w", newline="", encoding="ascii") as file:
        writer = csv.writer(file)
        writer.writerow(TRACE_COLUMNS)

        for index, sample in enumerate(samples):
            uc.reg_write(UC_ARM_REG_R0, STATE_ADDRESS)
            uc.reg_write(UC_ARM_REG_R1, int(sample))
            uc.reg_write(UC_ARM_REG_R2, CALLBACK_POINTER)
            uc.reg_write(UC_ARM_REG_LR, RETURN_ADDRESS | 1)
            try:
                uc.emu_start(DSP_ENTRY | 1, RETURN_ADDRESS, count=4096)
            except UcError as error:
                pc = uc.reg_read(UC_ARM_REG_PC)
                raise RuntimeError(
                    f"Thumb DSP emulation failed at sample {index}, PC=0x{pc:08X}: {error}"
                ) from error

            pc = uc.reg_read(UC_ARM_REG_PC) & 0xFFFFFFFE
            if pc != RETURN_ADDRESS:
                raise RuntimeError(
                    f"DSP did not return for sample {index}: PC=0x{pc:08X}"
                )
            writer.writerow(capture_state(uc, index, int(sample)))

    for path_index, bits in enumerate(emitted):
        Path(f"{args.bits_prefix}{path_index}.bits").write_text(
            "".join(bits) + "\n", encoding="ascii"
        )

    print(
        f"THUMB_DSP_EXECUTED samples={len(samples)} "
        f"collector_calls={collector_hits} lane_bits={len(emitted[0])}/{len(emitted[1])}"
    )
    if invalid_collectors:
        raise RuntimeError(f"unexpected collector pointers: {invalid_collectors[:4]}")

    bit_status = 0
    for path_index in range(2):
        c_bit_path = Path(f"research/tdcsrx_rx03/tdcsrx_path{path_index}_scale_100.bits")
        if c_bit_path.exists():
            c_bits = c_bit_path.read_text(encoding="ascii").strip()
            asm_bits = "".join(emitted[path_index])
            common = min(len(c_bits), len(asm_bits))
            errors = sum(a != b for a, b in zip(c_bits[:common], asm_bits[:common]))
            print(
                f"BIT_COMPARE lane={path_index} thumb={len(asm_bits)} C={len(c_bits)} "
                f"overlap={common} mismatches={errors}"
            )
            if len(c_bits) != len(asm_bits) or errors:
                bit_status = 1

    if not args.c_trace.exists():
        print(f"C trace not found: {args.c_trace}")
        return 2
    state_status = compare_c_trace(args.trace, args.c_trace)
    return state_status or bit_status


if __name__ == "__main__":
    raise SystemExit(main())
