# Art asset pipeline (P5 foundation)

The game currently uses its procedural sprite fallback. Real fighter atlases
are **not included** in this repository yet. When original/licensed art is
ready, add `assets/manifest.json` using schema version 1 and run:

```sh
python3 tools/validate_art_assets.py assets/manifest.json
```

Each atlas needs a unique `id`, a project-relative PNG `texture`, explicit
`license.creator`, `license.license`, and `license.source` fields, plus at
least one named clip. Each clip declares `fps`, boolean `loop`, and a non-empty
`frames` array of `{x,y,w,h}` pixel rectangles. Textures must be PNG files
with an alpha channel. Paths that escape the project root, missing metadata,
invalid frame bounds, and duplicate atlas IDs fail validation.

This check validates file structure and metadata only. It does not prove that
art is original, that the declared license is valid, that animation is
visually coherent, or that hitboxes align to the rendered character. Those
still require review and evidence before P5 can pass its gate.
