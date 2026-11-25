#include "World/Arena.h"
#include "Core/Config.h"

#include <cassert>

using namespace ember;

int main() {
    Arena arena;
    const float width = static_cast<float>(config::kRenderWidth);
    const float height = static_cast<float>(config::kRenderHeight);

    arena.reset(Arena::Id::TrainingRing);
    assert(arena.id() == Arena::Id::TrainingRing);
    assert(arena.platforms().size() == 3);
    assert(arena.spawn_x(0) < arena.spawn_x(1));
    assert(!arena.is_out_of_bounds(Aabb{arena.spawn_x(0), arena.spawn_y() - 64.0f, 48.0f, 64.0f}));

    arena.reset(Arena::Id::EmberFoundry);
    assert(arena.id() == Arena::Id::EmberFoundry);
    assert(arena.platforms().size() == 4);
    assert(arena.spawn_x(0) >= 150.0f);
    assert(arena.spawn_x(1) < width - 150.0f);

    arena.reset(Arena::Id::SkyTemple);
    assert(arena.id() == Arena::Id::SkyTemple);
    assert(arena.platforms().size() == 4);
    assert(arena.spawn_y() < height);

    assert(arena.is_out_of_bounds(Aabb{-400.0f, 200.0f, 48.0f, 64.0f}));
    assert(arena.is_out_of_bounds(Aabb{width + 400.0f, 200.0f, 48.0f, 64.0f}));
    assert(arena.is_out_of_bounds(Aabb{200.0f, -400.0f, 48.0f, 64.0f}));
    assert(arena.is_out_of_bounds(Aabb{200.0f, arena.death_zone_y() + 1.0f, 48.0f, 64.0f}));
    assert(!arena.is_out_of_bounds(Aabb{200.0f, 200.0f, 48.0f, 64.0f}));
    return 0;
}
