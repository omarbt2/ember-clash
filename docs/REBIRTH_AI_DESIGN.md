> **SUPERSEDED** by the production bible in `docs/rebirth/` (see `docs/rebirth/README.md`). Kept for history; some values here were corrected in `docs/rebirth/00_PLAN_REVIEW_CORRECTIONS.md`.

# EMBER CLASH REBIRTH — AI Design

## 1. Principle
AI outputs the same `InputFrame` a human produces (D6). It reads the same `MatchState` a human could see: positions, velocities, states, damage — but only via a **perception layer** that adds reaction delay and noise. No frame-perfect reads of the opponent's *next* input.

## 2. Baseline (audited)
`AiController` + `AiConfig.h`: `Difficulty` struct with kStandard/kEasy/kHard; deterministic with seed; tests in `ember_test_ai`. No EXPERT, no personalities, no training behaviors, no multi-opponent logic.

## 3. Architecture
Perception (delayed snapshot) → Situation model (distance bands, platform, ledge/stage risk, opponent state and recovery frames, own resources) → Utility planner (scores: approach, poke, punish, defend, recover, reposition) → Execution (input macros: dash-in poke, shield-grab, recovery routes) → InputFrame.

## 4. Difficulty (measurable)
| Level | Reaction delay | Execution error | Capabilities |
|---|---|---|---|
| EASY | 20–30 f | 15% dropped/wrong inputs | no shield/parry, weak recovery |
| NORMAL | 12–18 f | 8% | shields, basic punish |
| HARD | 8–12 f | 3% | punish on known recovery windows, combos ≤ 3 hits, ledge-aware |
| EXPERT | 6–8 f (never < human floor 6) | 1% | full punish tables, parry/dodge reads, stage control, adapts |
Acceptance: in 200-match AI-vs-AI sims, win rate of each level vs the one below ≥ 65%; EXPERT cannot react faster than 6 frames (unit-tested on the perception layer); same seed → same match hash.

## 5. Personalities
Aggressive, Defensive, Technical, Adaptive (tracks opponent habits over a match, e.g. shield rate, and shifts weights). Personality only changes utility weights, never perception limits.

## 6. Per-character move knowledge
Each fighter's move table carries tags (range, speed, safe-on-block, kill-power, recovery type); AI uses tags, so new fighters need no bespoke AI except special behaviors (traps, projectiles).

## 7. Training AI
Dummy settings: Stand, Crouch/shield (always/after hit/random), Attack (repeat chosen move), Recover (on/off), Replay recorded input loop. Frame data + hitbox overlay display. Phase P6.

## 8. Multi-player
FFA: target selection by threat/opportunity with commitment timer (no instant focus flipping). Teams: avoids friendly fire per mode rules, supports partner.

## 9. Tests
Perception latency, recovery success rate (≥ 95% EXPERT from standard launch set), difficulty ordering, determinism, no-cheat check (AI never reads opponent's pending input), 1000-match crash/soft-lock fuzz.
