# Product Vision

| Field | Value |
|---|---|
| Purpose | Defines what Ember Clash Rebirth is, for whom, and what a successful first release means. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 02, 25 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. What it is
An original side-view competitive platform fighter: percentage damage, knockback, stocks, ring-outs, per-fighter specials, local multiplayer first, online later. Setting: a world of forges, ash and elemental clashes. All fighters, names, art, music and stages are original; other platform fighters are genre references only.

## 2. Target players
1. **Couch players** (2-4 local, mixed skill): need instant readability, join-in menus, items-free fair matches.
2. **Competitive hobbyists**: need frame-consistent mechanics, fixed stages, training tools.
3. **Solo players**: need good AI at 4 levels, Arcade/Survival, training.

## 3. Pillars converted into design goals
| Pillar | Measurable goal |
|---|---|
| Responsive | First state change 1 tick after sampled input (04 section 2); no input drops during state transitions (tests) |
| Readable | Each fighter identifiable by black silhouette at 64 px height; 4-player match legible at 1280x720 (playtest checklist, 08) |
| Deep, learnable | Every move has counterplay noted in its fighter spec; new player can complete a match in under 2 minutes of onboarding (13) |
| Fair | Ranked-style rules: fixed stages, no hazards, no random items (07) |
| Identity | Every fighter has one unique C++ behavior hook or mechanic (05) |

## 4. What the game deliberately is not
Not an items party game; not a combo-heavy traditional fighter; no pay-to-win; no copying of characters, moves, stage layouts or UI of other games.

## 5. Accessibility goals
Full rebinding, colour-blind palettes, reduced shake, text scaling, one-handed-capable alternate profile (14).

## 6. Horizons
- **Short (to vertical slice):** 2 real-art fighters, 1 arena, local 2P, AI.
- **Medium (first release):** 4 fighters, 3 arenas, local up to 4, modes in 25.
- **Long:** 8 fighters, 6 arenas, online unranked then ranked, cosmetics.

## 7. Definition of a successful first release
Stranger-playable 4-player local game with authored art, audio and menus; 60 fps on reference hardware [SC-13]; no known crash/soft-lock; 4 fighters balanced within a 40-60% AI-vs-AI band and passing human playtests; Windows, Linux and web builds produced by CI.
