# Fighter Spec: Kaelen Ashforge

| Field | Value |
|---|---|
| Purpose | Design specification for Kaelen Ashforge (Balanced). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Kaelen Ashforge** - Balanced fighter. A former apprentice of the Ashen Forge who carries the blade he failed to finish. Steady, stubborn, protective of newcomers.
- Combat fantasy: The reliable swordfighter: do everything decently, win by reading the opponent.
- Silhouette: Medium height, broad shoulder cloak, long blade diagonal - reads as a diagonal line plus a cape triangle.
- Palette: forge orange #E8602C, ash gray #4A4A52, deep indigo #1E1B3A
- Signature weapon/tool: Ember blade (single-handed bastard sword with vented spine)
- Preferred range: Mid
- Strategy: Space with tilts, confirm Forge Heat stacks (0-3, +1 per blade hit up to 3, decays after 6 s) to upgrade the next special.
- Weaknesses: No armor, no projectile zoning, average recovery.
- Counterplay: Out-range with projectiles, punish special recovery.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 100. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Forge Heat: stacks upgrade next special (burst radius +, flame trail lingers, rising phoenix gets a second hit). Stack count is simulation state.

## 4. Specials
- **Neutral - Ember Burst:** Hold to charge to 40 ticks; radial burst; charge adds knockback; at 3 Heat the burst leaves a 60-tick flame ring.
- **Side - Flare Dash:** Forward dash strike; re-press in a 10-tick window for a follow-up slash (cancel window); trail damages lightly.
- **Up - Rising Phoenix:** Vertical recovery with ledge snap; usable once per airtime; 3 Heat adds a second hit.
- **Down - Ground Slam:** Blade plunge; grounded shockwave, aerial dive with landing lag.
- **Ember Surge:** Overheat: Heat locked at 3 for the Surge duration (300 ticks).

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.0 and power 1.0; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 4 | 3 | 9 | 3 | 20 |
| FTilt | ground | 8 | 4 | 16 | 9 | 28 |
| UTilt | ground | 7 | 5 | 17 | 8 | 30 |
| DTilt | ground | 6 | 4 | 15 | 7 | 24 |
| FSmash | smash | 14 | 4 | 32 | 18 | 45 |
| USmash | smash | 12 | 5 | 34 | 16 | 44 |
| DSmash | smash | 10 | 6 | 36 | 15 | 40 |
| NAir | aerial | 5 | 10 | 12 | 8 | 22 |
| FAir | aerial | 9 | 5 | 18 | 11 | 30 |
| BAir | aerial | 8 | 4 | 16 | 12 | 32 |
| UAir | aerial | 7 | 5 | 15 | 9 | 28 |
| DAir | aerial | 12 | 4 | 20 | 13 | 34 |
| Grab | grab | 6 | 3 | 24 | 0 | 0 |
| SpecialN: Ember Burst | special | per move | - | - | per move | - |
| SpecialS: Flare Dash | special | per move | - | - | per move | - |
| SpecialU: Rising Phoenix | special | per move | - | - | per move | - |
| SpecialD: Ground Slam | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Wide arcs, weight on rear foot, smoke from vents on blade. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Orange arcs, ember trail, white-hot core on Heat 3. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Metal ring + low furnace hum; bright blade chimes on Heat gain. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Forge Heat: stacks upgrade next special (burst radius +, flame trail lingers, rising phoenix gets a second hit). Stack count is simulation state..
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).

## 12. Extended production detail (first production target)
- Hurtbox: body 48x64 stand (VERIFIED default), 48x44 when airborne-crouch; blade hitboxes extend 50-94 px past the body (matches VERIFIED jab 50-78 / ftilt 50-94 reach).
- Move intent: Jab 1-2-3 string (3 / 3 / 6 damage) for close spacing; FTilt is the main poke (angle 40 deg); DTilt trips low (angle 80); FSmash is the kill move (~110% kill at center stage, target); NAir is the escape (multi-frame).
- Heat rules: +1 per blade hit that is not blocked; consumed by the next special; lost on KO; max 3; decays 1 per 360 ticks without hits.
- AI tags: spacing poke = FTilt; punish = FSmash; recovery = Rising Phoenix at >= 40 px under ledge.
- First tests (P4): heat stack increments/decays; Flare Dash cancel window 10 ticks; Rising Phoenix once per airtime; Ember Burst charge cap 40.
