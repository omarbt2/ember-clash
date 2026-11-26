// D-002 numeric gate probe. Run the same 100k-tick input stream under each
// compiler/optimization configuration and compare the emitted replay digest.
// This probe records constrained-float behavior; it does not decide D-002 by
// itself and does not substitute for Windows/MSVC or Emscripten evidence.
#include "Gameplay/AiController.h"
#include "Gameplay/Match.h"
#include "Input/InputManager.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {
constexpr int kReplayTicks = 100'000;

std::uint32_t next_random(std::uint32_t& value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

void fold_hash(std::uint64_t& digest, std::uint64_t state) {
    // FNV-1a over a defined little-endian byte sequence, independent of host
    // memory layout or struct padding.
    for (unsigned byte = 0; byte < 8; ++byte) {
        digest ^= static_cast<std::uint8_t>(state >> (byte * 8));
        digest *= 1099511628211ull;
    }
}
} // namespace

int main() {
    using namespace ember;
    Match match;
    match.force_live_state_for_test();
    ai::Difficulty difficulty = ai::kStandard;
    difficulty.seed = 0xD0022026u;
    match.set_ai(1, std::make_unique<AiController>(difficulty));

    InputManager input;
    std::uint32_t rng = 0xA341316Cu;
    std::uint64_t digest = 14695981039346656037ull;
    for (int tick = 0; tick < kReplayTicks; ++tick) {
        const std::uint32_t a = next_random(rng);
        const std::uint32_t b = next_random(rng);
        input.begin_frame_for_test();
        input.set_held_for_test(SDLK_A, (a & 0x7u) == 0u);
        input.set_held_for_test(SDLK_D, (a & 0x18u) != 0u);
        input.set_held_for_test(SDLK_W, (a & 0x3FFu) == 0x11u);
        input.set_held_for_test(SDLK_S, (a & 0x1F00u) == 0x800u);
        input.set_held_for_test(SDLK_J, (b & 0x1Fu) == 0u);
        input.set_held_for_test(SDLK_K, (b & 0x7Fu) == 0x21u);
        input.set_held_for_test(SDLK_L, (b & 0xFFu) == 0x42u);
        input.set_held_for_test(SDLK_I, (b & 0x3FFu) == 0x155u);
        input.set_held_for_test(SDLK_U, (a & 0xFFFFu) == 0xBEEFu);
        input.set_held_for_test(SDLK_LSHIFT, (a & 0x3FFFu) == 0x1222u);
        input.set_held_for_test(SDLK_O, (b & 0x1FFu) < 3u);
        input.set_held_for_test(SDLK_P, (a & 0x7FFu) == 0x333u);
        input.set_held_for_test(SDLK_Y, (b & 0xFFFFu) == 0xCAFEu);
        match.update(input);
        fold_hash(digest, match.state_hash());
    }

    std::printf("D002_FLOAT_REPLAY_HASH=%016llx TICKS=%d\n",
        static_cast<unsigned long long>(digest), kReplayTicks);
    return 0;
}
