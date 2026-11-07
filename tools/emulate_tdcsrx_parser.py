#!/usr/bin/env python3
"""Execute the original TDCSRX RX 0.3 Thumb frame parser against test frames.

The exact FAP1 parser at code offset 0x061C is run by Unicorn. This verifies
its input length convention, 40-byte compact output, variable index q, and
the two dynamic reads/stores without reimplementing the parser in Python.
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


def make_frame(train_number: int) -> bytes:
    if not 0 <= train_number <= 0xFFFFFF:
        raise ValueError("train number must fit in 24 bits")
    frame = bytearray(BASE_FRAME)
    frame[0x12:0x15] = train_number.to_bytes(3, "little")
    crc = crc16_xmodem(frame[:43])
    frame[43:45] = crc.to_bytes(2, "big")
    if len(frame) != 45:
        raise AssertionError(f"test frame must be 45 bytes, got {len(frame)}")
    return bytes(frame)


def execute_parser(code: bytes, train_number: int) -> tuple[int, bytes, bytes]:
    frame = make_frame(train_number)

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

    def trace_hook(machine, address, _size, _user_data):
        offset = (address & 0xFFFFFFFE) - LINK_VMA
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
    return result, compact, guard


def print_result(train_number: int, result: int, compact: bytes, guard: bytes) -> None:
    q = compact[0x25]
    q_offset = 0x1F + q
    dynamic_frame_index = 0x28 + q
    print(
        f"PARSER train={train_number} accepted={result} q={q} "
        f"dynamic_record_offset=0x{q_offset:02X} "
        f"dynamic_frame_index=0x{dynamic_frame_index:02X}"
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
        f"  auxiliary={compact[0x12:0x14].hex().upper()} "
        f"record_1f_27={compact[0x1F:0x28].hex().upper()} "
        f"reserved_26_27={compact[0x26:0x28].hex().upper()} "
        f"guard={guard.hex().upper()}"
    )


def main() -> None:
    code = fetch_app()

    # q is obtained by the original parser's decimal-threshold selection and
    # repeated subtraction. These examples exercise q=0, q=1, q=3, q=8 and
    # the explicit q=9 rejection branch.
    test_numbers = (319, 1234, 12345, 123456, 3_456_789, 8_765_432, 9_123_456)
    for train_number in test_numbers:
        result, compact, guard = execute_parser(code, train_number)
        print_result(train_number, result, compact, guard)

    # The collector supplies 46 decoded bytes, not a last-index value of 44.
    result_46, compact_46, guard_46 = execute_parser(code, 319)
    if result_46 != 1:
        raise AssertionError("original parser rejected the 46-byte collector buffer")
    if int.from_bytes(compact_46[4:8], "little") != 319:
        raise AssertionError("compact record train number mismatch")
    if compact_46[0x12] != 0x82 or compact_46[0x13] != 0xD2:
        raise AssertionError("q=1 must read frame[0x28] and frame[0x29]")
    if compact_46[0x26:0x28] != b"\xCC\xCC":
        raise AssertionError("unexpected parser writes into reserved bytes 0x26..0x27")
    if any(value != 0xCC for value in guard_46):
        raise AssertionError("parser wrote beyond the 40-byte compact record")

    result_9, _, _ = execute_parser(code, 9_123_456)
    if result_9 != 0:
        raise AssertionError("original parser should reject q=9")

    print("ORIGINAL_PARSER_ABI_PASS: 46-byte input accepted, 40-byte output bounded")


if __name__ == "__main__":
    main()
