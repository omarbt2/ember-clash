# AI Design

| Field | Value |
|---|---|
| Purpose | Implementation-ready AI specification. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 05, 13, 21 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Baseline (VERIFIED)
`AiController` + `Difficulty{reaction_delay_ticks, attack_frequency, recovery_priority, defensive_threshold_percent, jump_aggression, seed}`. Easy delay 70, Hard 18 ticks. Decisions are per-tick, seeded, tick-based cooldown (kAiAttackCooldownTicks 30) and startup grace 90 ticks. Tests: `ember_test_ai`. No EXPERT, personalities, training mode, multi-opponent, projectile/trap awareness.

## 2. Architecture
Perception -> Situation model -> Utility scoring -> Execution -> `InputFrame` (same type as human, D6). The AI only reads a `PerceivedState` built from the sim state delayed by N ticks (the reaction delay) with optional position noise; it never reads the opponent's pending input or hidden RNG.

## 3. Perception and anti-cheat rules
Reaction delay buffer of N ticks for opponent state changes (attack start, shield, dodge). Hidden info forbidden: opponent input buffer, RNG state, future positions. Unit test: swapping the opponent's pending input does not change the AI's output within the reaction window.

## 4. Difficulty (calibrated bands; replaces earlier 6-8 f proposal)
| Level | Reaction ticks | Execution error | Capabilities | Success criteria |
|---|---|---|---|---|
| EASY | 45-70 | 15% drop/wrong | no shield/parry, weak recovery, attacks at 35% frequency | loses to NORMAL >= 65% |
| NORMAL | 28-36 | 8% | shield, basic punish, recovers 70% | loses to HARD >= 60% |
| HARD | 18-24 | 3% | punishes known windows, combos <= 3 hits, stage aware | loses to EXPERT >= 55% |
| EXPERT | 12-16 (never < 12) | 1% | full punish table, parry/dodge reads, adaptive | beats HARD >= 55% |
Calibration: human reaction to a visual cue is about 12-15 ticks in practice; measure the real playtest distribution (P6 manual) and shift bands if testers' median is faster. Difficulty is capability+noise, never hidden information or sub-human reaction.

## 5. Decision making
Situation features: distance band (0-60, 60-140, 140+), opponent state/recovery ticks remaining, platform/ledge risk, own %, shield stamina, Surge meter, resources. Utility = weight x feature scores for actions {approach, poke, punish, defend, recover, reposition, bait}. Execution via macros (dash-in poke, shield-grab, ledge recovery routes). Recovery logic: choose route (jump count, special, ledge) with success estimate; recovery_priority parameter retained (VERIFIED).
Combo behavior: follows move tags (`combo_starter`, `kill`, `safe_on_block`); stops when DI probability high.

## 6. Personalities
Aggressive, Defensive, Technical, Adaptive: change utility weights only. Adaptive tracks opponent shield rate/habit counters per match.

## 7. Multi-opponent and teams
Target selection by threat/opportunity score with 90-tick commitment; FFA avoids dogpiling a mid-recovery fighter unless kill-confirmable; teams: do not attack partner, protect partner at ledge.

## 8. Determinism
Seeded PRNG per AI (xorshift32) stored in the sim snapshot; same seed + inputs = same match hash.

## 9. Training mode
Dummy: stand/crouch/shield(always, after hit, random)/attack(repeat chosen move)/recover(on/off)/replay recorded inputs. Settings: damage % set/reset, position reset (hotkey), infinite shield/Surge, frame data panel (startup/active/recovery/advantage), hitbox/hurtbox overlay, input display, record/playback of up to 600 ticks, practice scenarios (edge-guard, recovery, punish drills as data).

## 10. Testing
Perception latency test; difficulty ordering via 200-match AI-vs-AI per pair; recovery success (EXPERT >= 95% from standard launch set); determinism; no-hidden-info test; 1000-match soft-lock fuzz; matchup simulation tool (05).
