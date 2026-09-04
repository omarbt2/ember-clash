// Headless M7-AI verification. The M4 AI test (ember_test_ai.cpp) proves
// the FSM plays at all; this file proves the M7-AI polish:
//   [1] Difficulty presets: kEasy, kStandard, kHard have the expected
//       ordering of reaction_delay_ticks and attack_frequency.
//   [2] Defend actually backs off: when the bot is in Defend and the
//       opponent is to its right, the bot sets move_left = true.
//   [3] Defend always enters when the opponent is in attack startup
//       in range: no more 50% roll.
//   [4] Defend exits early when the opponent's attack ends.

#include "Core/Math.h"
#include "Gameplay/AiConfig.h"
#include "Gameplay/AiController.h"
#include "Gameplay/Attack.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/Match.h"
#include "Input/InputManager.h"
#include "Physics/Motion.h"
#include "World/Arena.h"

#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace ember;

namespace {

void drive(Match& m, InputManager& input, int ticks, double dt = 1.0 / 60.0) {
    for (int i = 0; i < ticks; ++i) {
        m.update(input, dt);
    }
}

// Begin a J-tap on the next tick for P1. Jab startup = 3, active = 2,
// total = 11 frames; the active window is on tick 3-4 of the attack.
void begin_jab_for_p1(InputManager& input) {
    input.begin_frame_for_test();
    input.set_held_for_test(SDLK_J, true);
}

} // namespace

