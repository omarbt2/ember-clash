import json
import struct
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from validate_art_assets import ValidationError, validate_manifest


def write_png_header(path: Path, width: int = 128, height: int = 96, color_type: int = 6) -> None:
    header = (b"\x89PNG\r\n\x1a\n" + struct.pack(">I", 13) + b"IHDR"
              + struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0))
    path.write_bytes(header)


class ArtManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / "assets").mkdir()
        (self.root / "tools").mkdir()
        self.texture = self.root / "assets" / "fighter.png"
        write_png_header(self.texture)
        self.manifest = self.root / "assets" / "manifest.json"
        self.data = {
            "version": 1,
            "atlases": [{
                "id": "kaelen",
                "texture": "assets/fighter.png",
                "license": {"creator": "Studio", "license": "CC-BY-4.0", "source": "original"},
                "clips": {"idle": {"fps": 10, "loop": True,
                    "frames": [{"x": 0, "y": 0, "w": 64, "h": 64}]}}
            }]
        }

    def tearDown(self) -> None:
        self.temp.cleanup()

    def save(self) -> None:
        self.manifest.write_text(json.dumps(self.data), encoding="utf-8")

    def test_valid_manifest(self) -> None:
        self.save()
        self.assertIn("1 clips, 1 frames", validate_manifest(self.manifest)[0])

    def test_rejects_frame_outside_texture(self) -> None:
        self.data["atlases"][0]["clips"]["idle"]["frames"][0]["x"] = 100
        self.save()
        with self.assertRaisesRegex(ValidationError, "exceeds"):
            validate_manifest(self.manifest)

    def test_requires_alpha_channel(self) -> None:
        write_png_header(self.texture, color_type=2)
        self.save()
        with self.assertRaisesRegex(ValidationError, "must include alpha"):
            validate_manifest(self.manifest)

    def test_rejects_path_traversal(self) -> None:
        self.data["atlases"][0]["texture"] = "../outside.png"
        self.save()
        with self.assertRaisesRegex(ValidationError, "escapes project root"):
            validate_manifest(self.manifest)

    def test_requires_license_metadata(self) -> None:
        del self.data["atlases"][0]["license"]["creator"]
        self.save()
        with self.assertRaisesRegex(ValidationError, "license.creator"):
            validate_manifest(self.manifest)

    def test_rejects_boolean_schema_version(self) -> None:
        self.data["version"] = True
        self.save()
        with self.assertRaisesRegex(ValidationError, "version"):
            validate_manifest(self.manifest)

    def test_rejects_duplicate_ids(self) -> None:
        self.data["atlases"].append(dict(self.data["atlases"][0]))
        self.save()
        with self.assertRaisesRegex(ValidationError, "duplicate atlas id"):
            validate_manifest(self.manifest)


if __name__ == "__main__":
    unittest.main()
