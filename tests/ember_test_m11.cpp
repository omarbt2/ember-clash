// Headless M11-AUDIT verification: deterministic state fingerprint.
//
// The M11 milestone is described in AGENTS.md / README.md as
// "online / competitive hardening - determinism + desync + disconnect
// audit". Because M9 (local multiplayer) and M10 (online multiplayer)
// are MISSING from the repo, M11 cannot harden any real online path.
// What M11 CAN do is verify that the simulation foundation is
// deterministic enough that a future online implementation would be
// possible. This harness proves that property.
//
// We test three contracts:
//   [1] state_hash() exists, returns a non-zero 64-bit value, and
//       changes after at least one match tick.
//   [2] Same inputs + same AI seeds -> identical state_hash() on
//       every tick over a 600-tick match with active combat.
//   [3] Different AI seeds -> different state_hash() sequences (i.e.
//       the hash is not a constant and not seeded to the same RNG
//       regardless of bot parameters).
//   [4] Two independent Matches driven side by side stay in lockstep
//       tick-by-tick; the hash sequence is bit-identical for 600
//       ticks even with the bot pressing attacks and being attacked.

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
#include <vector>

using namespace ember;

namespace {

} // namespace

int main() {
    int failures = 0;
    int total    = 0;
    auto check = [&](const char* label, bool cond) {
        ++total;
        std::printf("  %-58s %s\n", label, cond ? "PASS" : "FAIL");
        if (!cond) ++failures;
    };

    std::printf("== Ember Clash M11-AUDIT determinism test ==\n");

    // ---- [1] state_hash() basic contract -----------------------------------
    std::printf("\n[1] state_hash() basic contract\n");
    {
        // Use an active AI on P2 so the simulation actually does
        // something during the 30 ticks. Without input AND without
        // an AI, both fighters would just stand on the platform and
        // the hash would (correctly) not change - that's the right
        // hash behaviour but it doesn't prove the hash is alive.
        ai::Difficulty bot{};
        bot.reaction_delay_ticks = 5;
        bot.attack_frequency     = 80;
        bot.recovery_priority    = 80;
        bot.seed                 = 0xC0FFEEu;

        Match m;
        m.force_live_state_for_test();
        m.set_ai(1, std::make_unique<AiController>(bot));

        const std::uint64_t h0 = m.state_hash();
        check("state_hash() returns non-zero on a fresh match", h0 != 0);

        InputManager input;
        input.clear_for_test();
        for (int i = 0; i < 30; ++i) {
            input.begin_frame_for_test();
            m.update(input);
        }
        const std::uint64_t h30 = m.state_hash();
        check("state_hash() changes after 30 ticks of bot-only activity",
              h30 != h0);

        // Hash the same state again (without ticking) - should be
        // trivially stable. Catches accidental time-based noise
        // sneaking into the hash.
        const std::uint64_t h30_again = m.state_hash();
        check("state_hash() is deterministic (same state -> same hash)",
              h30 == h30_again);
    }

    // ---- [1b] Hash fidelity and simulation-state coverage -----------------
    std::printf("\n[1b] Hash fidelity and simulation-state coverage\n");
    {
        auto fighter_hash_changes = [](auto mutate) {
            Match baseline;
            Match changed;
            mutate(changed.fighter(0));
            return baseline.state_hash() != changed.state_hash();
        };
        check("fractional body movement changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.body().x += 0.25f; }));
        check("fractional velocity changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.velocity().x += 0.125; }));
        check("jump buffer timer changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_jump_buffer_ticks_for_test(3); }));
        check("coyote timer changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_coyote_ticks_for_test(2); }));
        check("attack hit-consumed state changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_attack_has_hit_for_test(true); }));
        check("shield and guard timer change the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_guard_state_for_test(550, 4); }));
        check("dodge frame, cooldown and direction change the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_dodge_state_for_test(2, 30, -1); }));
        check("surge meter and active frames change the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_surge_state_for_test(321, 12); }));
        check("parry flash changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_parry_flash_for_test(5); }));
        check("up-special resource changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_up_special_used_for_test(true); }));
        check("air-dodge resource changes the hash", fighter_hash_changes(
            [](Fighter& f) { f.set_air_dodge_used_for_test(true); }));

        Match default_ai;
        Match tuned_ai;
        ai::Difficulty tuned = ai::kStandard;
        tuned.seed ^= 0x13579BDFu;
        tuned.attack_frequency = 17;
        tuned_ai.set_ai(1, std::make_unique<AiController>(tuned));
        check("AI tuning and seed changes the hash",
              default_ai.state_hash() != tuned_ai.state_hash());

        Match default_runtime;
        Match changed_runtime;
        changed_runtime.ai_mutable(1)->set_runtime_state_for_test(
            AiController::State::Retreat, 7, 123, 0xDEADBEEFu);
        check("AI FSM, attack age and RNG changes the hash",
              default_runtime.state_hash() != changed_runtime.state_hash());

        Match default_shake_rng;
        Match changed_shake_rng;
        changed_shake_rng.set_shake_rng_state_for_test(
            default_shake_rng.shake_rng_state_for_test() ^ 0xA5A5A5A5u);
        check("KO shake RNG state changes the hash",
              default_shake_rng.state_hash() != changed_shake_rng.state_hash());

        // Simultaneous matches must not consume one process-global random
        // stream. Their KO shake state and fingerprints remain in lockstep.
        Match shake_left;
        Match shake_right;
        shake_left.set_ai(1, nullptr);
        shake_right.set_ai(1, nullptr);
        shake_left.force_live_state_for_test();
        shake_right.force_live_state_for_test();
        shake_left.fighter(0).set_stocks_for_test(0);
        shake_right.fighter(0).set_stocks_for_test(0);
        InputManager shake_input;
        bool shake_lockstep = true;
        for (int tick = 0; tick < 8; ++tick) {
            shake_left.update(shake_input);
            shake_right.update(shake_input);
            shake_lockstep = shake_lockstep
                && shake_left.state_hash() == shake_right.state_hash();
        }
        check("independent KO shakes remain deterministic in lockstep",
              shake_lockstep);
    }

    // ---- [2] Same inputs + same seeds -> same hash sequence ----------------
    std::printf("\n[2] Same inputs + same AI seed -> identical hash sequence\n");
    {
        auto play = [&]() {
            ai::Difficulty bot{};
            bot.reaction_delay_ticks = 5;
            bot.attack_frequency     = 80;
            bot.recovery_priority    = 80;
            bot.defensive_threshold_percent = 200;
            bot.jump_aggression      = 30;
            bot.seed                 = 0xC0FFEEu;

            Match m;
            m.force_live_state_for_test();
            m.set_ai(1, std::make_unique<AiController>(bot));

            InputManager input;
            input.clear_for_test();

            std::vector<std::uint64_t> hashes;
            hashes.reserve(601);
            hashes.push_back(m.state_hash());
            for (int i = 0; i < 600; ++i) {
                m.update(input);
                hashes.push_back(m.state_hash());
            }
            return hashes;
        };
        const auto a = play();
        const auto b = play();
        check("Both runs produced 601 hashes", a.size() == 601 && b.size() == 601);
        bool identical = (a == b);
        check("601/601 hashes match between the two runs", identical);
        if (!identical) {
            // Diagnostic: find the first divergence so a future
            // breakage can be debugged without rerunning manually.
            for (size_t i = 0; i < a.size(); ++i) {
                if (a[i] != b[i]) {
                    std::printf("    [debug] first diff at hash %zu: a=0x%016llx b=0x%016llx\n",
                                i,
                                (unsigned long long)a[i],
                                (unsigned long long)b[i]);
                    break;
                }
            }
        }
    }

    // ---- [3] Different AI seeds -> different hash sequences ---------------
    std::printf("\n[3] Different AI seeds -> different hash sequences\n");
    {
        auto play = [](std::uint32_t seed) {
            ai::Difficulty bot{};
            bot.reaction_delay_ticks = 5;
            bot.attack_frequency     = 80;
            bot.recovery_priority    = 80;
            bot.defensive_threshold_percent = 200;
            bot.jump_aggression      = 30;
            bot.seed                 = seed;

            Match m;
            m.force_live_state_for_test();
            m.set_ai(1, std::make_unique<AiController>(bot));

            InputManager input;
            input.clear_for_test();
            std::vector<std::uint64_t> hashes;
            hashes.reserve(301);
            for (int i = 0; i < 300; ++i) {
                m.update(input);
                hashes.push_back(m.state_hash());
            }
            return hashes;
        };
        const auto a = play(0xAAAAAAAAu);
        const auto b = play(0xBBBBBBBBu);
        check("Both runs produced 300 hashes", a.size() == 300 && b.size() == 300);
        bool different = (a != b);
        check("Different seeds produce different hash sequences", different);
        if (!different) {
            std::printf("    [debug] WARNING: two distinct AI seeds "
                        "produced identical state sequences\n");
        }
    }

    // ---- [4] Two matches side-by-side: identical lockstep ------------------
    // This is the core M11 contract: given the same tick-by-tick inputs,
    // two independent Match instances must produce the same hash on the
    // same tick. We use a real input sequence this time (alternating J
    // presses) so the AI and the human are both moving.
    std::printf("\n[4] Two independent Matches stay in lockstep\n");
    {
        auto play = [&]() {
            ai::Difficulty bot{};
            bot.reaction_delay_ticks = 5;
            bot.attack_frequency     = 60;
            bot.recovery_priority    = 60;
            bot.defensive_threshold_percent = 150;
            bot.jump_aggression      = 20;
            bot.seed                 = 0xDEADBEEFu;

            Match m;
            m.force_live_state_for_test();
            m.set_ai(1, std::make_unique<AiController>(bot));
            InputManager input;
            input.clear_for_test();

            std::vector<std::uint64_t> hashes;
            hashes.reserve(601);
            for (int i = 0; i < 600; ++i) {
                // Human presses J every 11th tick, K every 17th tick,
                // and L every 23rd tick. Combined with the bot's
                // own decisions, this exercises a lot of state.
                if (i % 11 == 0) {
                    input.begin_frame_for_test();
                    input.set_held_for_test(SDLK_J, true);
                } else if (i % 17 == 0) {
                    input.begin_frame_for_test();
                    input.set_held_for_test(SDLK_K, true);
                } else if (i % 23 == 0) {
                    input.begin_frame_for_test();
                    input.set_held_for_test(SDLK_L, true);
                } else {
                    input.begin_frame_for_test();
                }
                m.update(input);
                hashes.push_back(m.state_hash());
            }
            return hashes;
        };
        const auto a = play();
        const auto b = play();
        check("Both runs produced 600 hashes", a.size() == 600 && b.size() == 600);
        bool identical = (a == b);
        check("600/600 hashes match under identical input streams", identical);
        if (!identical) {
            for (size_t i = 0; i < a.size(); ++i) {
                if (a[i] != b[i]) {
                    std::printf("    [debug] first diff at hash %zu: a=0x%016llx b=0x%016llx\n",
                                i,
                                (unsigned long long)a[i],
                                (unsigned long long)b[i]);
                    break;
                }
            }
        }
    }

    // ---- Summary -----------------------------------------------------------
    std::printf("\nM11-AUDIT: %d assert(s), %d failed\n", total, failures);
    return failures == 0 ? 0 : 1;
}
