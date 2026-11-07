#!/usr/bin/env python3
"""Execute the original TDCSRX RX 0.3 Thumb frame parser against test frames.

The exact FAP1 parser at code offset 0x061C is run by Unicorn. This verifies
its 46-byte collector input, the 40-byte compact output ABI, the decimal
leading-digit validation quotient, fixed index-one copies, header-variant
flag, and untouched output tail without reimplementing the parser in Python.
"""

from __future__ import annotations

import hashlib
import struct
import urllib.request
import zlib

from unicorn import Uc, UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_MCLASS, UC_MODE_THUMB
from unicorn.arm_const import (
    UC_ARM_REG_CPSR,
    UC_ARM_REG_LR,
    UC_ARM_REG_PC,
    UC_ARM_REG_R0,
    UC_ARM_REG_R1,
    UC_ARM_REG_R2,
    UC_ARM_REG_R3,
    UC_ARM_REG_R4,
    UC_ARM_REG_R5,
    UC_ARM_REG_R6,
    UC_ARM_REG_R7,
    UC_ARM_REG_SP,
)

SOURCE_URL = (
    "https://raw.githubusercontent.com/troilus/lbjradioweb/main/"
    "archive/apps/TDCSRX-cn.v5.5.6.app"
)
EXPECTED_GIT_BLOB_SHA1 = "af2142508e75b9ef2d2d2c4963be33ace5ee8523"
HEADER = struct.Struct("<4sHBBIIHH16s16sIIHH")
LINK_VMA = 0x20000280
PARSER_ENTRY = LINK_VMA + 0x061C
RETURN_ADDRESS = 0x20003F00
STACK_TOP = 0x20003E00
MEMORY_BASE = 0x20000000
MEMORY_SIZE = 0x4000
INPUT_ADDRESS = 0x20003000
OUTPUT_ADDRESS = 0x20003200
INPUT_BYTES = 64
OUTPUT_BYTES = 56
DECODED_BYTES = 46

BASE_FRAME = bytes.fromhex(
    "0C2B003F1F00001F8C3020000004202020583F0100ED002000"
    "6D374314C00048042D011219010E0E82D24FB27A"
)


def fetch_app() -> bytes:
    request = urllib.request.Request(
        SOURCE_URL,
        headers={"User-Agent": "K6-LBJ-RX-TDCSRX-Parser-Emulator/1.0"},
    )
    with urllib.request.urlopen(request, timeout=45) as response:
        data = response.read()

    blob = hashlib.sha1(
        b"blob " + str(len(data)).encode("ascii") + b"\0" + data
    ).hexdigest()
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
            f"FAP1 code CRC mismatch: header={code_crc:08X}, actual={actual_crc:08X}"
        )
    if vma != LINK_VMA or code_size != 4076:
        raise RuntimeError(
            f"unexpected code layout: vma=0x{vma:08X}, size={code_size}"
        )
    print(
        f"ORIGINAL_FAP1_PASS blob_sha1={blob} "
        f"code_crc32={actual_crc:08X} vma={vma:08X}"
    )
    return code


def crc16_xmodem(data: bytes) -> int:
    crc = 0
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def make_frame(
    train_number: int,
    *,
    frame_byte_27: int | None = None,
    frame_byte_29: int | None = None,
    ff_header: bool = False,
) -> bytes:
    if not 0 <= train_number <= 0xFFFFFF:
        raise ValueError("train number must fit in 24 bits")
    frame = bytearray(BASE_FRAME)
    if ff_header:
        frame[7:10] = b"\\xff\\xff\\xff"
    frame[0x12:0x15] = train_number.to_bytes(3, "little")
    if frame_byte_27 is not None:
        frame[0x27] = frame_byte_27 & 0xFF
    if frame_byte_29 is not None:
        frame[0x29] = frame_byte_29 & 0xFF
    crc = crc16_xmodem(frame[:43])
    frame[43:45] = crc.to_bytes(2, "big")
    if len(frame) != 45:
        raise AssertionError(f"test frame must be 45 bytes, got {len(frame)}")
    return bytes(frame)


