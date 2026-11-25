# Art and Asset Pipeline

| Field | Value |
|---|---|
| Purpose | Compares production methods, defines the decision framework, a one-fighter spike, formats and validation. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 08, 10, 28 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Tool inventory (checked in this environment)
Image generation: none available. Blender: not installed. 2D rigging tools: none. Available: Python 3 + Pillow 13, ffmpeg, SDL3 toolchain. Nothing visual has been generated; no authored art exists in the repo (MISSING). The owner can make progress without these tools by: supplying concept art; running the spike locally in Blender with the scripts defined below; or authorizing a generator/connector in a future session.

## 2. Methods compared
| Criterion | A. Blender 3D -> baked sprites | B. Layered 2D rig -> baked | C. Hand-drawn frame-by-frame |
|---|---|---|---|
| Skill needed | modeling, rigging, animation, Blender Python | 2D art, rigging (Spine/DragonBones/Blender 2D) | strong animation drawing |
| Time per extra fighter | lowest after rig (reuse) | medium | highest (47 clips x ~10 frames) |
| Visual quality | consistent lighting, "3D look" needs outline/ink shaders to feel painted | hand-drawn look; risk of rig stiffness | best potential |
| Consistency across roster | high | medium | depends on artist |
| Reusability of animation | high (retarget shared rig) | medium | none |
| Animation flexibility/iteration | high (edit pose, rebake) | high | low (redraw) |
| Technical integration | atlas + JSON via script | runtime or bake | atlas + JSON |
| Hardware | mid PC, CPU/GPU for baking (minutes/clip) | low | low |
| Licensing | Blender GPL output unrestricted; asset sources must be own | tool license (Spine paid) | none |
| Long-term maintenance | source .blend + scripts | rig files | PSD/Krita layers |
| Main risk | style may look generic/3D | AI-generated art is hard to cut into parts | cost |
**Decision framework:** score each method 1-5 on: visual target match, cost per fighter, consistency, owner skills, iteration speed; weights 30/25/15/15/15. Method choice is OPEN (D-004) until the spike below concludes; Blender is the working hypothesis, not a decision.

## 3. One-fighter spike (must complete before committing roster production)
Subject: Kaelen Ashforge. Deliver, per method tried (A and B minimum):
1. Concept sheet (front/side/back) and palette.
2. Rigged character.
3. Clips: Idle, Walk, Run, Jump, Jab, FTilt, FSmash, Hit-light, KO (9 clips).
4. Baked atlas `kaelen.png` + `anim.json` loaded by a prototype `SpriteSheet` loader (code work happens in P5, not now).
5. In-game overlay of hurtbox/hitbox alignment on all 9 clips.
6. Time log (hours per clip), file sizes, and a 5-person readability test.
Pass criteria: <= 3 h average per attack clip after rig; atlas <= 4096x4096; hitbox alignment error <= 2 px; fighter readable at 64 px; owner approves style. Spike produces a go/no-go recorded in D-004.

## 4. Formats and layout
```
art_src/<fighter>/        concept/, model/ or rig/, anim/ (Git LFS: .blend, .psd, .kra, .png > 1 MB)
assets/fighters/<id>/     atlas_0.png, anim.json, palette.json
assets/arenas/<id>/       layers/*.png, arena.json
assets/audio/ , assets/ui/ , assets/LICENSES.md
tools/art/                bake.py (Blender), pack_atlas.py (Pillow), validate_assets.py
```
Runtime: PNG RGBA premultiplied alpha, cell 256x256 default (fighter), anchor at feet-center, facing right (mirrored at runtime). `anim.json`: clip -> frames[{rect, duration_ticks, anchor, events}].
Mirroring: asymmetric designs (Rook weapon hand, Kaelen cape) must be mirror-safe or provide `mirror_safe:false` with a baked left-facing set (doubles those clips); decision per fighter at concept stage.
Frame timing: sprite frame durations are in ticks and the sum for attack clips equals startup+active+recovery from move data (validator enforces).
Hitbox alignment: hitboxes are authored in move data, previewed over sprites by a debug overlay; baking script exports bone positions per frame for reference.

## 5. Validation script requirements (`validate_assets.py`)
atlas size and VRAM estimate, no empty cells, anchor within cell, every required clip present (47), clip durations match move data, license entry exists for each file, PNG premultiplied check.

## 6. Git LFS
Track `art_src/**` large binaries and `assets/**/*.png` > 1 MB. CI fetches LFS only for release builds. Fallback: external storage with hash manifest if LFS bandwidth is an issue.

## 7. Licensing
Every asset has a record in 28; AI-generated concepts only if the generator's terms allow commercial use and are logged with prompts and tool/version; no third-party IP.
