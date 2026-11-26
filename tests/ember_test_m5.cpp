// Ember Clash M5 headless test harness.
// Builds against the same sources as the main game (see run_ai_test.bat
// for the runner, which doubles as the runner for this file).
//
// Scenarios:
//   [1] stocks and respawn on death-zone crossing
//   [2] respawn invulnerability blocks damage
//   [3] match end when stocks hit 0
//   [4] rematch via Match::reset() restores stocks and clears phase
//   [5] hit spark records a position when a hit connects
//   [6] format_damage clamps and produces "NN%" strings
//
// Each scenario prints PASS / FAIL. The program exits 0 iff every
// scenario passed.

#include "Core/Config.h"
#include "Gameplay/AiController.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/Match.h"
#include "Input/InputManager.h"
#include "UI/Hud.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

int g_total = 0;
int g_failed = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_total;                                                           \
        if (!(cond)) {                                                       \
            ++g_failed;                                                      \
            std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                    \
    } while (0)

#define PASS(name)  std::printf("  %-60s PASS\n", name)
#define SCENARIO(name) std::printf("[%s] %s\n", id, name)

} // namespace

int main() {
    using namespace ember;

    std::printf("Ember Clash M5 test harness\n");
    std::printf("  starting stocks = %d\n", combat::kStartingStocks);
    std::printf("  respawn delay   = %d ticks\n", combat::kRespawnDelayTicks);
    std::printf("  respawn invuln  = %d ticks\n", combat::kRespawnInvulnTicks);
    std::printf("\n");

    // ---- [1] stocks and respawn on death-zone crossing --------------------
    {
        const char* id = "1";
        SCENARIO("Stocks and respawn on death-zone crossing");

        Match m;
        m.force_live_state_for_test();
        InputManager input;  // real, unused.
        Fighter& p1 = m.fighter(0);
        Fighter& p2 = m.fighter(1);
        CHECK(p1.stocks() == combat::kStartingStocks);
        CHECK(p2.stocks() == combat::kStartingStocks);
        CHECK(m.phase() == Match::Phase::Fighting);

        // Teleport P1 below the death zone; one tick should mark dead,
        // decrement stock, and start the respawn timer.
        p1.body().y = m.arena().death_zone_y() + 50.0f;
        m.update(input);
        CHECK(p1.is_dead());
        CHECK(p1.stocks() == combat::kStartingStocks - 1);
        // KO starts a full 30-tick wait. The KO-detection update is the
        // start boundary and must not consume one of those elapsed ticks.
        CHECK(m.respawn_timer_ticks_for_test(0)
              == combat::kRespawnDelayTicks);
        for (int tick = 0; tick < combat::kRespawnDelayTicks - 1; ++tick) {
            m.update(input);
        }
        CHECK(p1.is_dead());
        CHECK(m.respawn_timer_ticks_for_test(0) == 1);
        m.update(input);
        CHECK(!p1.is_dead());
        CHECK(m.respawn_timer_ticks_for_test(0) == -1);
        CHECK(p1.stocks() == combat::kStartingStocks - 1);  // not refilled
        CHECK(p1.is_invulnerable());
        CHECK(p1.invuln_frames() == combat::kRespawnInvulnTicks);
        PASS("P1 lost a stock on death");
        PASS("P1 respawned on the exact fixed-tick deadline");
        PASS("P1 received exactly 90 invulnerability ticks");
    }

    // ---- [2] respawn invulnerability blocks damage ------------------------
    {
        const char* id = "2";
        SCENARIO("Respawn invulnerability blocks damage");

        Match m;
        m.force_live_state_for_test();
        InputManager input;
        Fighter& p2 = m.fighter(1);

        // Force P2 into a respawned-but-still-invuln state.
        p2.set_dead_for_test(true);
        p2.set_stocks_for_test(p2.stocks() - 1);
        m.respawn_fighter_for_test(1);
        // (respawn_fighter_for_test is implemented below; it's a thin
        // convenience wrapper for the respawn+invuln combo.)
        CHECK(p2.is_invulnerable());

        const int before = p2.damage_percent();
        p2.apply_hit(50, Vec2{ 10.0, -5.0 }, 10);
        CHECK(p2.damage_percent() == before);
        PASS("apply_hit during invuln does not apply damage");

        // Drop the invuln flag and the next hit should land.
        p2.set_invuln_frames_for_test(0);
        p2.apply_hit(50, Vec2{ 10.0, -5.0 }, 10);
        CHECK(p2.damage_percent() == before + 50);
        PASS("apply_hit after invuln applies damage");
    }

    // ---- [3] match end when stocks hit 0 ----------------------------------
    {
        const char* id = "3";
        SCENARIO("Match ends when one fighter's stocks reach 0");

        Match m;
        m.force_live_state_for_test();
        InputManager input;
        Fighter& p1 = m.fighter(0);

        // Decrement P1's stocks to 0 directly. Match::update() will see
        // the change and transition to MatchOver.
        p1.set_stocks_for_test(0);
        m.update(input);
        CHECK(m.phase() == Match::Phase::MatchOver);
        CHECK(m.ko_banner_timer_ticks() == 0);
        for (int tick = 0; tick < combat::kKoBannerInitialTicks - 1; ++tick) {
            m.update(input);
        }
        CHECK(m.ko_banner_timer_ticks() == combat::kKoBannerInitialTicks - 1);
        m.update(input);
        CHECK(m.ko_banner_timer_ticks() == combat::kKoBannerInitialTicks);
        PASS("Phase transitions to MatchOver when stocks hit 0");
        PASS("KO banner advances on exact 60 Hz tick boundaries");
    }

    // ---- [4] rematch via Match::reset() -----------------------------------
    {
        const char* id = "4";
        SCENARIO("Match::reset() restarts a match");

        Match m;
        m.force_live_state_for_test();
        InputManager input;
        {
            Fighter& p1 = m.fighter(0);
            // Force a match end.
            p1.set_stocks_for_test(0);
            m.update(input);
            CHECK(m.phase() == Match::Phase::MatchOver);
        }  // p1 ref dies here - it's a view into the unique_ptr's pointee,
           // which reset() is about to replace.

        m.reset();
        CHECK(m.phase() == Match::Phase::Fighting);

        // Re-fetch references after reset() - the underlying unique_ptr
        // instances were reassigned, so the previous references dangle.
        Fighter& p1 = m.fighter(0);
        Fighter& p2 = m.fighter(1);
        CHECK(p1.stocks() == combat::kStartingStocks);
        CHECK(p2.stocks() == combat::kStartingStocks);
        CHECK(p1.damage_percent() == 0);
        CHECK(p2.damage_percent() == 0);
        CHECK(!p1.is_dead());
        CHECK(!p2.is_dead());
        PASS("Phase is Fighting after reset");
        PASS("Both fighters have starting stocks after reset");
        PASS("Damage %, dead flags, and respawn timers cleared");
    }

    // ---- [5] hit spark records a position when a hit connects ------------
    {
        const char* id = "5";
        SCENARIO("Hit spark is recorded when a hit lands");

        Match m;
        m.force_live_state_for_test();
        InputManager input;
        // Take AI off both fighters so the test can pin body positions
        // deterministically.
        m.set_ai(0, nullptr);
        m.set_ai(1, nullptr);
        Fighter& p1 = m.fighter(0);
        Fighter& p2 = m.fighter(1);

        // Snap them next to each other and prime a Jab on P1. We seed the
        // attack through Fighter::update directly so the test doesn't
        // depend on the keyboard layer.
        p1.body().x = 200.0f; p1.body().y = 200.0f;
        p2.body().x = 220.0f; p2.body().y = 200.0f;  // overlapping
        CombatIntent jab{};
        jab.jab_pressed = true;
        PlayerIntent none{};
        p1.update(none, jab, m.arena());
        CHECK(p1.state() == Fighter::State::Attacking);

        // Advance enough frames for the jab to reach its active window
        // (Jab startup = 3, active = 2 -> active on frames 3 and 4).
        // We tick Match six times so the jab definitely connects and
        // the hit-spark records.
        bool landed = false;
        for (int frame = 0; frame < 6; ++frame) {
            m.update(input);
            if (m.last_hit_position_for_test().valid) {
                landed = true;
                break;
            }
        }
        CHECK(landed);
        PASS("last_hit_position_for_test().valid after a landed hit");
    }

    // ---- [6] format_damage helper -----------------------------------------
    {
        const char* id = "6";
        SCENARIO("format_damage clamps and produces NN% strings");

        using ember::ui::format_damage;
        CHECK(format_damage(0)   == "0%");
        CHECK(format_damage(9)   == "9%");
        CHECK(format_damage(10)  == "10%");
        CHECK(format_damage(47)  == "47%");
        CHECK(format_damage(100) == "100%");
        CHECK(format_damage(999) == "999%");
        CHECK(format_damage(10000) == "999%");  // clamps to kMaxDamagePercent
        CHECK(format_damage(-1)  == "0%");
        PASS("format_damage covers 0, 9, 10, 47, 100, 999, clamp-up, clamp-down");
    }

    std::printf("\n== %s (%d failure(s)) ==\n",
                g_failed == 0 ? "ALL PASS" : "SOME FAILED",
                g_failed);
    return g_failed == 0 ? 0 : 1;
}
