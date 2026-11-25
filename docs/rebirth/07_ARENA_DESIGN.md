# Arena Design

| Field | Value |
|---|---|
| Purpose | Six original arenas, ranked/casual separation, data requirements. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 19, 20, 08 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline (VERIFIED): one default layout - full-width floor at y = H-80, two side platforms 240x24 at floor-160, kill plane at y = H+200. 4 blast zones, camera bounds, spawns beyond 2 are MISSING.

## 1. Common rules
Logical space 1280x720 [SC-8] extended per stage with camera bounds. Each arena data file (19): platforms, pass-through flags, blast zones (L/R/T/B), spawn points (4), respawn platform position, camera bounds, hazard list (casual only), background layers, music/ambience IDs.
Ranked/competitive: Arenas 1-3 only, hazards off, no moving geometry.

## 2. Arenas
| # | Arena | Theme | Layout | Hazards (casual) | Competitive | Art needs |
|---|---|---|---|---|---|---|
| 1 | Ashen Forge | Giant foundry, glowing slag below, anvil silhouettes | wide main stage + 2 side platforms (like VERIFIED default, but finite floor with ledges) | none | Yes - standard | 5 parallax layers, ember particles |
| 2 | Cinder Bridge | Stone bridge across lava canyon | narrow main + 1 center pass-through | falling embers (casual, telegraphed) | Yes | 5 layers, lava glow |
| 3 | Tidal Foundry | Flooded smithy, steam vents | 3 tiered platforms | steam burst (casual) | Yes | water shader-free animated tiles |
| 4 | Sky Kiln | Floating kiln platforms | 2 floating islands, drifting wind | wind gusts | Casual only | sky layers |
| 5 | Glass Dunes | Desert of fused glass | sloped-look flat platforms, 2 low | sandstorm visibility (reduced) | Casual only | haze overlay |
| 6 | Magma Vault | Inside a volcano | 4 platforms stacked | rising lava line | Casual only | heavy VFX |

## 3. Comparison matrix
| Arena | Main width (px) | Platforms | Edge type | Blast margin | Perf cost | Ranked |
|---|---|---|---|---|---|---|
| Ashen Forge | 900 | 2 | ledges | L/R 280, T 240, B 220 | low | yes |
| Cinder Bridge | 720 | 1 | ledges | L/R 300, T 260, B 240 | low | yes |
| Tidal Foundry | 760 | 3 | ledges | L/R 280, T 240, B 220 | medium | yes |
| Sky Kiln | 2 x 420 | 1 | ledges | 320/280/260 | medium | no |
| Glass Dunes | 1000 | 2 | ledges | 280/240/220 | medium | no |
| Magma Vault | 800 | 4 | ledges | 300/320/240 | high | no |
Numbers are initial targets.

## 4. Performance limits
<= 6 background layers, <= 150 ambient particles, background textures <= 24 MB VRAM per arena (20).

## 5. Tests
Collision fixtures per arena, KO from each blast zone, spawn separation >= 160 px, camera bounds contain all spawn points, ledge reachability, hazard-off equals ranked set.