def execute_parser(
    code: bytes,
    train_number: int,
    *,
    frame_byte_27: int | None = None,
    frame_byte_29: int | None = None,
    ff_header: bool = False,
) -> tuple[int, bytes, bytes, int | None, int | None, int | None]:
    frame = make_frame(
        train_number,
        frame_byte_27=frame_byte_27,
        frame_byte_29=frame_byte_29,
        ff_header=ff_header,
    )

    # The collector has a 64-byte path buffer. It writes 46 decoded bytes
    # (23 BCH words, the final low byte being padding); the remaining bytes
    # stay as zero-initialised collector storage in this controlled test.
    input_buffer = bytearray(INPUT_BYTES)
    input_buffer[:45] = frame
    input_buffer[45] = 0
    output_buffer = bytes([0xCC]) * OUTPUT_BYTES

    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    uc.mem_map(MEMORY_BASE, MEMORY_SIZE)
    uc.mem_write(LINK_VMA, code)
    uc.mem_write(INPUT_ADDRESS, bytes(input_buffer))
    uc.mem_write(OUTPUT_ADDRESS, output_buffer)
    uc.mem_write(RETURN_ADDRESS, b"\x70\x47")

    uc.reg_write(UC_ARM_REG_SP, STACK_TOP)
    uc.reg_write(UC_ARM_REG_CPSR, 0x20)
    uc.reg_write(UC_ARM_REG_R0, INPUT_ADDRESS)
    uc.reg_write(UC_ARM_REG_R1, DECODED_BYTES)
    uc.reg_write(UC_ARM_REG_R2, OUTPUT_ADDRESS)
    uc.reg_write(UC_ARM_REG_LR, RETURN_ADDRESS | 1)

    trace_offsets = {
        0x06C0, 0x06C2, 0x06C6, 0x06C8, 0x06CE, 0x06D0,
        0x06D6, 0x06D8, 0x06DC, 0x06E0, 0x06E2, 0x06E6,
        0x06E8, 0x06EE, 0x06F0, 0x06F4, 0x06F6, 0x07E4,
        0x07E6, 0x07E8, 0x07EA, 0x0700, 0x0702, 0x0704,
        0x0708, 0x070A, 0x0710, 0x0712, 0x0714, 0x0720,
        0x072A, 0x0730, 0x0740, 0x0750, 0x0760, 0x0770,
        0x0780, 0x0790, 0x0798, 0x079A, 0x079C, 0x07A0,
        0x07A2, 0x07A4, 0x07A6, 0x07A8, 0x07AA, 0x07B0,
        0x07B2, 0x07B4, 0x07B6, 0x07B8, 0x07BA, 0x07BC,
        0x07BE,
    }
    trace_rows = []
    quotient_trace: list[tuple[int, int, int]] = []

    def trace_hook(machine, address, _size, _user_data):
        offset = (address & 0xFFFFFFFE) - LINK_VMA
        if offset == 0x06FA:
            quotient_trace.append((
                machine.reg_read(UC_ARM_REG_R1),
                machine.reg_read(UC_ARM_REG_R3),
                machine.reg_read(UC_ARM_REG_R4),
            ))
        if train_number == 319 and offset in trace_offsets:
            trace_rows.append((
                offset,
                machine.reg_read(UC_ARM_REG_R1),
                machine.reg_read(UC_ARM_REG_R3),
                machine.reg_read(UC_ARM_REG_R4),
                machine.reg_read(UC_ARM_REG_R5),
                machine.reg_read(UC_ARM_REG_R2),
                machine.reg_read(UC_ARM_REG_R6),
                machine.reg_read(UC_ARM_REG_R7),
            ))

    uc.hook_add(UC_HOOK_CODE, trace_hook)
    uc.emu_start(PARSER_ENTRY | 1, RETURN_ADDRESS, count=20000)
    if train_number == 319:
        for offset, r1, r3, r4, r5, r2, r6, r7 in trace_rows:
            print(
                f"  TRACE offset=0x{offset:04X} r1={r1} r2=0x{r2:08X} "
                f"r3={r3} r4=0x{r4:08X} r5=0x{r5:08X} "
                f"r6=0x{r6:08X} r7=0x{r7:08X}"
            )
    pc = uc.reg_read(UC_ARM_REG_PC) & 0xFFFFFFFE
    if pc != RETURN_ADDRESS:
        raise RuntimeError(f"parser failed to return: PC=0x{pc:08X}")

    result = uc.reg_read(UC_ARM_REG_R0)
    compact = bytes(uc.mem_read(OUTPUT_ADDRESS, 40))
    guard = bytes(uc.mem_read(OUTPUT_ADDRESS + 40, OUTPUT_BYTES - 40))
    quotient, remainder, divisor = (
        quotient_trace[-1] if quotient_trace else (None, None, None)
    )
    return result, compact, guard, quotient, remainder, divisor


