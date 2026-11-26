#include "Core/Config.h"
#include "World/Arena.h"

#include <cassert>
#include <cmath>
#include <cstdint>

int main() {
    using namespace ember;

    for (Arena::Id id : {Arena::Id::TrainingRing,
                         Arena::Id::EmberFoundry,
                         Arena::Id::SkyTemple}) {
        Arena arena;
        arena.reset(id);
        const BlastZone& zone = arena.blast_zone();
        assert(std::abs(zone.left + 170.0f) < 1e-6f);
        assert(std::abs(zone.right
            - (static_cast<float>(config::kRenderWidth) + 170.0f)) < 1e-6f);
        assert(std::abs(zone.top + 180.0f) < 1e-6f);
        assert(std::abs(zone.bottom
            - (static_cast<float>(config::kRenderHeight) + 200.0f)) < 1e-6f);
        assert(arena.death_zone_y() == zone.bottom);

        assert(!arena.is_out_of_bounds(Aabb{0.0f, 0.0f, 48.0f, 64.0f}));
        // Equality at a blast-zone boundary remains in bounds; crossing it
        // by any positive amount triggers the KO check.
        assert(!arena.is_out_of_bounds(Aabb{zone.left - 48.0f, 100.0f, 48.0f, 64.0f}));
        assert(arena.is_out_of_bounds(Aabb{zone.left - 48.01f, 100.0f, 48.0f, 64.0f}));
        assert(!arena.is_out_of_bounds(Aabb{zone.right, 100.0f, 48.0f, 64.0f}));
        assert(arena.is_out_of_bounds(Aabb{zone.right + 0.01f, 100.0f, 48.0f, 64.0f}));
        assert(!arena.is_out_of_bounds(Aabb{100.0f, zone.top - 64.0f, 48.0f, 64.0f}));
        assert(arena.is_out_of_bounds(Aabb{100.0f, zone.top - 64.01f, 48.0f, 64.0f}));
        assert(!arena.is_out_of_bounds(Aabb{100.0f, zone.bottom, 48.0f, 64.0f}));
        assert(arena.is_out_of_bounds(Aabb{100.0f, zone.bottom + 0.01f, 48.0f, 64.0f}));
    }

    return 0;
}