int main() {
    int failures = 0;
    int total    = 0;
    auto check = [&](const char* label, bool cond) {
        ++total;
        std::printf("  %-58s %s\n", label, cond ? "PASS" : "FAIL");
        if (!cond) ++failures;
    };

    std::printf("== Ember Clash M7-AI polish test ==\n");

    // ---- [1] Difficulty preset field values --------------------------------
    std::printf("\n[1] Difficulty preset field values\n");
    {
        check("kEasy.reaction_delay_ticks == 70",
              ai::kEasy.reaction_delay_ticks == 70);
        check("kStandard.reaction_delay_ticks == 40",
              ai::kStandard.reaction_delay_ticks == 40);
        check("kHard.reaction_delay_ticks == 18",
              ai::kHard.reaction_delay_ticks == 18);
        check("Easy is slower than Standard (reaction)",
              ai::kEasy.reaction_delay_ticks
                > ai::kStandard.reaction_delay_ticks);
        check("Hard is faster than Standard (reaction)",
              ai::kHard.reaction_delay_ticks
                < ai::kStandard.reaction_delay_ticks);

        check("kEasy.attack_frequency < kStandard.attack_frequency",
              ai::kEasy.attack_frequency < ai::kStandard.attack_frequency);
        check("kHard.attack_frequency > kStandard.attack_frequency",
              ai::kHard.attack_frequency > ai::kStandard.attack_frequency);

        check("Easy recovery is lazier than Standard",
              ai::kEasy.recovery_priority < ai::kStandard.recovery_priority);
        check("Hard recovery is greedier than Standard",
              ai::kHard.recovery_priority > ai::kStandard.recovery_priority);

        check("All three seeds are distinct",
              ai::kEasy.seed != ai::kStandard.seed
              && ai::kEasy.seed != ai::kHard.seed
              && ai::kStandard.seed != ai::kHard.seed);
    }

    // ---- [2] Defend actually backs off --------------------------------------
    std::printf("\n[2] Defend backs away from the opponent\n");
    {
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 0;
        bot.recovery_priority    = 0;
        bot.defensive_threshold_percent = 999;
        bot.jump_aggression      = 0;
        bot.seed                 = 0xC0DEu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        m.ai_mutable(1)->set_skip_grace_for_test(true);
        InputManager input;
        input.clear_for_test();

        // Place P1 (opponent) to the right of P2 (bot) within ftilt range.
        Aabb& p1 = m.fighter(0).body();
        Aabb& p2 = m.fighter(1).body();
        p1.x = p2.x + p2.w + 20.0f;
        p1.y = p2.y;

        // Re-tap J every jab-length so P1 keeps attacking.
        bool saw_defend_backing_off = false;
        for (int tick = 0; tick < 90; ++tick) {
            if (tick % 11 == 0) {
                begin_jab_for_p1(input);
            } else {
                input.begin_frame_for_test();
            }
            m.update(input, 1.0 / 60.0);

            if (m.ai(1)->state() == AiController::State::Defend) {
                // We can't probe the intent directly because the
                // public ai() accessor returns const. Instead, sample
                // the bot's x position a few frames later and confirm
                // it moved LEFT (away from the opponent on the right).
                const float x_before = m.fighter(1).body().x;
                int defend_ticks = 0;
                for (int k = 0; k < 8; ++k) {
                    if (tick % 11 == 0) {
                        begin_jab_for_p1(input);
                    } else {
                        input.begin_frame_for_test();
                    }
                    m.update(input, 1.0 / 60.0);
                    if (m.ai(1)->state() == AiController::State::Defend) {
                        ++defend_ticks;
                    }
                }
                const float x_after = m.fighter(1).body().x;
                if (x_after < x_before - 1.0f) {
                    saw_defend_backing_off = true;
                }
                break;
            }
        }
        check("Bot in Defend moved LEFT (backed off from the opp on the right)",
              saw_defend_backing_off);
    }

    // ---- [3] Defend always enters when opp is in range + attacking ----------
    std::printf("\n[3] Defend always enters when opp is in range + attacking\n");
    {
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 0;
        bot.recovery_priority    = 0;
        bot.defensive_threshold_percent = 999;
        bot.jump_aggression      = 0;
        bot.seed                 = 0xBEEFu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        m.ai_mutable(1)->set_skip_grace_for_test(true);
        InputManager input;
        input.clear_for_test();

        Aabb& p1 = m.fighter(0).body();
        Aabb& p2 = m.fighter(1).body();
        p1.x = p2.x + p2.w + 20.0f;
        p1.y = p2.y;

        bool reached_defend = false;
        for (int tick = 0; tick < 90; ++tick) {
            if (tick % 11 == 0) {
                begin_jab_for_p1(input);
            } else {
                input.begin_frame_for_test();
            }
            m.update(input, 1.0 / 60.0);
            if (m.ai(1)->state() == AiController::State::Defend) {
                reached_defend = true;
                break;
            }
        }
        check("Bot reached Defend when opp is consistently in attacking range",
              reached_defend);
    }

    // ---- [4] Defend exits early when opponent's attack ends -----------------
    std::printf("\n[4] Defend exits early when opponent's attack ends\n");
    {
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 0;
        bot.recovery_priority    = 0;
        bot.defensive_threshold_percent = 999;
        bot.jump_aggression      = 0;
        bot.seed                 = 0xFACEu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        m.ai_mutable(1)->set_skip_grace_for_test(true);
        InputManager input;
        input.clear_for_test();

        Aabb& p1 = m.fighter(0).body();
        Aabb& p2 = m.fighter(1).body();
        p1.x = p2.x + p2.w + 20.0f;
        p1.y = p2.y;

        // Start a single jab on P1. Hold J for one frame (the tap),
        // then let the jab run its course (11 frames) without re-pressing.
        input.begin_frame_for_test();
        input.set_held_for_test(SDLK_J, true);
        m.update(input, 1.0 / 60.0);
        input.set_held_for_test(SDLK_J, false);

        // Drive with no further input. The bot should reach Defend,
        // then exit Defend when the jab completes and P1 returns to
        // Idle.
        bool reached_defend = false;
        for (int i = 0; i < 30; ++i) {
            input.begin_frame_for_test();
            m.update(input, 1.0 / 60.0);
            if (m.ai(1)->state() == AiController::State::Defend) {
                reached_defend = true;
                break;
            }
        }
        check("Bot reached Defend", reached_defend);
        if (!reached_defend) {
            std::printf("\nM7-AI: %d assert(s), %d failed\n", total, failures);
            return failures == 0 ? 0 : 1;
        }

        bool left_defend = false;
        for (int i = 0; i < 60; ++i) {
            input.begin_frame_for_test();
            m.update(input, 1.0 / 60.0);
            if (m.ai(1)->state() != AiController::State::Defend) {
                left_defend = true;
                break;
            }
        }
        check("Bot left Defend within 60 frames after opponent's swing ended",
              left_defend);
    }

    // ---- Summary ------------------------------------------------------------
    std::printf("\nM7-AI: %d assert(s), %d failed\n", total, failures);
    return failures == 0 ? 0 : 1;
}
