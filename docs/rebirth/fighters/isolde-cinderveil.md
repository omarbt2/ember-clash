# Fighter Spec: Isolde Cinderveil

| Field | Value |
|---|---|
| Purpose | Design specification for Isolde Cinderveil (Technical). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Isolde Cinderveil** - Technical fighter. A ritualist who binds ash into fate. Calm, precise, always three moves ahead.
- Combat fantasy: The puzzle-fighter: set up, switch stance, punish mistakes.
- Silhouette: Tall with trailing veil and swinging chain - asymmetric vertical line.
- Palette: violet #5B3A8C, ash white #E9E4DC, ember #E8602C
- Signature weapon/tool: Chain and censer
- Preferred range: Mid-long
- Strategy: Two stances (Cinder: aggressive, Veil: control) change specials; place markers and detonate for combos.
- Weaknesses: Low damage without setup; stance switch is slow; fragile.
- Counterplay: Rush down, punish stance switch.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 95. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Stance state and marker position stored in snapshot; stance changes tint and move properties.

## 4. Specials
- **Neutral - Stance Swap:** Toggle Cinder/Veil; 20-tick startup; leaves ash cloud.
- **Side - Ash Chain:** Tethered projectile; Cinder: pulls toward; Veil: pulls opponent.
- **Up - Cinder Lift:** Recovery teleport to previously placed marker if any, else short lift.
- **Down - Mark Detonate:** Place a mark; re-press detonates after arming; max 1 mark.
- **Ember Surge:** Ritual Burn: marks detonate instantly and persist 2 detonations.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.0 and power 0.95; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 4 | 3 | 9 | 3 | 19 |
| FTilt | ground | 8 | 4 | 16 | 9 | 27 |
| UTilt | ground | 7 | 5 | 17 | 8 | 28 |
| DTilt | ground | 6 | 4 | 15 | 7 | 23 |
| FSmash | smash | 14 | 4 | 32 | 17 | 43 |
| USmash | smash | 12 | 5 | 34 | 15 | 42 |
| DSmash | smash | 10 | 6 | 36 | 14 | 38 |
| NAir | aerial | 5 | 10 | 12 | 8 | 21 |
| FAir | aerial | 9 | 5 | 18 | 10 | 28 |
| BAir | aerial | 8 | 4 | 16 | 11 | 30 |
| UAir | aerial | 7 | 5 | 15 | 9 | 27 |
| DAir | aerial | 12 | 4 | 20 | 12 | 32 |
| Grab | grab | 6 | 3 | 24 | 0 | 0 |
| SpecialN: Stance Swap | special | per move | - | - | per move | - |
| SpecialS: Ash Chain | special | per move | - | - | per move | - |
| SpecialU: Cinder Lift | special | per move | - | - | per move | - |
| SpecialD: Mark Detonate | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Deliberate, flowing, ritual gestures. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Violet ash, glyph rings. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Chime, chain rattle, whispered air. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Stance state and marker position stored in snapshot.
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
