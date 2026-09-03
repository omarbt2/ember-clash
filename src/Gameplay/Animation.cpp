#include "Gameplay/Animation.h"

namespace ember::anim {

int Driver::frames_in_pose() const {
    // Pose blocks in the strip. Walk gets 2 frames; everything else
    // gets 1.
    switch (pose_) {
        case Sprite::AnimationKind::Walk: return 2;
        case Sprite::AnimationKind::Idle:
        case Sprite::AnimationKind::Jump:
        case Sprite::AnimationKind::Attack:
        case Sprite::AnimationKind::Hitstun:
        default:
            return 1;
    }
}

int Driver::ticks_per_frame_for(Sprite::AnimationKind pose) const {
    switch (pose) {
        case Sprite::AnimationKind::Idle:    return timing_.idle_ticks_per_frame;
        case Sprite::AnimationKind::Walk:    return timing_.walk_ticks_per_frame;
        case Sprite::AnimationKind::Jump:    return timing_.jump_ticks_per_frame;
        case Sprite::AnimationKind::Attack:  return timing_.attack_ticks_per_frame;
        case Sprite::AnimationKind::Hitstun: return timing_.hitstun_ticks_per_frame;
        default:                             return timing_.idle_ticks_per_frame;
    }
}

void Driver::tick(const Context& ctx) {
    // ----- Pick the pose based on the current context -----
    // The priority order is: dead > hitstun > attack > jump (airborne) >
    // walk (grounded + moving) > idle (grounded + still). This matches
    // the order M5's render() already uses for hitbox / outline
    // overlays, so the visual state stays consistent.
    Sprite::AnimationKind new_pose;
    if (ctx.dead)                  new_pose = Sprite::AnimationKind::Idle;  // dead: render nothing (Fighter::render short-circuits)
    else if (ctx.in_hitstun)       new_pose = Sprite::AnimationKind::Hitstun;
    else if (ctx.attacking)        new_pose = Sprite::AnimationKind::Attack;
    else if (!ctx.grounded)        new_pose = Sprite::AnimationKind::Jump;
    else if (ctx.horizontal_speed > 0.05f) new_pose = Sprite::AnimationKind::Walk;
    else                           new_pose = Sprite::AnimationKind::Idle;

    if (new_pose != pose_) {
        pose_            = new_pose;
        frame_in_pose_   = 0;
        ticks_until_next_ = ticks_per_frame_for(pose_);
        return;
    }

    // Same pose: count down, advance frame if we hit zero.
    if (--ticks_until_next_ <= 0) {
        const int n = frames_in_pose();
        frame_in_pose_ = (frame_in_pose_ + 1) % n;
        ticks_until_next_ = ticks_per_frame_for(pose_);
    }
}

} // namespace ember::anim
