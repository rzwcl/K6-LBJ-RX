#!/usr/bin/env python3
"""Pack and inspect K6-native overlay application images.

The K6A1 container is intentionally not binary-compatible with F4HWN FAP1
applications. It uses the same general idea (a small header plus a position-
independent-in-practice, fixed-VMA ARM image), but a separate K6 API/ABI.

Image layout:
    0x00..0x3F  64-byte little-endian K6A1 header
    0x40..      raw application image (.text + .rodata + initialized .data)

The BSS is not stored in the file; its byte count is recorded in the header and
cleared by the resident K6 loader after copying the image to SRAM.
"""
from __future__ import annotations

import argparse
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path

MAGIC = b"K6A1"
HEADER_VERSION = 1
ABI_MAJOR = 1
API_LEVEL = 1
HEADER = struct.Struct("<4sHBBIIHH16s16sIII")
HEADER_SIZE = HEADER.size
DEFAULT_LINK_VMA = 0x20001800
DEFAULT_RAM_LIMIT = 0x1800       # 6 KiB overlay; leave 4 KiB for main stack
DEFAULT_SLOT_SIZE = 0x2000       # 8 KiB EEPROM slot
DEFAULT_SLOT_COUNT = 8

# Header flag bit 0 is set only after the complete image has been written.
FLAG_COMMITTED = 0x0001


class ImageError(ValueError):
    """Invalid K6 application image."""


@dataclass(frozen=True)
class AppImage:
    name: str
    version: str
    api_min: int
    image_size: int
    image_crc32: int
    entry_offset: int
    flags: int
    link_vma: int
    bss_size: int
    required_caps: int
    payload: bytes


def _encode_fixed(text: str, length: int, field: str) -> bytes:
    try:
        raw = text.encode("ascii")
    except UnicodeEncodeError as exc:
        raise ImageError(f"{field} must contain ASCII only") from exc
    if not raw or len(raw) >= length:
        raise ImageError(f"{field} must contain 1..{length - 1} ASCII bytes")
    return raw + bytes(length - len(raw))


