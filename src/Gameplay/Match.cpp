#include "Gameplay/Match.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Gameplay/Attack.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/InputMap.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"

#include <cmath>

namespace ember {

namespace {

// True iff two AABBs overlap (touching counts). Half-open in the inclusive
// direction; the smash-style "edge-tap" feel of touching = hit is desired
// here. The existing arena collision uses the same convention.
bool aabb_overlap(const Aabb& a, const Aabb& b) {
    return a.x < b.x + b.w
        && a.x + a.w > b.x
        && a.y < b.y + b.h
        && a.y + a.h > b.y;
}

// Given an attacker whose active hitbox overlaps a target's hurtbox, compute
// the per-hit response (knockback velocity + hitstun) and apply it. The
// caller is responsible for verifying the overlap and the attacker's
// `attack_has_hit_` flag; this function does not check them.
void apply_attack(Fighter& attacker, Fighter& target) {
    const Attack& atk = get_attack(attacker.current_attack_id());

    // Knockback formula per the design brief \u00a713.
    //     knockback = base * (1 + damage / divisor)
    const int damage = target.damage_percent();
    const double kb = static_cast<double>(atk.base_knockback)
                    * (1.0
                       + static_cast<double>(damage)
                         / static_cast<double>(combat::kKnockbackDamageDivisor));

    // Knockback angle is in attacker's local space; facing flips the x.
    const double sx = static_cast<double>(atk.knockback_angle_x) * static_cast<double>(attacker.facing());
    const double sy = static_cast<double>(atk.knockback_angle_y);
    // Re-normalize the unit vector in case a designer typed a non-unit
    // (cos, sin) pair into Attack.cpp. Cheap, no math.h needed.
    const double mag = std::sqrt(sx * sx + sy * sy);
    const double nx  = (mag > 1e-6) ? sx / mag : sx;
    const double ny  = (mag > 1e-6) ? sy / mag : sy;

    const Vec2 knockback_velocity{ nx * kb, ny * kb };

    target.apply_hit(atk.damage, knockback_velocity, atk.hitstun_frames);
}

} // namespace

Match::Match() {
    reset();
}

Match::~Match() = default;

void Match::reset() {
    arena_.reset_default();
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i] = std::make_unique<Fighter>(i, arena_);
    }
}

void Match::update(const InputManager& input, double dt) {
    // 1) Sample input once per fighter and advance physics + combat state.
    for (int i = 0; i < kFighterCount; ++i) {
        const InputMap::Sample s = InputMap::sample(input, i);
        fighters_[i]->update(s.motion, s.combat, arena_, dt);
    }

    // 2) Resolve hits: for each ordered pair (attacker, defender) where the
    //    attacker's current attack is in its active window and the hitbox
    //    overlaps the defender's hurtbox, apply the hit. has_hit_ prevents
    //    one swing from re-hitting every frame its hitbox is on screen.
    for (int a = 0; a < kFighterCount; ++a) {
        Fighter& attacker = *fighters_[a];
        const Aabb hb = attacker.active_hitbox();
        if (hb.w <= 0.0f || hb.h <= 0.0f) continue;

        for (int d = 0; d < kFighterCount; ++d) {
            if (d == a) continue;
            Fighter& target = *fighters_[d];
            if (target.is_dead()) continue;
            if (aabb_overlap(hb, target.body())) {
                apply_attack(attacker, target);
            }
        }
    }

    // 3) Auto-respawn: M3 has no KO counter yet, so dead fighters simply
    //    teleport back to their starting position. M5 will add a respawn
    //    timer and the stock / KO counter.
    for (int i = 0; i < kFighterCount; ++i) {
        if (fighters_[i]->is_dead()) {
            fighters_[i]->respawn(arena_);
        }
    }
}

void Match::render(Renderer& renderer) const {
    arena_.render(renderer);
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i]->render(renderer);
    }
    // M3 hurtbox overlay. Helps verify hit detection is firing on the
    // right frames; remove in M6.
    for (int i = 0; i < kFighterCount; ++i) {
        if (fighters_[i]->is_dead()) continue;
        const Aabb& body = fighters_[i]->body();
        renderer.draw_rect_outline(
            {body.x, body.y, body.w, body.h},
            config::kCombatHurtboxColor);
    }
}

const Fighter& Match::fighter(int index) const {
    return *fighters_[index];
}

} // namespace ember
