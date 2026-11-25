# Art Direction

| Field | Value |
|---|---|
| Purpose | Visual language guide for characters, arenas, VFX and UI. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 09, 10, 13 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Visual references (written descriptions only; no images created or collected): the look of a hand-forged poster - heavy ink outlines, warm light from below, cold shadows; layered painted backgrounds with silhouetted architecture; characters read as strong geometric shapes.

## 1. Shape language
Fighters: one dominant geometric motif each (Kaelen diagonal, Torga block, Vesper spikes, Isolde vertical flow, Rook horizontal barrel, Maren vertical trident, Zephyr triangle, Bastian circle). Arenas: large horizontal masses and diagonal supports.

## 2. Proportions
Heads 1/5.5 of height; fighter collision box 48x64 baseline (VERIFIED); visual sprite taller than hurtbox by <= 24 px for weapons/hair. Hurtbox always drawn from data, not from the sprite.

## 3. Silhouette rule
Each fighter is distinct at 64 px height in solid black; test by a contact sheet of the roster in black.

## 4. Colour system
Roster palettes in `fighters/*.md`. Rules: one dominant hue + one accent per fighter; no fighter shares the dominant hue with the arena foreground; team tints are an outline overlay, never a fill. Ember orange #E8602C is a shared brand accent, reserved for impacts/UI highlights outside Kaelen/Isolde accents.

## 5. Lighting, shading, outline
Rim light matches arena key (warm from below on forge arenas). 3 tones + highlight. Outline 2 px at 1080p, darker tint of local colour, thicker (3 px) on fighters than on background.

## 6. Materials and weapons
Metal: hard highlight band; stone: broken-edge shading; cloth: soft two-tone; fire: additive layers. Weapons must be readable as a single shape at 64 px.

## 7. Readability
Background luminance 20-40% darker/lower contrast than fighters; hazard cues (casual) use a unique colour (magenta-white flash) not used elsewhere; hit effects do not cover the face/torso for more than 6 ticks.

## 8. UI visual language
Dark charcoal panels, ember gradient edges, bold condensed typeface (open-licensed, e.g. SIL OFL), 8 px grid, consistent iconography; no stock gradient buttons.
