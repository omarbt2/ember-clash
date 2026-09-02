#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// AI opponent tuning constants.
//
// The first enemy AI is a deterministic state machine; this file holds the
// per-difficulty knobs the AiController reads each tick. Centralising them
// here means a designer can rebalance the bot without touching the FSM
// implementation.
//
// The M4 build ships a single "Standard" difficulty and the Match wires
// P2 to it by default. The struct is copyable so a future difficulty-select
// menu can hand the controller a different Difficulty each rematch.
// -----------------------------------------------------------------------------

namespace ember::ai {

// Single source of truth for how the AI "feels". All values are tuned
// for a 60 Hz simulation; the FSM scales them by dt where appropriate.
struct Difficulty {
    // 0..100. 0 = bot decides every frame (a strong CPU is a problem in
    // 1v1 because it can react on the same frame as the input). 20 = about
    // 1 tick of reaction, ~16 ms (feels like a fast human). 40 = the
    // default "Standard" bot.
    int reaction_delay_ticks = 40;

    // 0..100. Probability that the bot commits to an attack once in range
    // and past the reaction delay. Lower = more passive (the bot
    // approaches, baits, and then commits). 60 is the default.
    int attack_frequency = 60;

    // 0..100. How aggressively the bot recovers when launched off-stage.
    // 100 = always jumps back on the first opportunity. 0 = never jumps,
    // dies on purpose. Default 80.
    int recovery_priority = 80;

    // Damage% above which the bot prefers to retreat and wait for openings
    // instead of pressing advantage. Default 90.
    int defensive_threshold_percent = 90;

    // 0..100. Probability the bot double-jumps while approaching vs. just
    // walking. Higher = bot plays more air-to-air. Default 35.
    int jump_aggression = 35;

    // Xorshift32 seed. Two matches with the same seed + same opponent
    // inputs produce the same AI trace; the headless test harness relies
    // on this for determinism.
    std::uint32_t seed = 0xC1A5u;
};

// One default difficulty table for the M4 build. Future difficulty
// selections will add entries here.
inline const Difficulty kStandard = Difficulty{};

// The bot's three "in range" distance thresholds, in world pixels. These
// are read from the active hitbox reach (see src/Gameplay/Attack.cpp):
//   Jab    - body + 50..78 (reach ~ 50 px past own body)
//   FTilt  - body + 50..94 (reach ~ 70 px past own body)
//   NAir   - centered, +/- 55 around body (close, requires being airborne)
// We use a slightly smaller number than the raw reach so the bot tends to
// commit only when it is clearly in range, not on the edge.
inline constexpr float kJabRangePx   = 60.0f;
inline constexpr float kFtiltRangePx = 90.0f;
inline constexpr float kNairRangePx  = 70.0f;

// How long the bot stays in Retreat or Defend before re-evaluating. At
// 60 Hz this is ~1 second; it gives the human time to over-commit and
// reopens an attack window.
inline constexpr int kRetreatDurationTicks = 60;
inline constexpr int kDefendDurationTicks   = 30;

} // namespace ember::ai
