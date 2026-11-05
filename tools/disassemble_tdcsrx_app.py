#!/usr/bin/env python3
"""Fetch, validate and disassemble the public TDCSRX RX 0.3 FAP1 application."""
from __future__ import annotations
import argparse
import hashlib
import struct
import sys
import urllib.request
import zlib
from pathlib import Path

SOURCE_URL = "https://raw.githubusercontent.com/troilus/lbjradioweb/main/archive/apps/TDCSRX-cn.v5.5.6.app"
EXPECTED_GIT_BLOB_SHA1 = "af2142508e75b9ef2d2d2c4963be33ace5ee8523"
HEADER = struct.Struct("<4sHBBIIHH16s16sIIHH")
REGIONS = (
    (0x000, 0x090, "entry / radio initialization"),
    (0x090, 0x220, "sync / BCH decode"),
    (0x220, 0x430, "frame assembly / parser support"),
    (0x5C0, 0x800, "application frame parse / field decode"),
    (0x8F0, 0xB30, "audio DSP around recovered offset 0x950"),
    (0xB30, 0xE00, "display formatting / application UI"),
)

def fetch(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "K6-LBJ-RX-TDCSRX-Research/1.0"})
    with urllib.request.urlopen(request, timeout=45) as response:
        return response.read()

def git_blob_sha1(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()

def load_app(data: bytes):
    if len(data) < HEADER.size:
        raise ValueError(f"truncated FAP1 file: {len(data)} bytes")
    fields = HEADER.unpack_from(data)
    (magic, hdr_version, abi_major, api_min, code_size, code_crc32,
     entry_off, flags, name, version, link_vma, required_caps,
     asset_size, asset_crc) = fields
    if magic != b"FAP1":
        raise ValueError(f"bad magic: {magic!r}")
    if len(data) != HEADER.size + code_size + asset_size:
        raise ValueError(f"size mismatch: file={len(data)}, code={code_size}, assets={asset_size}")
    code = data[HEADER.size:HEADER.size + code_size]
    actual_crc = zlib.crc32(code) & 0xFFFFFFFF
    if actual_crc != code_crc32:
        raise ValueError(f"code CRC mismatch: header={code_crc32:08x}, computed={actual_crc:08x}")
    metadata = {
        "file_size": len(data), "blob_sha1": git_blob_sha1(data),
        "header_version": hdr_version, "abi_major": abi_major, "api_min": api_min,
        "code_size": code_size, "code_crc32": code_crc32, "entry_off": entry_off,
        "flags": flags, "name": name.split(b"\0", 1)[0].decode("ascii", "replace"),
        "version": version.split(b"\0", 1)[0].decode("ascii", "replace"),
        "link_vma": link_vma, "required_caps": required_caps,
        "asset_size": asset_size, "asset_crc": asset_crc,
    }
    return metadata, code

def disassemble(code: bytes, vma: int):
    from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_MCLASS
    import re

    engine = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_MCLASS)
    # Capstone skip-data mode is incompatible with detail mode. Keep it off:
    # BL targets are extracted from the rendered immediate operand instead.
    def skip_thumb_halfword(_code, _size, _offset, _user_data):
        return 2
    engine.skipdata_setup = (".hword", skip_thumb_halfword, None)
    engine.skipdata = True

    instructions = list(engine.disasm(code, vma))
    lines = []
    calls = {}
    for insn in instructions:
        offset = insn.address - vma
        raw = insn.bytes.hex(" ")
        lines.append(f"{offset:04x}  {insn.address:08x}  {raw:<11} {insn.mnemonic:<8} {insn.op_str}")
        if insn.mnemonic == "bl":
            match = re.search(r"#?(0x[0-9a-fA-F]+|[0-9]+)", insn.op_str)
            if match:
                target = int(match.group(1), 0) & ~1
                calls.setdefault(target - vma, []).append(offset)
    return "\n".join(lines) + "\n", calls