def _decode_fixed(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")


def build_image(
    payload: bytes,
    *,
    name: str,
    version: str,
    api_min: int = API_LEVEL,
    entry_offset: int = 0,
    flags: int = FLAG_COMMITTED,
    link_vma: int = DEFAULT_LINK_VMA,
    bss_size: int = 0,
    required_caps: int = 0,
    ram_limit: int = DEFAULT_RAM_LIMIT,
    slot_size: int = DEFAULT_SLOT_SIZE,
) -> bytes:
    if not payload:
        raise ImageError("application payload is empty")
    if len(payload) + HEADER_SIZE > slot_size:
        raise ImageError(
            f"image is {len(payload) + HEADER_SIZE} bytes; slot limit is {slot_size}"
        )
    if len(payload) + bss_size > ram_limit:
        raise ImageError(
            f"image+BSS is {len(payload) + bss_size} bytes; RAM limit is {ram_limit}"
        )
    if not 0 <= api_min <= 255:
        raise ImageError("api_min must be in 0..255")
    if not 0 <= entry_offset < len(payload):
        raise ImageError("entry_offset must point inside the payload")
    if entry_offset & 1:
        raise ImageError("entry_offset must be halfword-aligned")
    if not 0 <= flags <= 0xFFFF:
        raise ImageError("flags must be in 0..65535")
    if not 0 <= link_vma <= 0xFFFFFFFF:
        raise ImageError("link_vma must be a 32-bit address")
    if not 0 <= bss_size <= 0xFFFFFFFF:
        raise ImageError("bss_size must be a 32-bit size")
    if not 0 <= required_caps <= 0xFFFFFFFF:
        raise ImageError("required_caps must be a 32-bit bitmask")

    checksum = zlib.crc32(payload) & 0xFFFFFFFF
    header = HEADER.pack(
        MAGIC,
        HEADER_VERSION,
        ABI_MAJOR,
        api_min,
        len(payload),
        checksum,
        entry_offset,
        flags,
        _encode_fixed(name, 16, "name"),
        _encode_fixed(version, 16, "version"),
        link_vma,
        bss_size,
        required_caps,
    )
    return header + payload


def parse_image(data: bytes, *, ram_limit: int = DEFAULT_RAM_LIMIT,
                slot_size: int = DEFAULT_SLOT_SIZE) -> AppImage:
    if len(data) < HEADER_SIZE:
        raise ImageError(f"file is shorter than the {HEADER_SIZE}-byte header")
    (
        magic,
        header_version,
        abi_major,
        api_min,
        image_size,
        image_crc32,
        entry_offset,
        flags,
        raw_name,
        raw_version,
        link_vma,
        bss_size,
        required_caps,
    ) = HEADER.unpack_from(data)

    if magic != MAGIC:
        raise ImageError(f"wrong magic {magic!r}; expected {MAGIC!r}")
    if header_version != HEADER_VERSION:
        raise ImageError(f"unsupported header version {header_version}")
    if abi_major != ABI_MAJOR:
        raise ImageError(f"unsupported K6 APP ABI major {abi_major}")
    if not (flags & FLAG_COMMITTED):
        raise ImageError("image is not marked committed")
    if image_size == 0 or HEADER_SIZE + image_size != len(data):
        raise ImageError("payload length does not match the header")
    if len(data) > slot_size:
        raise ImageError(f"file is {len(data)} bytes; slot limit is {slot_size}")
    if image_size + bss_size > ram_limit:
        raise ImageError(
            f"image+BSS is {image_size + bss_size} bytes; RAM limit is {ram_limit}"
        )
    if entry_offset >= image_size or entry_offset & 1:
        raise ImageError("entry offset is invalid or not halfword-aligned")

    payload = data[HEADER_SIZE:]
    actual_crc = zlib.crc32(payload) & 0xFFFFFFFF
    if actual_crc != image_crc32:
        raise ImageError(
            f"CRC32 mismatch: header 0x{image_crc32:08X}, actual 0x{actual_crc:08X}"
        )

    return AppImage(
        name=_decode_fixed(raw_name),
        version=_decode_fixed(raw_version),
        api_min=api_min,
        image_size=image_size,
        image_crc32=image_crc32,
        entry_offset=entry_offset,
        flags=flags,
        link_vma=link_vma,
        bss_size=bss_size,
        required_caps=required_caps,
        payload=payload,
    )


def _parse_int(value: str) -> int:
    return int(value, 0)


def cmd_pack(args: argparse.Namespace) -> int:
    payload = Path(args.bin).read_bytes()
    image = build_image(
        payload,
        name=args.name,
        version=args.version,
        api_min=args.api_min,
        entry_offset=args.entry_offset,
        link_vma=args.link_vma,
        bss_size=args.bss_size,
        required_caps=args.required_caps,
        ram_limit=args.ram_limit,
        slot_size=args.slot_size,
    )
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(image)
    info = parse_image(image, ram_limit=args.ram_limit, slot_size=args.slot_size)
    print(f"Output:       {out}")
    print(f"Name:         {info.name}")
    print(f"Version:      {info.version}")
    print(f"ABI/API:      {ABI_MAJOR}/{info.api_min}")
    print(f"Link VMA:     0x{info.link_vma:08X}")
    print(f"Entry offset: 0x{info.entry_offset:04X}")
    print(f"Image:        {info.image_size} bytes")
    print(f"BSS:          {info.bss_size} bytes")
    print(f"RAM total:    {info.image_size + info.bss_size}/{args.ram_limit} bytes")
    print(f"File:         {len(image)}/{args.slot_size} bytes")
    print(f"CRC32:        0x{info.image_crc32:08X}")
    return 0


def cmd_info(args: argparse.Namespace) -> int:
    data = Path(args.image).read_bytes()
    info = parse_image(data, ram_limit=args.ram_limit, slot_size=args.slot_size)
    print(f"Valid K6A1 image: {args.image}")
    print(f"Name:         {info.name}")
    print(f"Version:      {info.version}")
    print(f"ABI/API:      {ABI_MAJOR}/{info.api_min}")
    print(f"Link VMA:     0x{info.link_vma:08X}")
    print(f"Entry offset: 0x{info.entry_offset:04X}")
    print(f"Image:        {info.image_size} bytes")
    print(f"BSS:          {info.bss_size} bytes")
    print(f"RAM total:    {info.image_size + info.bss_size}/{args.ram_limit} bytes")
    print(f"File:         {len(data)}/{args.slot_size} bytes")
    print(f"CRC32:        0x{info.image_crc32:08X}")
    print(f"Capabilities: 0x{info.required_caps:08X}")
    return 0


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    pack = sub.add_parser("pack", help="wrap a linked raw application image")
    pack.add_argument("--bin", required=True, help="raw linked ARM image (.text/.rodata/.data)")
    pack.add_argument("--out", required=True, help="output .k6app file")
    pack.add_argument("--name", required=True)
    pack.add_argument("--version", required=True)
    pack.add_argument("--api-min", type=int, default=API_LEVEL)
    pack.add_argument("--entry-offset", type=_parse_int, default=0)
    pack.add_argument("--link-vma", type=_parse_int, default=DEFAULT_LINK_VMA)
    pack.add_argument("--bss-size", type=_parse_int, default=0)
    pack.add_argument("--required-caps", type=_parse_int, default=0)
    pack.add_argument("--ram-limit", type=_parse_int, default=DEFAULT_RAM_LIMIT)
    pack.add_argument("--slot-size", type=_parse_int, default=DEFAULT_SLOT_SIZE)
    pack.set_defaults(func=cmd_pack)

    info = sub.add_parser("info", help="validate and display a .k6app image")
    info.add_argument("image")
    info.add_argument("--ram-limit", type=_parse_int, default=DEFAULT_RAM_LIMIT)
    info.add_argument("--slot-size", type=_parse_int, default=DEFAULT_SLOT_SIZE)
    info.set_defaults(func=cmd_info)
    return parser


def main() -> int:
    args = make_parser().parse_args()
    try:
        return args.func(args)
    except (OSError, ImageError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
