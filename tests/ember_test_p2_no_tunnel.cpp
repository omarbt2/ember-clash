#include "World/Arena.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
std::uint32_t next_random(std::uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
}

int main() {
    using namespace ember;
    Arena arena;
    arena.reset(Arena::Id::TrainingRing);
    const float floor_top = arena.platforms().front().aabb.y;

    // A single 600 px/tick endpoint update used to jump from above to below
    // the floor without overlapping it. Bounded substeps must land instead.
    {
        Aabb body{600.0f, 200.0f, 48.0f, 64.0f};
        Vec2 velocity{0.0, 600.0};
        assert(arena.move_and_resolve(body, velocity));
        assert(std::abs(body.y - (floor_top - body.h)) < 1e-4f);
        assert(velocity.y == 0.0);
    }

    // Deterministic vertical high-speed sweep set. Every trajectory begins
    // above the main floor and must resolve on its top surface regardless of
    // how far it would move in one unsplit tick.
    std::uint32_t rng = 0xC001D00Du;
    for (int sample = 0; sample < 512; ++sample) {
        const std::uint32_t r0 = next_random(rng);
        const std::uint32_t r1 = next_random(rng);
        const float y = 40.0f + static_cast<float>(r0 % 500u);
        // Guarantee that each sample actually crosses the floor during this
        // tick; a velocity that stops 1 px short is not a landing failure.
        const double remaining_gap = static_cast<double>(floor_top - (y + 64.0f));
        const double vy = remaining_gap + 1.0
            + static_cast<double>(r1 % 2000u);
        Aabb body{600.0f, y, 48.0f, 64.0f};
        Vec2 velocity{0.0, vy};
        const bool grounded = arena.move_and_resolve(body, velocity);
        assert(grounded);
        assert(std::abs(body.y - (floor_top - body.h)) < 1e-4f);
        assert(velocity.y == 0.0);
        assert(!intersects(body, arena.platforms().front().aabb));
    }

    // High-speed descent must also land on a thin elevated platform rather
    // than passing through to the main floor.
    {
        const Platform& side_platform = arena.platforms()[1];
        Aabb body{350.0f, side_platform.aabb.y - 220.0f, 48.0f, 64.0f};
        Vec2 velocity{0.0, 500.0};
        assert(arena.move_and_resolve(body, velocity));
        assert(std::abs(body.bottom() - side_platform.aabb.y) < 1e-4f);
        assert(velocity.y == 0.0);
    }

    // Invalid velocity is contained before it can poison an AABB or an
    // integer substep calculation.
    {
        Aabb body{600.0f, 200.0f, 48.0f, 64.0f};
        Vec2 velocity{0.0, std::numeric_limits<double>::quiet_NaN()};
        assert(!arena.move_and_resolve(body, velocity));
        assert(std::isfinite(body.x) && std::isfinite(body.y));
        assert(velocity.x == 0.0 && velocity.y == 0.0);
        assert(std::abs(body.y - 200.0f) < 1e-6f);
    }

    return 0;
}