def print_result(
    train_number: int,
    result: int,
    compact: bytes,
    guard: bytes,
    quotient: int | None,
    remainder: int | None,
    divisor: int | None,
) -> None:
    print(
        f"PARSER train={train_number} accepted={result} "
        f"leading_digit_quotient={quotient} remainder={remainder} divisor={divisor} "
        "dynamic_record_offsets=0x12..0x13,0x1F..0x20 "
        "dynamic_frame_indices=0x28..0x29,0x26..0x27"
    )
    print("  record40=" + compact.hex(" ").upper())
    print(
        f"  prefix={compact[0:4].hex().upper()} "
        f"train={int.from_bytes(compact[4:8], 'little')} "
        f"loco_type={int.from_bytes(compact[0x16:0x18], 'little')} "
        f"serial={int.from_bytes(compact[0x0C:0x0E], 'little')} "
        f"speed={int.from_bytes(compact[0x14:0x16], 'little')}"
    )
    print(
        f"  record_12_13={compact[0x12:0x14].hex().upper()} "
        f"record_1F_25={compact[0x1F:0x26].hex().upper()} "
        f"record_26_27_untouched={compact[0x26:0x28].hex().upper()} "
        f"header_variant={compact[0x24]} index_byte={compact[0x25]} "
        f"guard={guard.hex().upper()}
    )


def main() -> None:
    code = fetch_app()

    # The parser derives a leading decimal digit quotient by repeated
    # subtraction. It rejects quotient 9. This validation value is NOT the
    # later record byte +0x25 and does NOT control the two fixed index-one
    # copies at record +0x13 and +0x20.
    expected = {
        0: (1, 0, 1),
        1: (1, 1, 1),
        319: (1, 3, 100),
        1234: (1, 1, 1000),
        8_765_432: (1, 8, 1_000_000),
        9_123_456: (0, 9, 1_000_000),
    }
    test_numbers = tuple(expected)
    parser_results = {}
    for train_number in test_numbers:
        result, compact, guard, quotient, remainder, divisor = execute_parser(
            code, train_number
        )
        print_result(
            train_number, result, compact, guard, quotient, remainder, divisor
        )
        expected_result, expected_quotient, expected_divisor = expected[train_number]
        if result != expected_result:
            raise AssertionError(
                f"train {train_number}: accepted={result}, expected={expected_result}"
            )
        if quotient != expected_quotient or divisor != expected_divisor:
            raise AssertionError(
                f"train {train_number}: quotient/divisor={quotient}/{divisor}, "
                f"expected={expected_quotient}/{expected_divisor}"
            )
        parser_results[train_number] = (compact, guard, remainder)

    compact_319, guard_319, remainder_319 = parser_results[319]
    if remainder_319 != 19:
        raise AssertionError(f"train 319 remainder mismatch: {remainder_319}")
    if int.from_bytes(compact_319[4:8], "little") != 319:
        raise AssertionError("compact record train number mismatch")
    if compact_319[0x12:0x14] != b"\\x82\\xd2":
        raise AssertionError("record +0x12..+0x13 must copy frame[0x28..0x29]")
    if compact_319[0x1F:0x21] != b"\\x0e\\x0e":
        raise AssertionError("record +0x1F..+0x20 must copy frame[0x26..0x27]")
    if compact_319[0x24] != 0:
        raise AssertionError("1F 8C 30 header variant flag must be zero")
    if compact_319[0x25] != 1:
        raise AssertionError("record +0x25 must preserve the target's constant index byte 1")
    if compact_319[0x26:0x28] != b"\\xcc\\xcc":
        raise AssertionError("parser unexpectedly wrote untouched bytes 0x26..0x27")
    if any(value != 0xCC for value in guard_319):
        raise AssertionError("parser wrote beyond the 40-byte compact record")

    # Distinct byte markers prove the parser uses literal index one rather
    # than the train-number leading-digit quotient for both dynamic copies.
    result_marked, marked, marked_guard, marked_quotient, _, _ = execute_parser(
        code, 319, frame_byte_27=0xA6, frame_byte_29=0x5C
    )
    if result_marked != 1 or marked_quotient != 3:
        raise AssertionError("marked dynamic-copy test failed parser validation")
    if marked[0x20] != 0xA6 or marked[0x13] != 0x5C:
        raise AssertionError("parser did not copy fixed frame indices 0x27 and 0x29")
    if any(value != 0xCC for value in marked_guard):
        raise AssertionError("marked parser wrote beyond the 40-byte record")

    result_ff, compact_ff, _, q_ff, _, _ = execute_parser(
        code, 319, ff_header=True
    )
    if result_ff != 1 or q_ff != 3:
        raise AssertionError("FF FF FF header variant was rejected")
    if compact_ff[0x24] != 1:
        raise AssertionError("FF FF FF header variant flag must be one")

    print(
        "ORIGINAL_PARSER_ABI_PASS: 46-byte input, 40-byte output, "
        "leading-digit validation quotient 0/1/3/8/9, fixed index-one "
        "copies, both header variants, untouched tail"
    )


if __name__ == "__main__":
    main()
