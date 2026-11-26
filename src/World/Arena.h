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
    // One-way platforms can be landed on from above but never block ascent.
    bool one_way{false};
};

// Four-sided out-of-bounds limits are arena data, not hard-coded inside the
// death test. The playable interval is half-open at each boundary according
// to Arena::is_out_of_bounds().
struct BlastZone {
    float left{0.0f};
    float right{0.0f};
    float top{0.0f};
    float bottom{0.0f};
};

// Owns the static arena layout (platforms + blast zone) and is the single
// source of truth for "what's solid here". Fighters query it for collision.
class Arena {
public:
    static constexpr int kMaxPlatforms = 8;
    enum class Id : int { TrainingRing = 0, EmberFoundry = 1, SkyTemple = 2 };

    Arena();

    // Loads one built-in, data-driven arena layout. The geometry is kept
    // separate from rendering so collision/spawn rules are deterministic.
    void reset(Id id);
    void reset_default() { reset(Id::TrainingRing); }
    Id id() const { return id_; }

    std::span<const Platform> platforms() const {
        return std::span<const Platform>(data_.data(),
                                          static_cast<std::size_t>(count_));
    }

    // Full four-sided blast-zone data. A body beyond any side is KO'd.
    const BlastZone& blast_zone() const { return blast_zone_; }
    // Compatibility accessor for older bottom-only consumers.
    float death_zone_y() const { return blast_zone_.bottom; }

    // Y at which a freshly-spawned fighter should sit. Two slots: P1 (left)
    // and P2 (right).
    float spawn_y() const { return spawn_y_; }
    float spawn_x(int player_index) const;

    // Resolves AABB-vs-platform overlap at the body's current position.
    // Mutates body and velocity; returns true if the body landed this step.
    bool resolve_collision(Aabb& body, Vec2& velocity,
                           bool drop_through_one_way = false) const;
    // Integrates one fixed-tick velocity using bounded substeps before each
    // collision pass. This prevents a fast hitstun/launch vector from crossing
    // a thin platform between two endpoint-only overlap checks.
    bool move_and_resolve(Aabb& body, Vec2& velocity,
                          bool drop_through_one_way = false) const;
    // True when the body is standing on a one-way platform (within tolerance).
    bool supported_by_one_way(const Aabb& body) const;
    // Four-sided blast-zone test, evaluated by Match before fighter update.
    bool is_out_of_bounds(const Aabb& body) const;

    // Renders the platforms via the supplied renderer.
    void render(Renderer& renderer) const;

private:
    std::array<Platform, kMaxPlatforms> data_{};
    Id    id_{Id::TrainingRing};
    int   count_{0};
    BlastZone blast_zone_{};
    float spawn_y_{0.0f};
    float spawn_x_[2]{0.0f, 0.0f};
};

} // namespace ember
