#pragma once

#include <cstdint>
#include <string_view>

// -----------------------------------------------------------------------------
// AI opponent tuning constants.
//
// The first enemy AI is a deterministic state machine; this file holds the
// per-difficulty knobs the AiController reads each tick. Centralising them
// here means a designer can rebalance the bot without touching the FSM
// implementation.
//
// Four shipped presets are selected through Settings and injected into
// the CPU controller when a match is created. The struct remains copyable
// so match setup and headless tests can choose a deterministic tier.
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

// Standard remains the compatibility/default preset.
inline const Difficulty kStandard = Difficulty{};

// M7-AI: explicit Easy / Hard presets alongside kStandard so the future
// difficulty-select menu has a knob to hand the Match without us having
// to retune the struct fields in a UI file.
//
// Tuning rationale:
//   Easy:   reaction_delay = 70 (~1.2s, slow), attack_frequency = 35
//           (rarely commits), recovery_priority = 50 (lazy recovery).
//           Mostly approaches, baits, and occasionally whiffs. A new
//           player can practice spacing and combos against this.
//   Standard: the original M4 default. reaction_delay = 40,
//           attack_frequency = 60, recovery_priority = 80. Honest bot
//           with a real punish game.
//   Hard:   reaction_delay = 18 (~0.3s, fast), attack_frequency = 85
//           (commits in range almost always), recovery_priority = 95
//           (never SDs). Tuned for the future "vs CPU" challenge tier;
//           the M7 match ships against Standard by default.
inline const Difficulty kEasy = Difficulty{
    /*reaction_delay_ticks*/     70,
    /*attack_frequency*/         35,
    /*recovery_priority*/        50,
    /*defensive_threshold_percent*/ 100,
    /*jump_aggression*/          20,
    /*seed*/                     0xE45Eu,
};

inline const Difficulty kHard = Difficulty{
    /*reaction_delay_ticks*/     18,
    /*attack_frequency*/         85,
    /*recovery_priority*/        95,
    /*defensive_threshold_percent*/ 70,
    /*jump_aggression*/          55,
    /*seed*/                     0xB0B5u,
};

// EXPERT is deliberately capped at a human-readable reaction floor. It is
// stronger than HARD through consistency/recovery, not frame-zero input reads.
inline const Difficulty kExpert = Difficulty{
    /*reaction_delay_ticks*/        14,
    /*attack_frequency*/            92,
    /*recovery_priority*/           100,
    /*defensive_threshold_percent*/ 55,
    /*jump_aggression*/             65,
    /*seed*/                        0xE7E27u,
};

// Stable settings-to-runtime mapping shared by startup and tests. Unknown or
// missing values safely fall back to Standard.
inline const Difficulty& from_name(std::string_view name) {
    if (name == "easy") return kEasy;
    if (name == "hard") return kHard;
    if (name == "expert") return kExpert;
    return kStandard;
}

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