def render_report(metadata, full_disassembly, calls):
    out = ["# TDCSRX RX 0.3 — validated Thumb disassembly", "", "## Source verification", ""]
    for key, value in metadata.items():
        if key in ("code_crc32", "link_vma", "required_caps", "flags"):
            out.append(f"- {key}: 0x{int(value):08x}")
        else:
            out.append(f"- {key}: {value}")
    out += ["", "## Callers of known routines", ""]
    for target in (0x1D4, 0x204, 0x314, 0x61C, 0x950, 0xAD4, 0xBF4):
        callers = calls.get(target, [])
        rendered = ", ".join(f"0x{x:04x}" for x in callers) or "(no direct BL found)"
        out.append(f"- target 0x{target:04x}: {rendered}")
    out += ["", "## Selected instruction windows", ""]
    for start, end, label in REGIONS:
        out += [f"### 0x{start:04x}..0x{end:04x} — {label}", ""]
        for line in full_disassembly.splitlines():
            try:
                offset = int(line[:4], 16)
            except ValueError:
                continue
            if start <= offset < end:
                out.append("    " + line)
        out.append("")
    out += [
        "## Important boundaries", "",
        "- Disassembly is a linear Thumb sweep with invalid halfwords emitted as .hword data. Literal pools and embedded data that happen to decode as valid opcodes can still appear as instructions; verify control-flow before assigning function boundaries.",
        "- Capstone detail mode is disabled in skip-data mode; direct BL targets are parsed from rendered immediates. BLX register calls and indirect function pointers require separate data-flow analysis.",
        "- Code offsets are relative to the byte immediately after the 64-byte FAP1 header; runtime addresses add link_vma.",
        "- This is the published FAP1 application, not a standalone radio firmware and not original C source.",
        "- The DSP at offset 0x950 still requires semantic reconstruction; this report does not claim that the DSP is understood.",
        "",
    ]
    return "\n".join(out)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default=SOURCE_URL)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--output", type=Path, default=Path("tdcsrx_rx03_disassembly.md"))
    parser.add_argument("--raw-disassembly", type=Path, default=Path("tdcsrx_rx03_disassembly.asm"))
    args = parser.parse_args()
    data = args.binary.read_bytes() if args.binary else fetch(args.url)
    metadata, code = load_app(data)
    actual_blob_sha = metadata["blob_sha1"]
    if actual_blob_sha != EXPECTED_GIT_BLOB_SHA1:
        raise ValueError(f"unexpected Git blob SHA1: {actual_blob_sha}")
    disasm, calls = disassemble(code, int(metadata["link_vma"]))
    args.raw_disassembly.write_text(disasm, encoding="utf-8")
    args.output.write_text(render_report(metadata, disasm, calls), encoding="utf-8")
    decoded_offsets = [
        int(line[:4], 16) for line in disasm.splitlines()
        if len(line) >= 4 and all(c in "0123456789abcdef" for c in line[:4])
    ]
    instruction_count = sum(1 for line in disasm.splitlines()
                            if len(line) >= 4 and all(c in "0123456789abcdef" for c in line[:4])
                            and " .hword " not in line)
    if instruction_count < 400 or not decoded_offsets or max(decoded_offsets) < 0x0E00:
        raise RuntimeError(
            f"incomplete Thumb disassembly: {instruction_count} instruction rows, "
            f"last offset 0x{max(decoded_offsets, default=0):04x}"
        )
    print("TDCSRX FAP1 source validation: PASS")
    print(f"file size: {metadata['file_size']} bytes")
    print(f"Git blob SHA1: {actual_blob_sha}")
    print(f"code size: {metadata['code_size']} bytes")
    print(f"code CRC32: 0x{int(metadata['code_crc32']):08x}")
    print(f"link VMA: 0x{int(metadata['link_vma']):08x}")
    print(f"Thumb instructions decoded: {instruction_count}")
    print(f"report: {args.output}")
    print(f"raw disassembly: {args.raw_disassembly}")
    print("DSP context: offsets 0x08f0..0x0b30 in report")

if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
