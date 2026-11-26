// Headless AI verification (M4 scratch test).
// Compiles outside the main project, links against the same gameplay sources,
// and prints the result. Delete after running.

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
#include <string>

using namespace ember;

namespace {
// Advance the match without SDL event polling; the default input state is idle.
void drive(Match& m, InputManager& input, int ticks) {
    for (int i = 0; i < ticks; ++i) m.update(input);
}
} // namespace

int main() {
    int failures = 0;
    auto check = [&](const char* label, bool cond) {
        std::printf("  %-58s %s\n", label, cond ? "PASS" : "FAIL");
        if (!cond) ++failures;
    };

    std::printf("== Ember Clash M4 AI scratch test ==\n");

    // --- Test 1: AI defaults to P2, P1 stays human -----------------------------
    std::printf("\n[1] Default AI wiring\n");
    {
        Match m;
        m.force_live_state_for_test();
        check("P1 (index 0) is human by default", !m.is_ai(0));
        check("P2 (index 1) is AI by default",     m.is_ai(1));
        check("P2 has an attached AiController",   m.ai(1) != nullptr);
    }

    // --- Test 2: Custom difficulty wires through -----------------------------
    std::printf("\n[2] Custom difficulty + determinism\n");
    {
        ai::Difficulty fast{};
        fast.reaction_delay_ticks = 5;
        fast.attack_frequency     = 95;
        fast.recovery_priority    = 100;
        fast.defensive_threshold_percent = 999;  // never retreat
        fast.jump_aggression      = 100;
        fast.seed                 = 0xABCDEFu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(fast));
        check("P2 now uses the custom controller", m.is_ai(1));
        check("Custom difficulty reached through match", m.ai(1)->difficulty().seed == 0xABCDEFu);
    }

    // --- Test 3: Bot actually plays (attacks happen) -------------------------
    std::printf("\n[3] Bot plays a real match against a stationary human\n");
    {
        // Make a Match where the human (P1) is just standing still, and the
        // bot has aggressive settings so it WILL commit.
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 95;
        bot.recovery_priority    = 100;
        bot.defensive_threshold_percent = 999;
        bot.jump_aggression      = 5;
        bot.seed                 = 0xC0FFEEu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        InputManager input;

        // Run for 6 seconds simulated (360 ticks).
        drive(m, input, 360);

        // P1 (the human) should have taken some damage from the bot's
        // attacks. The exact number depends on RNG, but with attack_frequency
        // = 95% and reaction_delay = 5, we expect at least one hit.
        const int p1_dmg = m.fighter(0).damage_percent();
        std::printf("    [debug] P1 final damage = %d\n", p1_dmg);
        check("P1 took damage from the bot (>= 1)", p1_dmg >= 1);

        // P2 (the bot) should have left Idle at some point and visited
        // Approach and Attack.
        const AiController* ai = m.ai(1);
        // We can't see the state *history* from the public API, only the
        // current state. So we just assert the current state is one of
        // the active ones, not Idle.
        const auto st = ai->state();
        std::printf("    [debug] P2 final AI state = %d\n", static_cast<int>(st));
        check("P2's AI is not stuck in Idle after 6s",
              st != AiController::State::Idle);
    }

    // --- Test 4: Bot transitions out of Idle (instrumented) ------------------
    std::printf("\n[4] FSM transition behaviour (deterministic)\n");
    {
        // Two matches with identical bot + identical opponents must play
        // identically. We track the bot's state sequence by snapshotting
        // `state()` at every tick.
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 10;
        bot.attack_frequency     = 80;
        bot.recovery_priority    = 80;
        bot.defensive_threshold_percent = 200;
        bot.jump_aggression      = 10;
        bot.seed                 = 0xDEADBEEFu;

        auto play = [&]() {
            Match m;
            m.force_live_state_for_test();
            m.set_ai(1, std::make_unique<AiController>(bot));
            InputManager input;
            std::string trace;
            for (int i = 0; i < 300; ++i) {
                drive(m, input, 1);
                trace += static_cast<char>('0' + static_cast<int>(m.ai(1)->state()));
            }
            return trace;
        };
        const std::string a = play();
        const std::string b = play();
        std::printf("    [debug] trace len a=%zu, b=%zu\n", a.size(), b.size());
        std::printf("    [debug] trace[..40] a = %.40s\n", a.c_str());
        std::printf("    [debug] trace[..40] b = %.40s\n", b.c_str());
        check("Identical seed -> identical AI trace (determinism)",
              a == b);
        // The trace must contain at least one Approach (1) and at least
        // one Attack (2) over 300 ticks.
        check("Trace contains Approach (1)", a.find('1') != std::string::npos);
        check("Trace contains Attack (2)",   a.find('2') != std::string::npos);
    }

    // --- Test 5: Bot retreats when its own damage is high --------------------
    std::printf("\n[5] Retreat on high damage\n");
    {
        // Set the bot's defensive threshold very low (20%) and crank up
        // attack_frequency so it would normally keep pressing. If the
        // retreat logic works, once it gets damaged above 20% it should
        // spend some time in Retreat (state 3).
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 99;
        bot.recovery_priority    = 80;
        bot.defensive_threshold_percent = 20;
        bot.jump_aggression      = 0;
        bot.seed                 = 0x12345u;

        // We can't easily hit the bot from outside without faking an
        // attacker. Simulate a "the bot got hit" by directly calling
        // apply_hit on the bot to push its damage above 20.
        AiController controller(bot);
        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        Match& mm = m;  // non-const for fighter access
        mm.fighter(1).apply_hit(40, {0, 0}, 0);

        // Now the bot should retreat at some point. We let it run long
        // enough for the reaction window to expire and the FSM to evaluate.
        bool saw_retreat = false;
        InputManager input;
        for (int i = 0; i < 300; ++i) {
            drive(m, input, 1);
            if (m.ai(1)->state() == AiController::State::Retreat) {
                saw_retreat = true;
                break;
            }
        }
        check("Bot entered Retreat after taking 40% damage (threshold 20)",
              saw_retreat);
    }

    // --- Test 6: Bot recovers when below the death zone ----------------------
    std::printf("\n[6] Recover when launched off-stage\n");
    {
        // We can't easily make the bot be "launched" by a hit, but we can
        // just shove it off-stage with apply_hit, then watch the FSM
        // transition into Recover.
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 50;
        bot.recovery_priority    = 100;
        bot.defensive_threshold_percent = 999;
        bot.jump_aggression      = 0;
        bot.seed                 = 0x99887u;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));
        Match& mm = m;  // non-const for fighter access
        // Launch just beyond the platform edge but keep the fighter inside
        // the arena side-KO margin. A larger impulse skips the short airborne
        // recovery window and correctly gets handled as an immediate KO.
        mm.fighter(1).apply_hit(0, {200.0f, -50.0f}, 0);

        bool saw_recover = false;
        InputManager input;
        for (int i = 0; i < 60; ++i) {
            drive(m, input, 1);
            if (m.ai(1)->state() == AiController::State::Recover) {
                saw_recover = true;
                break;
            }
        }
        check("Bot entered Recover when knocked off-stage", saw_recover);
    }

    // --- Summary -------------------------------------------------------------
    std::printf("\n== %s (%d failure(s)) ==\n",
                failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
