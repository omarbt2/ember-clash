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

    // Elevated platforms are one-way: they catch a descending fighter,
    // but do not block a fighter jumping upward through their underside.
    {
        const Platform& ledge = arena.platforms()[1];
        Aabb falling{350.0f, ledge.aabb.y - 100.0f, 48.0f, 64.0f};
        Vec2 down{0.0, 120.0};
        assert(arena.move_and_resolve(falling, down));
        assert(std::abs(falling.bottom() - ledge.aabb.y) < 1e-4f);
        assert(down.y == 0.0);

        Aabb rising{350.0f, ledge.aabb.bottom() + 2.0f - 64.0f, 48.0f, 64.0f};
        Vec2 up{0.0, -40.0};
        assert(!arena.move_and_resolve(rising, up));
        assert(rising.bottom() < ledge.aabb.y);
    }

    // Down+jump drop-through ignores one-way platforms for the initiating
    // tick only. On the next tick the body remains below the platform rather
    // than being snapped back onto it; the solid main floor still collides.
    {
        const Platform& ledge = arena.platforms()[1];
        Aabb body{350.0f, ledge.aabb.y - 64.0f, 48.0f, 64.0f};
        assert(arena.supported_by_one_way(body));
        Vec2 first_tick{0.0, 4.0};
        assert(!arena.move_and_resolve(body, first_tick, true));
        assert(body.bottom() > ledge.aabb.y);
        assert(!arena.supported_by_one_way(body));

        Vec2 second_tick{0.0, 4.0};
        assert(!arena.move_and_resolve(body, second_tick));
        assert(body.bottom() > ledge.aabb.y);

        const float floor_top = arena.platforms().front().aabb.y;
        Aabb above_floor{600.0f, floor_top - 80.0f, 48.0f, 64.0f};
        Vec2 toward_floor{0.0, 40.0};
        assert(arena.move_and_resolve(above_floor, toward_floor, true));
        assert(std::abs(above_floor.bottom() - floor_top) < 1e-4f);
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
