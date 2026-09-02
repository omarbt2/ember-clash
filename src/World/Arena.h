#pragma once

#include "Core/Math.h"

#include <array>
#include <span>

namespace ember {

class Renderer;

// One solid platform in the arena. Currently just an AABB; the spec keeps
// platforms simple (no slopes, no moving pieces) so we don't need more.
struct Platform {
    Aabb aabb;
};

// Owns the static arena layout (platforms + death zone) and is the single
// source of truth for "what's solid here". Fighters query it for collision.
class Arena {
public:
    static constexpr int kMaxPlatforms = 8;

    Arena();

    // Resets the platform list to the default training-stage layout and the
    // death-zone y. Called once at match start.
    void reset_default();

    std::span<const Platform> platforms() const {
        return std::span<const Platform>(data_.data(),
                                          static_cast<std::size_t>(count_));
    }

    // World-space y beyond which a fighter has died. Anything below is
    // considered outside the playable space.
    float death_zone_y() const { return death_zone_y_; }

    // Y at which a freshly-spawned fighter should sit. Two slots: P1 (left)
    // and P2 (right).
    float spawn_y() const { return spawn_y_; }
    float spawn_x(int player_index) const;

    // Resolves AABB-vs-platform collision against every platform. Mutates
    // body and velocity; returns true if the body ended up resting on top of
    // at least one platform (i.e. grounded this frame).
    bool resolve_collision(Aabb& body, Vec2& velocity) const;

    // Renders the platforms via the supplied renderer.
    void render(Renderer& renderer) const;

private:
    std::array<Platform, kMaxPlatforms> data_{};
    int   count_{0};
    float death_zone_y_{0.0f};
    float spawn_y_{0.0f};
    float spawn_x_[2]{0.0f, 0.0f};
};

} // namespace ember
