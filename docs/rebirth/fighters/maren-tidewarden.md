# Fighter Spec: Maren Tidewarden

| Field | Value |
|---|---|
| Purpose | Design specification for Maren Tidewarden (Trapper). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Maren Tidewarden** - Trapper fighter. Keeper of the flooded foundry vents. Patient and cunning.
- Combat fantasy: Shape the stage and punish movement through it.
- Silhouette: Long vertical trident, bulky shoulders, coat with fin shapes.
- Palette: deep blue #1F4E79, seafoam #7FD1C7, rust #B5522B
- Signature weapon/tool: Trident-anchor
- Preferred range: Mid
- Strategy: Place pools and mines, herd opponents into traps.
- Weaknesses: Setup time, vulnerable while placing, traps are limited.
- Counterplay: Destroy traps with attacks, rush the placer.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 105. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Trap entities with owner, arming ticks, max 2 pools and 3 mines; snapshot holds all traps.

## 4. Specials
- **Neutral - Tide Pool:** Place a pool that slows movement by 25% for 6 s; max 2.
- **Side - Undertow Net:** Slow projectile that snares for 40 ticks.
- **Up - Geyser Rise:** Recovery; leaves a geyser that launches.
- **Down - Brine Mine:** Mine arms in 30 ticks; max 3; detonates on contact.
- **Ember Surge:** Spring Tide: traps cannot be destroyed, limits +1.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.1 and power 1.0; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 4 | 3 | 10 | 3 | 20 |
| FTilt | ground | 9 | 4 | 18 | 9 | 28 |
| UTilt | ground | 8 | 5 | 19 | 8 | 30 |
| DTilt | ground | 7 | 4 | 16 | 7 | 24 |
| FSmash | smash | 15 | 4 | 35 | 18 | 45 |
| USmash | smash | 13 | 5 | 37 | 16 | 44 |
| DSmash | smash | 11 | 6 | 40 | 15 | 40 |
| NAir | aerial | 6 | 10 | 13 | 8 | 22 |
| FAir | aerial | 10 | 5 | 20 | 11 | 30 |
| BAir | aerial | 9 | 4 | 18 | 12 | 32 |
| UAir | aerial | 8 | 5 | 16 | 9 | 28 |
| DAir | aerial | 13 | 4 | 22 | 13 | 34 |
| Grab | grab | 7 | 3 | 26 | 0 | 0 |
| SpecialN: Tide Pool | special | per move | - | - | per move | - |
| SpecialS: Undertow Net | special | per move | - | - | per move | - |
| SpecialU: Geyser Rise | special | per move | - | - | per move | - |
| SpecialD: Brine Mine | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Methodical, planting stance. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Blue water arcs, steam. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Water bursts, metal dings. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Trap entities with owner, arming ticks, max 2 pools and 3 mines.
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
