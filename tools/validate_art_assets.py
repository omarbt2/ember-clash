#!/usr/bin/env python3
"""Validate Ember Clash sprite-atlas manifests without third-party packages.

Manifest v1:
{
  "version": 1,
  "atlases": [{
    "id": "kaelen",
    "texture": "assets/fighters/kaelen.png",
    "license": {"creator": "...", "license": "...", "source": "..."},
    "clips": {"idle": {"fps": 10, "loop": true,
      "frames": [{"x": 0, "y": 0, "w": 64, "h": 64}]}}
  }]
}

The validator checks PNG dimensions/alpha, frame bounds and metadata. It does
not claim that frames are visually correct or that a license is legally valid.
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path
from typing import Any

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


class ValidationError(ValueError):
    pass


def _object(value: Any, context: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValidationError(f"{context}: expected an object")
    return value


def _nonempty_string(value: Any, context: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValidationError(f"{context}: expected a non-empty string")
    return value.strip()


def read_png_header(path: Path) -> tuple[int, int, int, int]:
    try:
        with path.open("rb") as file:
            header = file.read(29)
    except OSError as exc:
        raise ValidationError(f"cannot read texture {path}: {exc}") from exc
    if len(header) < 29 or header[:8] != PNG_SIGNATURE:
        raise ValidationError(f"{path}: missing or truncated PNG header")
    length = struct.unpack(">I", header[8:12])[0]
    chunk_type = header[12:16]
    if length < 13 or chunk_type != b"IHDR":
        raise ValidationError(f"{path}: malformed PNG IHDR")
    width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
        ">IIBBBBB", header[16:29]
    )
    if width <= 0 or height <= 0:
        raise ValidationError(f"{path}: invalid image dimensions")
    if color_type not in (4, 6):
        raise ValidationError(f"{path}: PNG must include alpha (color type 4 or 6)")
    if bit_depth not in (8, 16):
        raise ValidationError(f"{path}: alpha PNG bit depth must be 8 or 16")
    if compression != 0 or filtering != 0 or interlace not in (0, 1):
        raise ValidationError(f"{path}: unsupported PNG header settings")
    return width, height, bit_depth, color_type


def validate_manifest(manifest_path: Path) -> list[str]:
    try:
        raw = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ValidationError(f"cannot load manifest {manifest_path}: {exc}") from exc

    root = _object(raw, "manifest")
    version = root.get("version")
    if type(version) is not int or version != 1:
        raise ValidationError("manifest.version: expected integer 1")
    atlases = root.get("atlases")
    if not isinstance(atlases, list) or not atlases:
        raise ValidationError("manifest.atlases: expected a non-empty array")

    project_root = manifest_path.resolve().parent.parent
    seen_ids: set[str] = set()
    summaries: list[str] = []
    for atlas_index, item in enumerate(atlases):
        ctx = f"atlases[{atlas_index}]"
        atlas = _object(item, ctx)
        atlas_id = _nonempty_string(atlas.get("id"), f"{ctx}.id")
        if atlas_id in seen_ids:
            raise ValidationError(f"{ctx}.id: duplicate atlas id {atlas_id!r}")
        seen_ids.add(atlas_id)

        texture_value = _nonempty_string(atlas.get("texture"), f"{ctx}.texture")
        texture_rel = Path(texture_value)
        if texture_rel.is_absolute():
            raise ValidationError(f"{ctx}.texture: absolute paths are forbidden")
        texture = (project_root / texture_rel).resolve()
        try:
            texture.relative_to(project_root)
        except ValueError as exc:
            raise ValidationError(f"{ctx}.texture: path escapes project root") from exc
        width, height, _, _ = read_png_header(texture)

        license_data = _object(atlas.get("license"), f"{ctx}.license")
        for field in ("creator", "license", "source"):
            _nonempty_string(license_data.get(field), f"{ctx}.license.{field}")

        clips = _object(atlas.get("clips"), f"{ctx}.clips")
        if not clips:
            raise ValidationError(f"{ctx}.clips: expected at least one animation clip")
        frame_total = 0
        for clip_name, clip_value in clips.items():
            clip_ctx = f"{ctx}.clips.{clip_name}"
            _nonempty_string(clip_name, f"{ctx}.clips key")
            clip = _object(clip_value, clip_ctx)
            fps = clip.get("fps")
            if isinstance(fps, bool) or not isinstance(fps, (int, float)) or not (0 < fps <= 240):
                raise ValidationError(f"{clip_ctx}.fps: expected a number in (0, 240]")
            if not isinstance(clip.get("loop"), bool):
                raise ValidationError(f"{clip_ctx}.loop: expected true or false")
            frames = clip.get("frames")
            if not isinstance(frames, list) or not frames:
                raise ValidationError(f"{clip_ctx}.frames: expected a non-empty array")
            for frame_index, frame_value in enumerate(frames):
                frame_ctx = f"{clip_ctx}.frames[{frame_index}]"
                frame = _object(frame_value, frame_ctx)
                values: dict[str, int] = {}
                for key in ("x", "y", "w", "h"):
                    value = frame.get(key)
                    if isinstance(value, bool) or not isinstance(value, int):
                        raise ValidationError(f"{frame_ctx}.{key}: expected an integer")
                    values[key] = value
                if values["x"] < 0 or values["y"] < 0 or values["w"] <= 0 or values["h"] <= 0:
                    raise ValidationError(f"{frame_ctx}: coordinates must be non-negative and size positive")
                if values["x"] + values["w"] > width or values["y"] + values["h"] > height:
                    raise ValidationError(f"{frame_ctx}: frame rectangle exceeds {width}x{height} texture")
            frame_total += len(frames)
        summaries.append(f"{atlas_id}: {len(clips)} clips, {frame_total} frames, {width}x{height} alpha PNG")
    return summaries


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", nargs="?", default="assets/manifest.json", type=Path)
    args = parser.parse_args(argv)
    try:
        summaries = validate_manifest(args.manifest)
    except ValidationError as exc:
        print(f"ART ASSET VALIDATION FAILED: {exc}", file=sys.stderr)
        return 1
    for summary in summaries:
        print(f"PASS {summary}")
    print(f"ART ASSET VALIDATION PASSED: {len(summaries)} atlas(es)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
