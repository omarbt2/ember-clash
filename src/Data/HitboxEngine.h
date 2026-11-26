#pragma once

#include "Core/Math.h"
#include "Data/MoveContent.h"

#include <vector>

namespace ember::data {

struct ActiveHitbox {
    HitboxShape shape = HitboxShape::Aabb;
    Aabb bounds{}; // For circles this is the diameter's bounding square.
    int damage = 0;
    int angle = 0;
    int base_knockback = 0;
    int knockback_scale = 0;
    int hitstun = 0;
    int group = 0;
};

// Evaluates inclusive move-local keyframe windows for one simulation tick.
// Local coordinates are mirrored around the fighter body when facing left.
std::vector<ActiveHitbox> hitboxes_at_tick(const MoveDefinition& move,
                                           int move_tick,
                                           const Aabb& fighter_body,
                                           int facing);

// Tests an active hitbox against the current AABB hurtbox. Circle hitboxes use
// closest-point distance; they are not approximated as rectangular hits.
bool hitbox_overlaps(const ActiveHitbox& hitbox, const Aabb& hurtbox);

} // namespace ember::data
