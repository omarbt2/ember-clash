# Fighter Spec: Zephyr Kite

| Field | Value |
|---|---|
| Purpose | Design specification for Zephyr Kite (Aerial). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Zephyr Kite** - Aerial fighter. A kite racer from the Sky Kiln who treats gravity as a rule to bend.
- Combat fantasy: Own the air: fall slowly, hit from above, out-recover everything.
- Silhouette: Triangular glider sails behind shoulders; thin legs.
- Palette: sky blue #5AB1E6, sand #E9D8A6, scarlet #C8423A
- Signature weapon/tool: Kite blades
- Preferred range: Mid
- Strategy: Glide in, aerial strings, fall back to the ledge.
- Weaknesses: Poor grounded game, light, low kill power on the ground.
- Counterplay: Anti-air, platform pressure, ground trap.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 85. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Glide state with ticks remaining (max 90) in snapshot; third jump.

## 4. Specials
- **Neutral - Wind Cutter:** Arcing aerial projectile; speeds up with fall.
- **Side - Kite Glide:** Hold to glide up to 90 ticks; attacks from glide.
- **Up - Thermal Lift:** High multi-jump recovery with wind sweep.
- **Down - Dive Talon:** Fast dive; bounces on hit; vulnerable on whiff.
- **Ember Surge:** Tailwind: infinite glide stamina and wind projectiles pierce.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 0.95 and power 0.85; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 4 | 3 | 9 | 3 | 17 |
| FTilt | ground | 8 | 4 | 15 | 8 | 24 |
| UTilt | ground | 7 | 5 | 16 | 7 | 26 |
| DTilt | ground | 6 | 4 | 14 | 6 | 20 |
| FSmash | smash | 13 | 4 | 30 | 15 | 38 |
| USmash | smash | 11 | 5 | 32 | 14 | 37 |
| DSmash | smash | 10 | 6 | 34 | 13 | 34 |
| NAir | aerial | 5 | 10 | 11 | 7 | 19 |
| FAir | aerial | 9 | 5 | 17 | 9 | 26 |
| BAir | aerial | 8 | 4 | 15 | 10 | 27 |
| UAir | aerial | 7 | 5 | 14 | 8 | 24 |
| DAir | aerial | 11 | 4 | 19 | 11 | 29 |
| Grab | grab | 6 | 3 | 23 | 0 | 0 |
| SpecialN: Wind Cutter | special | per move | - | - | per move | - |
| SpecialS: Kite Glide | special | per move | - | - | per move | - |
| SpecialU: Thermal Lift | special | per move | - | - | per move | - |
| SpecialD: Dive Talon | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Floaty with sharp pulls. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Cyan wind streaks. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Wind, cloth snaps. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Glide state with ticks remaining (max 90) in snapshot.
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
