# Fighter Roster

| Field | Value |
|---|---|
| Purpose | Roster overview and shared fighter framework. Per-fighter specs are in `fighters/`. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | fighters/, 03, 12 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Names/concepts are provisional inputs from the preliminary plan, kept unless noted. No changes were made to names; two design changes: Torga and Bastian get distinct roles (offensive armor vs reflect/counter) to avoid overlap; Vesper's down special changed from "Feather Trap" to a dive so trapping belongs to Maren only.

## 1. Overview
| # | Fighter | Archetype | Weight | Hook (unique) | Skill floor/ceiling | Build phase | First release? |
|---|---|---|---|---|---|---|---|
| 1 | Kaelen Ashforge | Balanced | 100 | Forge Heat stacks upgrade specials | low / medium | P4 | Yes |
| 2 | Torga Stonehide | Heavy | 135 | Armor thresholds and Quake shockwaves | low / medium | P4 | Yes |
| 3 | Vesper Quill | Speed | 80 | Air-dash and multi-hit quills | medium / high | P10 | Yes (wave 2) |
| 4 | Isolde Cinderveil | Technical | 95 | Stance switch Cinder/Veil, markers | high / high | P10 | Yes (wave 2) |
| 5 | Rook Emberwick | Ranged | 90 | Projectiles, turret | medium / high | P10 | Post-launch |
| 6 | Maren Tidewarden | Trapper | 105 | Placed traps with ownership limits | high / high | P10 | Post-launch |
| 7 | Zephyr Kite | Aerial | 85 | Glide and aerial control | medium / high | P10 | Post-launch |
| 8 | Bastian Wardenhold | Defensive | 115 | Reflect and counter | medium / high | P10 | Post-launch |

First release roster = fighters 1-4 (SC-3). Fighters 5-8 follow in waves (see 25).

## 2. Framework
**Engine-level move categories (shared):** jab, ftilt/utilt/dtilt, fsmash/usmash/dsmash, nair/fair/bair/uair/dair, grab+pummel+4 throws, special neutral/side/up/down, Surge. 16 ids exist (VERIFIED); grabs/throws added in P3.
**Unique fighter data:** frame data, hitboxes, damage, weights, animations, specials.
**C++ behavior hooks:** `forge_heat`, `armor_frames`, `air_dash_blink`, `stance_switch`, `projectile_spawn`, `trap_place`, `glide`, `counter_reflect`. One hook set per fighter, each documented with its snapshot state (18).

## 3. Common stats (initial targets)
| Fighter | Walk | Run | Air speed | Fall | Jumps | Notes |
|---|---|---|---|---|---|---|
| Kaelen | 6.0 | 8.5 | 5.0 | 12 | 2 | baseline VERIFIED walk 6.0 |
| Torga | 4.8 | 6.5 | 3.8 | 15 | 2 (weak 2nd) | |
| Vesper | 7.4 | 10.5 | 6.2 | 10 | 2 + air dash | |
| Isolde | 6.0 | 8.0 | 5.2 | 11.5 | 2 | |
| Rook | 5.6 | 7.8 | 4.8 | 12 | 2 | |
| Maren | 5.4 | 7.5 | 4.6 | 13 | 2 | |
| Zephyr | 6.6 | 9.0 | 6.0 | 9 (glide) | 3 | |
| Bastian | 5.0 | 7.0 | 4.0 | 14 | 2 | |

## 4. Balance system
Move data in JSON; `tools/matchup_sim` round-robins AI-vs-AI (HARD) 200 matches/pair/side-swapped; target 40-60% per pair, flagged outside 35-65%; plus human playtests; balance patch notes versioned (29). Counter-matrix intentionally leaves some 55-60% matchups.

## 5. Differentiation checks
Each fighter must differ in at least 4 of: speed, weight, range, special mechanic, defensive tool, recovery type. Verified by spec table, not by feel until playtests.
