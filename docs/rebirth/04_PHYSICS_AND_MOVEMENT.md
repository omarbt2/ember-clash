# Physics and Movement

| Field | Value |
|---|---|
| Purpose | Movement/physics specification, including the dt question and test scenarios. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 03, 18, 21 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Baseline (VERIFIED)
Constants in `PhysicsConfig.h` are px per 60 Hz tick (kWalkSpeed 6.0 px/tick, gravity, air control factor). Positions/velocities are `double`. `step_motion` is a pure function of (state, intent, tuning). 2 jumps (`jumps_remaining`). No coyote time, jump buffer, run, drop-through, ledge, wall.

## 2. Time model and the dt question (answers Correction C1)
- World unit = 1 px at logical 1280x720 [SC-8]. Tick = 1/60 s [SC-1].
- Decision D-001 (PROPOSED): gameplay code takes **no dt**. Mechanism: (a) remove `dt` params from `step_motion`, `Fighter::update`, `Match::update`, AI `decide`; (b) convert `respawn_timer_`, `ko_banner_timer_`, `respawn_invuln_seconds` to int ticks (0.5 s -> 30, 1.5 s -> 90, 1.5 s banner -> 90); (c) renderer interpolates with `alpha = accumulator/step`.
- **Proof obligation before merging (not just removing a parameter):** record 20 scripted input sequences (60-3600 ticks, covering every attack, KO, respawn) on baseline `2eeceae`; the refactor must reproduce per-tick positions within 0.01 px and identical event ticks (KO, respawn, attack start/end). Any difference must be explained (e.g. 30 vs 29 ticks from float accumulation) and signed off.
- Input latency requirement (answers B8): input read at the start of tick N; first resulting state change is visible in tick N state at the end of tick N (<= 1 tick of sim latency). Display latency budget: render once per vsync after tick; measured separately (manual camera test); online delay (SC-15) is additive and separate.

## 3. Mechanics
| Mechanic | Inputs / state | Parameters (initial) | Behavior and edge cases | Automated test | Manual feel test |
|---|---|---|---|---|---|
| Fixed tick | - | 60 Hz, catch-up cap kMaxSimTicksPerFrame | stall > cap drops time | accumulator unit test | drag-window stall |
| Gravity/terminal | airborne | g 0.55 px/t^2 (to tune), terminal 14 px/t | fast fall = 1.6x terminal | arc tests | jump arc feel |
| Ground accel/friction | move axis | accel 1.2, friction 0.8 | direction reversal 2-tick turn | speed-vs-time table | walk feel |
| Run/dash | double-tap or hold | dash 8 px/t for 12 ticks | dash-cancel into jump | dash distance test | - |
| Jump | jump pressed | full hop height ~150 px, short hop by release in first 5 ticks | 2 jumps | apex height tolerance +-2% | arc tuning |
| Jump buffer | jump pressed in last 5 ticks before landing | 5 ticks | executes on first actionable tick | buffer test | - |
| Coyote | left ledge | 4 ticks grace | ground jump allowed | coyote test | - |
| Air control | airborne move | factor kAirControl (VERIFIED value in config) | drift cap 0.8 x walk | drift test | - |
| Fast fall | down in air after apex | 1.6x | cancels on landing | - | - |
| Landing | land after aerial | landing lag per move | lag cancel none | lag tests | - |
| Drop-through | down + jump on pass-through platform | 6-tick ignore | not on main floor | platform test | - |
| Ledge | within 24 px of edge, falling, facing | grab 40-tick invuln first time per airtime | contention (4P): first claimant keeps; others pushed | ledge scenario | - |
| Wall | optional: none at launch; blast zones only | - | deferred to post-launch (no wall jump) | - | - |
| Hitstun movement | knockback vector | decay 0.98/t, bounce on floor if KB > threshold | tech window 8 ticks pre-landing (PROPOSED) | tech test | - |
| DI | direction held in hitstun | rotation <= 15 deg | cannot rotate into floor | angle tests | - |
| Collision | AABB vs platforms | resolve X then Y, 1 px slop | tunnelling guard: max step 16 px/substep | no-tunnel fuzz test | - |
| Moving platforms | not in first release | - | deferred | - | - |
| Camera | tracks fighters midpoint | zoom 0.8-1.2, smoothing 8 ticks, clamp to stage camera bounds | presentation only, floating-point OK | - | readability pass |

## 4. Number representation
Sim state numeric type is decided by the gate in 18 section 4 (Q16.16 vs constrained float). Movement tests must pass under both candidates.

## 5. Acceptance (P2)
All mechanics above have tests; replay equivalence proof (section 2); determinism gate result recorded; feel session notes with 2 testers.
