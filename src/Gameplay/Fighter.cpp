#include "Gameplay/Fighter.h"

#include "World/Arena.h"
#include "Graphics/Renderer.h"

namespace ember {

Fighter::Fighter(int player_index, const Arena& arena)
    : player_index_(player_index),
      color_(player_index == 0 ? config::kFighterP1 : config::kFighterP2) {
    respawn(arena);
}

void Fighter::respawn(const Arena& arena) {
    body_.x = arena.spawn_x(player_index_);
    body_.y = arena.spawn_y() - kHeight;  // feet flush with the spawn y
    body_.w = kWidth;
    body_.h = kHeight;
    velocity_      = {0.0, 0.0};
    grounded_      = true;
    jumps_remaining_ = 2;
    dead_          = false;
    damage_percent_ = 0;
}

void Fighter::update(const PlayerIntent& intent, const Arena& arena, double dt) {
    if (dead_) {
        return;
    }

    // --- Motion step (gravity, walk, jump, fast-fall) ------------------------
    const MotionState in{velocity_, grounded_, jumps_remaining_};
    const MotionResult out = step_motion(in, intent, dt);
    velocity_        = out.velocity;
    jumps_remaining_ = out.jumps_remaining;
    // grounded_ is updated below by collision resolution.

    // --- Integrate position --------------------------------------------------
    body_.x += static_cast<float>(velocity_.x);
    body_.y += static_cast<float>(velocity_.y);

    // --- Resolve collisions and refresh grounded state -----------------------
    grounded_ = arena.resolve_collision(body_, velocity_);

    // --- Death-zone check ----------------------------------------------------
    if (body_.y > arena.death_zone_y()) {
        dead_ = true;
    }
}

void Fighter::render(Renderer& renderer) const {
    if (dead_) {
        return;
    }
    renderer.draw_filled_rect(
        {body_.x, body_.y, body_.w, body_.h},
        color_);
    // Subtle outline so adjacent fighters read against the arena.
    renderer.draw_rect_outline(
        {body_.x, body_.y, body_.w, body_.h},
        config::kAccentColor);
}

} // namespace ember
