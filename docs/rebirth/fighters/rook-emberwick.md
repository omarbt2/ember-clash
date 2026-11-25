# Fighter Spec: Rook Emberwick

| Field | Value |
|---|---|
| Purpose | Design specification for Rook Emberwick (Ranged). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Rook Emberwick** - Ranged fighter. A lamplighter turned sharpshooter who guards a lonely tower. Dry humor.
- Combat fantasy: Control space with projectiles and punish approach.
- Silhouette: Long-barrel weapon extends the silhouette horizontally; short hooded torso.
- Palette: brass #B58A3A, steel blue #3C5A78, ember #E8602C
- Signature weapon/tool: Ember crossbow-pistol
- Preferred range: Long
- Strategy: Bolt pressure, turret zoning, retreat; chargeable bolts.
- Weaknesses: Poor in close range, slow jump, risk on whiff.
- Counterplay: Close distance, reflect, shield and rush.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 90. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Projectile ownership, lifetime and charge state; one turret at a time (snapshot).

## 4. Specials
- **Neutral - Ember Bolt:** Projectile, chargeable up to 40 ticks; straight line, lifetime 120.
- **Side - Fuse Lob:** Arcing bomb with 45-tick fuse; explodes on timer or hit.
- **Up - Thruster Hop:** Recovery burst; cannot attack during use.
- **Down - Spark Turret:** Deploy turret that fires every 60 ticks; max 1; destroyed by 2 hits.
- **Ember Surge:** Piercing Rounds: bolts pierce and the turret fires every 30 ticks.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.1 and power 0.9; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 4 | 3 | 10 | 3 | 18 |
| FTilt | ground | 9 | 4 | 18 | 8 | 25 |
| UTilt | ground | 8 | 5 | 19 | 7 | 27 |
| DTilt | ground | 7 | 4 | 16 | 6 | 22 |
| FSmash | smash | 15 | 4 | 35 | 16 | 40 |
| USmash | smash | 13 | 5 | 37 | 14 | 40 |
| DSmash | smash | 11 | 6 | 40 | 14 | 36 |
| NAir | aerial | 6 | 10 | 13 | 7 | 20 |
| FAir | aerial | 10 | 5 | 20 | 10 | 27 |
| BAir | aerial | 9 | 4 | 18 | 11 | 29 |
| UAir | aerial | 8 | 5 | 16 | 8 | 25 |
| DAir | aerial | 13 | 4 | 22 | 12 | 31 |
| Grab | grab | 7 | 3 | 26 | 0 | 0 |
| SpecialN: Ember Bolt | special | per move | - | - | per move | - |
| SpecialS: Fuse Lob | special | per move | - | - | per move | - |
| SpecialU: Thruster Hop | special | per move | - | - | per move | - |
| SpecialD: Spark Turret | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Sharp, recoil-driven. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Tracer trails, muzzle flash. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Crackling shots, mechanical clicks. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Projectile ownership, lifetime and charge state.
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
