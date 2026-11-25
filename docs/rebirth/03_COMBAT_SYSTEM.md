# Combat System

| Field | Value |
|---|---|
| Purpose | Specifies the combat engine, data model and edge cases. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 04, 05, 19 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline (VERIFIED): 16 `AttackId`s, `Attack{startup, active, recovery, damage, base_knockback, hitstun, angle, hitbox, self_vx/vy, once_per_airtime}`; hit-stop 3 f; shield/parry/dodge constants in `CombatConfig.h`; `Fighter::receive_hit` returns Ignored/Hit/Blocked/Parried/Dodged. Everything below beyond that is PROPOSED.

## 1. Attack state machine
`Idle -> Startup -> Active -> Recovery -> Idle`, with cancel windows. Frame index counts ticks from 0. Active window = `[startup, startup+active)` (matches `attack_frame_is_active`). Buffering: a press during the last 6 ticks of recovery executes on first actionable tick (04).

## 2. Hitboxes and hurtboxes
Each move: list of hitbox *keyframes* `{start_tick, end_tick, shape(AABB|circle), offset, size, damage, angle, base_kb, kb_scale, hitstun, group, effect}`. Fighter hurtbox: body AABB (48x64 today, VERIFIED) with per-state shrink (crouch/air). Multi-hit: `group` id; one hit per group per target per swing; `rehit_ticks` for multi-hit. Hit detection is evaluated in fixed player index order each tick to make simultaneous hits deterministic.

## 3. Damage and knockback (initial)
`KB = base_kb + kb_scale * damage_percent_after_hit / 100 * (100 / weight)`; launch vector = angle * KB; hitstun = max(4, round(KB * 0.4)) (kMinHitstunFrames=4 VERIFIED; coefficient PROPOSED). Weight table in 05. Surge multipliers VERIFIED (1.25/1.15).

## 4. DI and hitstun
During hitstun the victim holds a direction: launch vector rotates up to 15 degrees toward the held direction (PROPOSED); survival DI/ASDI details in 04.

## 5. Hit stop
3 ticks default (VERIFIED), heavy hits +2; both parties frozen, victim may DI during hit-stop (shake offset presentation only).

## 6. Armor
`armor_threshold` (damage that does not cause flinch) active on listed ticks; damage still accrues. Heavy fighters: armored frames on specific smashes only.

## 7. Shield, parry, dodge
Shield drains 2/tick, regens 3/tick (VERIFIED); damage absorbed costs 40 per % (VERIFIED); break stun 70 (VERIFIED). Parry: first 6 ticks of shield, attacker stunned 24 (VERIFIED). Dodge: 24 ticks, invuln 2..14 (VERIFIED), cooldown 40.

## 8. Grabs/throws (MISSING; P3)
Grab 6 startup, 3 active, ~24 recovery (initial). Beats shield, loses to dodge and aerial attacks. Pummel 3 dmg at 20-tick cadence, throw by direction. Grab release after `kGrabHold = f(damage)`.

## 9. Projectiles (MISSING; P3)
Entity `{owner, fighter_slot, spawn_tick, lifetime_ticks, pos, vel, shape, group, reflect_count, destroyable_hp}`. Ownership = spawning fighter slot; lifetime capped (<= 300 ticks); max live projectiles per fighter and per match (16/64 budgets, 20). Projectile vs projectile: higher priority destroys lower; equal cancels both. Reflector flips owner (reflect_count++ to cap 2). Projectile persists when owner dies unless `die_with_owner`.

## 10. Traps (MISSING)
Placed entities with arming time, limit per owner, and visible lifetime; teams do not trigger own team's traps (setting).

## 11. Counters and reflectors
Counter window = N ticks; if hit with qualifying move, hit is negated and a retaliation move starts; counters lose to grabs. Reflector active ticks listed in fighter data.

## 12. Priority and clanking
Two overlapping active hitboxes: damage difference >= 9 -> higher wins; otherwise clank, both enter 10-tick recovery pause (initial). Simultaneous hits on each other resolve as trade.

## 13. Invulnerability
Sources: respawn (90 ticks VERIFIED as 1.5 s), dodge, specific move frames, ledge. Invulnerable defenders still show hit effect "whiff"; grabs ignore intangible only if listed.

## 14. Combo windows
Combos are not guaranteed: hitstun and DI make true combos rare at higher %. Design rule: no zero-to-death infinite; guard against by hitstun decay (`staleness`: repeated move reduces damage 5%, floor 60%, queue of 8, PROPOSED).

## 15. Data representation
Moves stored as JSON (19). Example (Jab, illustrative initial targets):
```json
{ "id":"jab","category":"ground","startup":4,"active":3,"recovery":9,
  "cancel":{"to":["jab2"],"from":7,"to_tick":13},
  "hitboxes":[{"t0":4,"t1":6,"shape":"aabb","off":[30,18],"size":[34,22],
    "damage":3,"angle":361,"base_kb":20,"kb_scale":6,"hitstun":9,"group":0,"fx":"spark_light"}] }
```
C++ hooks: `onStart`, `onActive(tick)`, `onHit`, `onEnd` registered by name for unique behavior (05).

## 16. Tests
Per move: frame-exact scenario (press at tick 0 -> hitbox appears at tick startup; ends at startup+active; actionable at total). Pairwise priority matrix tests; projectile lifetime tests; deterministic replay (18).
