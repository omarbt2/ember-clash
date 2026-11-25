# Fighter Spec: Bastian Wardenhold

| Field | Value |
|---|---|
| Purpose | Design specification for Bastian Wardenhold (Defensive). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Bastian Wardenhold** - Defensive fighter. Captain of a ruined gate. Believes every attack should teach the attacker.
- Combat fantasy: Punish aggression: reflect, counter, shield break.
- Silhouette: Large rounded shield in front - a circle plus a thin mace.
- Palette: steel #7C8794, gold #C9A23C, crimson #8C2F39
- Signature weapon/tool: Tower shield and lantern mace
- Preferred range: Close-mid
- Strategy: Bait, counter, reflect, then trade with armored moves.
- Weaknesses: Slow offense, predictable, poor recovery.
- Counterplay: Grabs, mixups on counter window, ranged pressure from outside shield.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 115. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Counter window and reflect flag; reflect cap 2 per projectile (snapshot).

## 4. Specials
- **Neutral - Bastion Stance:** Raise shield; 12 ticks reflect then guard; reflects projectiles.
- **Side - Warden's Shove:** Armored push with shield-break bonus.
- **Up - Lantern Ascent:** Short rising recovery with armor on startup 6 ticks.
- **Down - Counter Ward:** 20-tick counter window; retaliates against qualifying hits; lose to grabs.
- **Ember Surge:** Iron Vigil: counter and parry windows +4 ticks; reflect speed +30%.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.2 and power 1.1; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 5 | 3 | 11 | 3 | 22 |
| FTilt | ground | 10 | 4 | 19 | 10 | 31 |
| UTilt | ground | 8 | 5 | 20 | 9 | 33 |
| DTilt | ground | 7 | 4 | 18 | 8 | 26 |
| FSmash | smash | 17 | 4 | 38 | 20 | 50 |
| USmash | smash | 14 | 5 | 41 | 18 | 48 |
| DSmash | smash | 12 | 6 | 43 | 16 | 44 |
| NAir | aerial | 6 | 10 | 14 | 9 | 24 |
| FAir | aerial | 11 | 5 | 22 | 12 | 33 |
| BAir | aerial | 10 | 4 | 19 | 13 | 35 |
| UAir | aerial | 8 | 5 | 18 | 10 | 31 |
| DAir | aerial | 14 | 4 | 24 | 14 | 37 |
| Grab | grab | 7 | 3 | 29 | 0 | 0 |
| SpecialN: Bastion Stance | special | per move | - | - | per move | - |
| SpecialS: Warden's Shove | special | per move | - | - | per move | - |
| SpecialU: Lantern Ascent | special | per move | - | - | per move | - |
| SpecialD: Counter Ward | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Planted stance, deliberate swings. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Gold arcs, shield sparks. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Heavy shield clangs, bell tones. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Counter window and reflect flag.
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).
