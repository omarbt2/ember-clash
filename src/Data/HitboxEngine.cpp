#include "Data/HitboxEngine.h"

#include <algorithm>
#include <cmath>

namespace ember::data {
namespace {

bool valid_bounds(const Aabb& bounds) {
    return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
           std::isfinite(bounds.w) && std::isfinite(bounds.h) &&
           bounds.w > 0.0f && bounds.h > 0.0f &&
           std::isfinite(bounds.x + bounds.w) && std::isfinite(bounds.y + bounds.h);
}

bool aabb_overlaps(const Aabb& a, const Aabb& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

} // namespace

std::vector<ActiveHitbox> hitboxes_at_tick(const MoveDefinition& move,
                                           int move_tick,
                                           const Aabb& fighter_body,
                                           int facing) {
    std::vector<ActiveHitbox> active;
    if (move_tick < 0 || (facing != -1 && facing != 1) || !valid_bounds(fighter_body))
        return active;

    for (const HitboxKeyframe& keyframe : move.hitboxes) {
        if (keyframe.t0 < 0 || keyframe.t1 < keyframe.t0 ||
            move_tick < keyframe.t0 || move_tick > keyframe.t1) continue;
        if (!std::isfinite(keyframe.offset_x) || !std::isfinite(keyframe.offset_y) ||
            !std::isfinite(keyframe.size_x) || !std::isfinite(keyframe.size_y) ||
            keyframe.size_x <= 0.0f || keyframe.size_y <= 0.0f) continue;
        if (keyframe.shape == HitboxShape::Circle &&
            std::fabs(keyframe.size_x - keyframe.size_y) > 0.001F) continue;

        const float local_x = facing > 0
            ? keyframe.offset_x
            : (fighter_body.w - keyframe.offset_x - keyframe.size_x);
        ActiveHitbox hitbox;
        hitbox.shape = keyframe.shape;
        hitbox.bounds = Aabb{
            fighter_body.x + local_x,
            fighter_body.y + keyframe.offset_y,
            keyframe.size_x,
            keyframe.size_y,
        };
        if (!valid_bounds(hitbox.bounds)) continue;
        hitbox.damage = keyframe.damage;
        hitbox.angle = keyframe.angle;
        hitbox.base_knockback = keyframe.base_knockback;
        hitbox.knockback_scale = keyframe.knockback_scale;
        hitbox.hitstun = keyframe.hitstun;
        hitbox.group = keyframe.group;
        active.push_back(hitbox);
    }
    return active;
}

bool hitbox_overlaps(const ActiveHitbox& hitbox, const Aabb& hurtbox) {
    if (!valid_bounds(hitbox.bounds) || !valid_bounds(hurtbox)) return false;
    if (hitbox.shape == HitboxShape::Aabb) return aabb_overlaps(hitbox.bounds, hurtbox);
    if (hitbox.shape != HitboxShape::Circle ||
        std::fabs(hitbox.bounds.w - hitbox.bounds.h) > 0.001F) return false;

    const float radius = hitbox.bounds.w * 0.5F;
    const float center_x = hitbox.bounds.x + radius;
    const float center_y = hitbox.bounds.y + radius;
    const float closest_x = std::clamp(center_x, hurtbox.x, hurtbox.x + hurtbox.w);
    const float closest_y = std::clamp(center_y, hurtbox.y, hurtbox.y + hurtbox.h);
    const float dx = center_x - closest_x;
    const float dy = center_y - closest_y;
    return dx * dx + dy * dy <= radius * radius;
}

} // namespace ember::data
