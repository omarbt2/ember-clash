> **SUPERSEDED** by the production bible in `docs/rebirth/` (see `docs/rebirth/README.md`). Kept for history; some values here were corrected in `docs/rebirth/00_PLAN_REVIEW_CORRECTIONS.md`.

# EMBER CLASH REBIRTH — Art and Asset Pipeline

## 1. Tool inventory (honest)
Checked in the planning environment: **no image-generation tool, no Blender, no 2D rigging tool, no audio device, no display.** Available: Python 3 + Pillow, ffmpeg, SDL3 build chain. Nothing was generated in this phase and no art exists in the repo. Do not request API keys in chat: if you have an authorized image service or Blender locally, connect it as an MCP/tool or run the specs below yourself and hand me the output files.

## 2. Art direction
Premium stylized 2D/2.5D fantasy. Warm ember palette (ash gray, forge orange, deep indigo shadows) with one strong signature color per fighter for readability. Thick consistent outline, 3-tone cel shading, rim light matching the arena. No copied IP.

## 3. Decision: production method
| Option | Pros | Cons |
|---|---|---|
| **A. Blender low-poly 3D → baked 2D sprite sheets (recommended)** | Shared humanoid rig and retargetable animation across 8 fighters; consistent lighting; mirroring free; hitbox/bone data exported; cheapest per additional fighter | Needs 3D modeling skill; stylized look must be tuned |
| B. 2D layered rig (Spine/DragonBones-style) baked to sheets | Hand-drawn feel; AI concept art can be cut into parts | Painful consistency; each fighter animated from scratch; AI art rarely splits cleanly |
| C. Hand-drawn frame-by-frame | Best look | Cost scales with frames (~200 animations) |
Choice: **A**, with a 1-fighter spike in P5 on both A and B before committing the other 7. Runtime stays simple: SDL3 renders atlas sprites.

## 4. Animation list per fighter (target frame budgets at 60 Hz sim; sprites may hold frames)
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, Jab, F/U/D Tilt, F/U/D Smash (charge+release), N/F/B/U/D Air, 4 Specials, Shield, Parry, Dodge, Hit-light, Hit-heavy, Knockback, KO, Respawn, Victory, Defeat. (~38 clips/fighter before variants.) Attack animations must match the data-table frame counts exactly: startup/active/recovery.

## 5. Export spec
- Transparent PNG atlases, 256x256 cell (fighter 48x64 collision box centered at feet-anchor), premultiplied alpha, facing right (mirror at runtime).
- Per fighter: `atlas.png` + `anim.json` (clip → frames, durations, anchor, optional hitbox overlay).
- Budget: ≤ 4096x4096 atlas per fighter, ≤ 16 MB VRAM per fighter.
- Source files (.blend, concepts) in `art_src/` via Git LFS; runtime assets in `assets/`.

## 6. Concept stage (when a generator exists)
Per fighter: front/side/back sheet, palette, 3 poses, silhouette test (must be distinguishable in 100% black at 64 px height). Review gate before modeling.

## 7. VFX
Spritesheet particles + additive blend: light hit, heavy hit, special per move, projectile trail, shield impact, parry flash, dash trail, KO burst, ambient embers. Budget ≤ 200 live particles, no fullscreen overdraw beyond KO.

## 8. Arena art
5 parallax layers + foreground accent; collision geometry is data, never inferred from art. Tools: Pillow/ffmpeg scripts for atlas packing, trim, and validation (`tools/`).

## 9. Audio assets
OGG, 44.1 kHz; SFX ≤ 1 s mono; music stems looped with markers. Original only; license log in `assets/LICENSES.md`.

## 10. Validation
Automated: atlas size, anchor sanity, frame count matches move table. Manual: in-game hitbox overlay toggle to check alignment.
