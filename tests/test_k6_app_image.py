import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from k6_app_image import (  # noqa: E402
    DEFAULT_LINK_VMA,
    HEADER_SIZE,
    ImageError,
    build_image,
    parse_image,
)


class K6AppImageTests(unittest.TestCase):
    def setUp(self):
        self.payload = bytes(range(128))

    def test_round_trip(self):
        blob = build_image(
            self.payload,
            name="LBJRX",
            version="0.1",
            bss_size=512,
        )
        app = parse_image(blob)
        self.assertEqual(app.name, "LBJRX")
        self.assertEqual(app.version, "0.1")
        self.assertEqual(app.payload, self.payload)
        self.assertEqual(app.link_vma, DEFAULT_LINK_VMA)
        self.assertEqual(app.image_size, len(self.payload))
        self.assertEqual(app.bss_size, 512)

    def test_rejects_f4hwn_fap1(self):
        with self.assertRaises(ImageError):
            parse_image(b"FAP1" + bytes(HEADER_SIZE - 4) + self.payload)

    def test_rejects_crc_mismatch(self):
        blob = bytearray(build_image(
            self.payload,
            name="LBJRX",
            version="0.1",
        ))
        blob[-1] ^= 0x01
        with self.assertRaisesRegex(ImageError, "CRC32 mismatch"):
            parse_image(bytes(blob))

    def test_rejects_ram_overflow(self):
        with self.assertRaisesRegex(ImageError, "RAM limit"):
            build_image(
                self.payload,
                name="LBJRX",
                version="0.1",
                bss_size=0x1800,
            )

    def test_rejects_slot_overflow(self):
        with self.assertRaisesRegex(ImageError, "slot limit"):
            build_image(
                bytes(0x2000),
                name="LBJRX",
                version="0.1",
            )


    def test_assets_round_trip(self):
        assets = b"loco-map\\x00K6 resource"
        blob = build_image(
            self.payload,
            name="LBJRX",
            version="0.1",
            assets=assets,
            bss_size=256,
        )
        app = parse_image(blob)
        self.assertEqual(app.assets, assets)
        self.assertEqual(app.asset_size, len(assets))
        self.assertNotEqual(app.asset_crc32, 0)

    def test_rejects_asset_crc_mismatch(self):
        blob = bytearray(build_image(
            self.payload,
            name="LBJRX",
            version="0.1",
            assets=b"asset-data",
        ))
        blob[60] ^= 0x01
        with self.assertRaisesRegex(ImageError, "asset CRC32 mismatch"):
            parse_image(bytes(blob))

    def test_slot_limit_includes_assets(self):
        with self.assertRaisesRegex(ImageError, "slot limit"):
            build_image(
                bytes(7000),
                name="LBJRX",
                version="0.1",
                assets=bytes(1200),
                ram_limit=8000,
            )

    def test_rejects_wrong_link_vma(self):
        with self.assertRaisesRegex(ImageError, "link_vma"):
            build_image(
                self.payload,
                name="LBJRX",
                version="0.1",
                link_vma=0x20001000,
            )

    def test_rejects_bad_entry_offset(self):
        with self.assertRaisesRegex(ImageError, "entry_offset"):
            build_image(
                self.payload,
                name="LBJRX",
                version="0.1",
                entry_offset=len(self.payload),
            )


if __name__ == "__main__":
    unittest.main()
