# Physics and Movement

| Field | Value |
|---|---|
| Purpose | Movement/physics specification, including the dt question and test scenarios. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 03, 18, 21 |
| Revision date | 2026-11-26 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Implementation checkpoint (PARTIALLY VERIFIED)
Constants in `PhysicsConfig.h` are authored per 60 Hz tick (kWalkSpeed 6.0 px/tick, gravity, air control factor). Positions/velocities remain floating point; the numeric representation decision D-002 is still open. `step_motion` is a pure function of state, intent and tuning, and no longer accepts `dt` in the current local continuation work. Two jumps, jump buffer, coyote time, run/dash and drop-through foundations are present. Four-sided blast zones are now represented as arena data, and platform motion uses bounded substeps (maximum 16 px per substep) to prevent high-speed tunnelling.

DI, buffered landing tech, and main-stage ledge recovery now have a local implementation with focused GCC/Clang headless coverage; official SDL3 build and feel tuning remain unverified. DI is constrained to a 15-degree window and may not increase the downward component. Tech uses a shield press buffered for eight ticks. Ledge grabs require inward facing and grant 40 ticks of invulnerability once per airtime. See [P2 recovery and snapshot evidence](evidence/p2-di-tech-ledge-snapshot-2026-11-26.md) and [P2 fixed-step and collision evidence](evidence/p2-fixed-step-and-collision-recheck-2026-11-26.md).

## 2. Time model and the dt question (answers Correction C1)
- World unit = 1 px at logical 1280x720 [SC-8]. Tick = 1/60 s [SC-1].
- Decision D-001 remains **PROPOSED; no owner sign-off is recorded**. In the current local continuation work, gameplay update APIs are dt-free (`step_motion`, `Fighter::update`, `Match::update`, AI `decide`); the App's desktop and Web adapters share one fixed 60 Hz accumulator; respawn, post-respawn invulnerability and KO banner timers use integer ticks (30/90/90 respectively). The renderer remains outside the simulation clock. This is implementation evidence, not decision approval.
- **Replay evidence:** the respawn ordering defect was fixed: the KO-detection tick is now the start boundary, and the 30-tick timer decrements only on the following 30 complete simulation ticks. `ember_test_m5` asserts this deadline. The controlled 20-script/37,200-row replay now has 0 event-field mismatches and a maximum position delta of `0.0001220701 px` (below 0.01 px) against the equivalent-rule control. A separate no-KO replay still captures the intended side-ring-out behavior added by four-sided blast zones; it is a rules delta, not a fixed-step mismatch. D-001 remains PROPOSED pending owner sign-off and broader D-002/cross-platform gates; see the evidence report.
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
| Ledge | within 24 px of a grabbable edge, falling, facing inward | grab 40-tick invuln first time per airtime | contention (4P): first claimant keeps; others pushed | locally tested on main-stage edges; 4P contention remains open | - |
| Wall | optional: none at launch; blast zones only | - | deferred to post-launch (no wall jump) | - | - |
| Hitstun movement | knockback vector | decay 0.98/t, bounce on floor if KB > threshold | shield edge buffered for up to 8 ticks before landing (PROPOSED) | buffered landing-tech test | - |
| DI | direction held in airborne hitstun | rotation <= 15 deg from launch vector | cannot increase downward component toward stage | angle/groundward-input tests | - |
| Collision | AABB vs platforms | resolve X then Y, 1 px slop | tunnelling guard: max step 16 px/substep | no-tunnel fuzz test | - |
| Moving platforms | not in first release | - | deferred | - | - |
| Camera | tracks fighters midpoint | zoom 0.8-1.2, smoothing 8 ticks, clamp to stage camera bounds | presentation only, floating-point OK | - | readability pass |

## 4. Number representation
Sim state numeric type is decided by the gate in 18 section 4 (Q16.16 vs constrained float). Movement tests must pass under both candidates.

## 5. Acceptance (P2)
All mechanics above have tests; replay equivalence proof (section 2); determinism gate result recorded; feel session notes with 2 testers.
