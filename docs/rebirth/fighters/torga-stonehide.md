# Fighter Spec: Torga Stonehide

| Field | Value |
|---|---|
| Purpose | Design specification for Torga Stonehide (Heavy). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 05_FIGHTER_ROSTER.md, 03_COMBAT_SYSTEM.md |
| Related | 05, 10, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Design brief
**Torga Stonehide** - Heavy fighter. Warden of the quarry who learned that patience wins fights. Speaks little; hits like a landslide.
- Combat fantasy: The immovable force: take a hit to give a bigger one.
- Silhouette: Wide, squat block with oversized fists - the widest silhouette of the roster.
- Palette: slate #5B6670, moss #6E7F4D, ember glow #E8602C
- Signature weapon/tool: Twin stone gauntlets
- Preferred range: Close
- Strategy: Use armor frames to bully trades; land smashes at 100%+ to take stocks.
- Weaknesses: Slow, huge hurtbox, weak recovery, grabs and projectiles punish armor.
- Counterplay: Grab, shield-poke, wait out armor frames, edge-guard.
- Skill floor/ceiling: see roster table in [05](../05_FIGHTER_ROSTER.md).

## 2. Attributes (INITIAL TUNING TARGETS, untested)
Weight 135. Movement values in [05 section 3](../05_FIGHTER_ROSTER.md). Jumps/air speed per that table.

## 3. Unique gameplay hook
Armor frames with a damage threshold (<= 10 damage does not flinch); armor never beats grabs; armor consumes a stamina bar that recovers over time (state in snapshot).

## 4. Specials
- **Neutral - Stone Guard:** Hold; 20 ticks of armor; releases a shove; cooldown 60 ticks.
- **Side - Boulder Charge:** Armored shoulder rush; ends against wall/ledge with stun.
- **Up - Rockfall Leap:** Slow short vertical leap; slam on descent; no armor in air; poor recovery by design.
- **Down - Quake Stomp:** Ground shockwave; launches grounded foes upward; weak in the air.
- **Ember Surge:** Mountain Heart: all smashes gain armor; stamina not consumed.

## 5. Move table (INITIAL TARGETS - not playtested; frames at 60 Hz, startup/active/recovery in ticks; damage in %; KB base)
Derived from the shared balanced template scaled by archetype speed 1.3 and power 1.3; per-move hand tuning happens in P4/P10.

| Move | Category | Startup | Active | Recovery | Damage | KB |
|---|---|---|---|---|---|---|
| Jab | ground | 5 | 3 | 12 | 4 | 26 |
| FTilt | ground | 10 | 4 | 21 | 12 | 36 |
| UTilt | ground | 9 | 5 | 22 | 10 | 39 |
| DTilt | ground | 8 | 4 | 20 | 9 | 31 |
| FSmash | smash | 18 | 4 | 42 | 23 | 58 |
| USmash | smash | 16 | 5 | 44 | 21 | 57 |
| DSmash | smash | 13 | 6 | 47 | 20 | 52 |
| NAir | aerial | 6 | 10 | 16 | 10 | 29 |
| FAir | aerial | 12 | 5 | 23 | 14 | 39 |
| BAir | aerial | 10 | 4 | 21 | 16 | 42 |
| UAir | aerial | 9 | 5 | 20 | 12 | 36 |
| DAir | aerial | 16 | 4 | 26 | 17 | 44 |
| Grab | grab | 8 | 3 | 31 | 0 | 0 |
| SpecialN: Stone Guard | special | per move | - | - | per move | - |
| SpecialS: Boulder Charge | special | per move | - | - | per move | - |
| SpecialU: Rockfall Leap | special | per move | - | - | per move | - |
| SpecialD: Quake Stomp | special | per move | - | - | per move | - |

Throws (F/B/U/D) and Pummel: 6-12 damage, 3-tick pummel hits; specifics in the move data file at implementation.

## 6. Animation requirements
Per [10](../10_ANIMATION_AND_VFX.md) the 47-clip list [SC-7]:
Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup, Jab, FTilt, UTilt, DTilt, FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash, NAir, FAir, BAir, UAir, DAir, SpecialN, SpecialS, SpecialU, SpecialD, Shield, Parry, Dodge, Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD, HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat, SurgeActivate.
Animation identity: Heavy anticipation, long follow-through, ground cracks. Attack clips must match the frame table above exactly (startup/active/recovery).

## 7. VFX requirements
Dust rings, rock chips, shockwave arcs. Effects must not hide the fighter silhouette (10).

## 8. Audio requirements
Low stone impacts, grinding rumble. Every move category has an attack, hit and whiff cue (11).

## 9. Balance risks
- Initial numbers are untested; high-risk areas: Armor frames with a damage threshold (<= 10 damage does not flinch).
- Matchup target: 40-60% vs each other fighter at HARD AI (05 section 4).

## 10. Test scenarios
1. Frame-exact tests for each move (03 section 16).
2. Hook state is included in snapshot and replays identically (18).
3. Counterplay listed above is reproducible in a scripted scenario.
4. Matchup sim vs every existing fighter.

## 11. Implementation dependencies
P3 combat engine (hooks, projectiles/traps/armor as needed), P5 art pipeline, build phase per [05](../05_FIGHTER_ROSTER.md).

## 12. Extended production detail (first production target)
- Hurtbox: 64x72 (wider than baseline 48x64) to enforce weakness.
- Armor budget: stamina 100; each armored absorb costs `damage*4`; at 0 the armor is off for 90 ticks.
- Move intent: Jab is short-range 2-hit; FSmash is the kill move with 8 armored ticks (target); DAir spike has long landing lag; Quake Stomp is the main anti-ground tool.
- Weight 135 means survives to ~150% before kill moves (target) and falls faster (hard to combo).
- AI tags: approach = Boulder Charge (when opponent shielding low); danger = projectile users.
- First tests (P4): armor absorbs <= 10 damage without flinch; grab ignores armor; stamina depletion; Boulder Charge ends on ledge.
