// P2 regression: identical scripted input must produce identical simulation
// state on every fixed tick. This is a durable in-tree counterpart to the
// 20-scenario replay evidence used during the fixed-step migration.
#include "Gameplay/AiController.h"
#include "Gameplay/Match.h"
#include "Input/InputManager.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

std::uint32_t next_random(std::uint32_t& value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

void set_scripted_input(ember::InputManager& input, int scenario, int tick,
                        std::uint32_t a, std::uint32_t b) {
    using namespace ember;
    input.begin_frame_for_test();
    const int move_phase = (tick + scenario * 13) % 460;
    const bool right = move_phase < 245 || ((a >> 2) % 11u == 0u);
    const bool left = !right && move_phase > 310;
    const int jump_period = 51 + scenario % 7;
    const int jab_period = 19 + scenario % 5;
    const int tilt_period = 43 + scenario % 9;
    const int air_period = 61 + scenario % 11;
    const bool jump = tick % jump_period == scenario * 3 % jump_period;
    const bool fast_fall = tick % 127 > 76 && (a & 3u) != 0u;
    const bool jab = tick % jab_period == scenario % jab_period;
    const bool tilt = tick % tilt_period == scenario * 5 % tilt_period;
    const bool nair = tick % air_period == scenario * 7 % air_period;
    const bool up = (b & 0x3Fu) == 0u;
    const bool special = tick % 109 == scenario * 11 % 109;
    const bool smash = tick % 137 > 118;
    const bool shield = tick % (113 + scenario % 13) < scenario % 5;
    const bool dodge = tick % (73 + scenario % 17) == scenario * 9 % (73 + scenario % 17);
    const bool surge = tick % 181 == scenario * 17 % 181;

    input.set_held_for_test(SDLK_A, left);
    input.set_held_for_test(SDLK_D, right);
    input.set_held_for_test(SDLK_W, jump);
    input.set_held_for_test(SDLK_S, fast_fall);
    input.set_held_for_test(SDLK_J, jab);
    input.set_held_for_test(SDLK_K, tilt);
    input.set_held_for_test(SDLK_L, nair);
    input.set_held_for_test(SDLK_I, up);
    input.set_held_for_test(SDLK_U, special);
    input.set_held_for_test(SDLK_LSHIFT, smash);
    input.set_held_for_test(SDLK_O, shield);
    input.set_held_for_test(SDLK_P, dodge);
    input.set_held_for_test(SDLK_Y, surge);
}

} // namespace

int main() {
    using namespace ember;
    constexpr int kScenarioCount = 20;
    int checked_ticks = 0;

    for (int scenario = 0; scenario < kScenarioCount; ++scenario) {
        Match first;
        Match second;
        first.set_ai(1, std::unique_ptr<AiController>{});
        second.set_ai(1, std::unique_ptr<AiController>{});
        first.force_live_state_for_test();
        second.force_live_state_for_test();

        const int ticks = 600 + (scenario % 16) * 200;
        std::uint32_t rng = 0x9E3779B9u
            ^ (static_cast<std::uint32_t>(scenario) * 0x85EBCA6Bu);
        InputManager input_first;
        InputManager input_second;

        for (int tick = 0; tick < ticks; ++tick) {
            const std::uint32_t a = next_random(rng);
            const std::uint32_t b = next_random(rng);
            set_scripted_input(input_first, scenario, tick, a, b);
            set_scripted_input(input_second, scenario, tick, a, b);
            first.update(input_first);
            second.update(input_second);

            if (first.state_hash() != second.state_hash()) {
                std::fprintf(stderr,
                    "Replay state hash mismatch: scenario=%d tick=%d\n",
                    scenario, tick);
                return 1;
            }
            ++checked_ticks;
        }
    }

    std::printf("PASS: %d deterministic ticks across %d scripted replays\n",
                checked_ticks, kScenarioCount);
    return 0;
}
