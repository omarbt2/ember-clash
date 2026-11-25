# Fighter Spec: Vesper Quill

| Field | Value |
|---|---|
| Purpose | Design specification for Vesper Quill (Speed). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Vesper Quill** - Speed fighter. A courier of the ash roads who learned to fight to stay ahead of debt collectors. Fast talker, faster feet.
- Combat fantasy: Hit and vanish: overwhelm with speed and many small threats.
- Silhouette: Slim body with spiked quill fan - spiky halo around the arms.
- Palette: teal #2FB8A8, cream #F2E6C9, rust #B5522B
- Signature weapon/tool: Fan of metal quills
- Preferred range: Close-mid
- Strategy: Air dash in, multi-hit pokes, escape before punish.
- Weaknesses: Light, kill power low, short reach.
- Counterplay: Trap landing, shield pressure, edge-guard.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 80. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Air dash with 4 ticks of invulnerability (blink); dash count resets on landing (state in snapshot).

## 4. Specials
- **Neutral - Quill Flurry:** Three quills at fixed angles; short range; multi-hit.
- **Side - Gale Step:** Air dash, 4 invuln ticks, choose direction.
- **Up - Updraft Spiral:** Multi-hit vertical recovery with sideways drift.
- **Down - Pin Drop:** Fast dive; bounces on hit; punishable on whiff.
- **Ember Surge:** Quicksilver: 2 extra air dashes and 10% faster dashes.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 0.8 and power 0.8; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 3 | 3 | 7 | 2 | 16 |
| FTilt | ground | 6 | 4 | 13 | 7 | 22 |
| UTilt | ground | 6 | 5 | 14 | 6 | 24 |
| DTilt | ground | 5 | 4 | 12 | 6 | 19 |
| FSmash | smash | 11 | 4 | 26 | 14 | 36 |
| USmash | smash | 10 | 5 | 27 | 13 | 35 |
| DSmash | smash | 8 | 6 | 29 | 12 | 32 |
| NAir | aerial | 4 | 10 | 10 | 6 | 18 |
| FAir | aerial | 7 | 5 | 14 | 9 | 24 |
| BAir | aerial | 6 | 4 | 13 | 10 | 26 |
| UAir | aerial | 6 | 5 | 12 | 7 | 22 |
| DAir | aerial | 10 | 4 | 16 | 10 | 27 |
| Grab | grab | 5 | 3 | 19 | 0 | 0 |
| SpecialN: Quill Flurry | special | per move | - | - | per move | - |
| SpecialS: Gale Step | special | per move | - | - | per move | - |
| SpecialU: Updraft Spiral | special | per move | - | - | per move | - |
| SpecialD: Pin Drop | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Lean forward, hair/feather streamers, quick turns. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Teal streaks and afterimages. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Fast metallic whooshes, light tinkles. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Air dash with 4 ticks of invulnerability (blink).
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
