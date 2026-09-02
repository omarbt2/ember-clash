#pragma once

#include "Core/Math.h"
#include "Core/Config.h"
#include "Physics/Motion.h"

#include <cstdint>

namespace ember {

class Arena;
class Renderer;

// One fighter on the stage. Holds its own physics state and exposes a single
// update() entry point. All game-facing mutations go through update() so the
// per-tick story is consistent.
class Fighter {
public:
    static constexpr float kWidth  = 48.0f;
    static constexpr float kHeight = 64.0f;

    // 0 = P1, 1 = P2. Determines spawn position (via Arena) and palette.
    Fighter(int player_index, const Arena& arena);

    int player_index() const { return player_index_; }
    const Aabb& body() const { return body_; }
    bool is_grounded() const { return grounded_; }
    bool is_dead() const { return dead_; }
    int  damage_percent() const { return damage_percent_; }

    // Resets the fighter to full health at its designated spawn position.
    void respawn(const Arena& arena);

    // One simulation tick. The intent is whatever the input layer produced
    // for this player; dt is currently passed through to the motion step but
    // unused in M2's per-tick model.
    void update(const PlayerIntent& intent, const Arena& arena, double dt);

    // Renders the fighter using its assigned palette color.
    void render(Renderer& renderer) const;

private:
    int   player_index_{0};
    Aabb  body_{};
    Vec2  velocity_{};
    bool  grounded_{false};
    int   jumps_remaining_{2};
    bool  dead_{false};
    int   damage_percent_{0};
    config::Color color_{};
};

} // namespace ember
