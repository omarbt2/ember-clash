#include "Gameplay/AiController.h"

#include "Core/Logger.h"
#include "Gameplay/Attack.h"
#include "Gameplay/Fighter.h"
#include "World/Arena.h"

#include <cmath>

namespace ember {

AiController::AiController(ai::Difficulty difficulty)
    : difficulty_(difficulty)
    , rng_(difficulty.seed) {}

// --------------------------------------------------------------------
// RNG
// --------------------------------------------------------------------
std::uint32_t AiController::next_rand() {
    // xorshift32. Must be non-zero; the constructor seeds from difficulty.seed
    // which we constrain to a non-zero value below.
    std::uint32_t x = rng_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_ = x;
    return x;
}

int AiController::rand_percent() {
    return static_cast<int>(next_rand() % 100u);
}

// --------------------------------------------------------------------
// Geometry helpers
// --------------------------------------------------------------------
float AiController::body_center_distance(const Fighter& a, const Fighter& b) {
    const float ax = a.body().x + a.body().w * 0.5f;
    const float ay = a.body().y + a.body().h * 0.5f;
    const float bx = b.body().x + b.body().w * 0.5f;
    const float by = b.body().y + b.body().h * 0.5f;
    const float dx = ax - bx;
    const float dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}

float AiController::horizontal_gap(const Fighter& a, const Fighter& b) {
    // Distance between the two body edges along x. Zero = touching; positive
    // = there is air between them. Negative = overlapping (shouldn't happen
    // post-collision but is fine; the AI just sees "very close").
    if (a.body().x + a.body().w < b.body().x) {
        return b.body().x - (a.body().x + a.body().w);
    }
    if (b.body().x + b.body().w < a.body().x) {
        return a.body().x - (b.body().x + b.body().w);
    }
    return 0.0f;
}

bool AiController::in_jab_range(const Fighter& self, const Fighter& opp) {
    return horizontal_gap(self, opp) <= ai::kJabRangePx;
}

bool AiController::in_ftilt_range(const Fighter& self, const Fighter& opp) {
    return horizontal_gap(self, opp) <= ai::kFtiltRangePx;
}

bool AiController::in_nair_range(const Fighter& self, const Fighter& opp) {
    // Nair is centred on the body; useful when both fighters are airborne
    // and close. We additionally require a small vertical gap so the AI
    // doesn't commit to a nair when both are standing.
    if (!opp.is_grounded() && !self.is_grounded()) {
        return body_center_distance(self, opp) <= ai::kNairRangePx;
    }
    return false;
}

bool AiController::is_off_stage(const Fighter& self, const Arena& arena) {
    // A fighter is "off stage" when they have either:
    //   * fallen below the death zone (lost), or
    //   * drifted past the rightmost platform's right edge while airborne.
    // The M4 AI recovers from "drifted off horizontally while in the air";
    // the Match itself still teleports truly-dead fighters back, so the
    // Recover state only fires in the brief window between launch and KO.
    if (self.body().y > arena.death_zone_y()) return true;

    // Walk the platform list to find the rightmost platform edge.
    float rightmost_x = -1.0e9f;
    for (const Platform& p : arena.platforms()) {
        const float edge = p.aabb.x + p.aabb.w;
        if (edge > rightmost_x) rightmost_x = edge;
    }
    // Off-stage horizontally if the body is past the right edge AND airborne.
    if (self.body().x + self.body().w > rightmost_x && !self.is_grounded()) {
        return true;
    }
    return false;
}

// --------------------------------------------------------------------
// Public entry point
// --------------------------------------------------------------------
void AiController::decide(const Fighter& self,
                          const Fighter& opp,
                          const Arena& arena,
                          double /*dt*/,
                          PlayerIntent& m,
                          CombatIntent& c) {
    // The current state handler fills the intents; per-tick transitions
    // are signalled by enter_state() inside the handler.
    switch (state_) {
        case State::Idle:     tick_idle    (self, opp, arena, m, c); break;
        case State::Approach: tick_approach(self, opp, arena, m, c); break;
        case State::Attack:   tick_attack  (self, opp, arena, m, c); break;
        case State::Retreat:  tick_retreat (self, opp, arena, m, c); break;
        case State::Recover:  tick_recover (self, opp, arena, m, c); break;
        case State::Defend:   tick_defend  (self, opp, arena, m, c); break;
    }
    ++state_ticks_;
}

// --------------------------------------------------------------------
// State handlers
//
// Each handler is responsible for:
//   1. Setting the intents for this tick.
//   2. Calling enter_state(NewState) at the moment the FSM should
//      transition. The transition always resets state_ticks_ to 0.
// --------------------------------------------------------------------
void AiController::tick_idle(const Fighter& self,
                             const Fighter& /*opp*/,
                             const Arena& arena,
                             PlayerIntent& /*m*/, CombatIntent& /*c*/) {
    // Off-stage interrupts Idle -> Recover (the bot is being launched).
    if (is_off_stage(self, arena)) {
        enter_state(State::Recover);
        return;
    }

    // Wait reaction_delay_ticks before committing to anything. While idle,
    // the intents are zero (do nothing). The bot may still be pushed by
    // external forces (knockback) during this window, which is the desired
    // Smash feel: a bot that just got hit shouldn't instantly counterattack.
    if (state_ticks_ >= difficulty_.reaction_delay_ticks) {
        // High damage -> Retreat, not Approach.
        if (self.damage_percent() >= difficulty_.defensive_threshold_percent) {
            enter_state(State::Retreat);
        } else {
            enter_state(State::Approach);
        }
    }
}

void AiController::tick_approach(const Fighter& self,
                                 const Fighter& opp,
                                 const Arena& arena,
                                 PlayerIntent& m, CombatIntent& c) {
    // Off-stage -> Recover.
    if (is_off_stage(self, arena)) {
        enter_state(State::Recover);
        return;
    }
    // Damage spiked above the threshold -> Retreat.
    if (self.damage_percent() >= difficulty_.defensive_threshold_percent) {
        enter_state(State::Retreat);
        return;
    }

    // Opponent is in windup (Attacking) within ftilt range -> Defend briefly.
    if (opp.state() == Fighter::State::Attacking
        && in_ftilt_range(self, opp)
        && rand_percent() < 50) {
        enter_state(State::Defend);
        return;
    }

    // In jab range: commit to a jab (most of the time) or ftilt if slightly
    // further. attack_frequency is the probability of committing once in
    // range; the rest of the time the bot keeps approaching.
    if (in_jab_range(self, opp) && self.is_grounded()) {
        if (rand_percent() < difficulty_.attack_frequency) {
            c.jab_pressed = true;
            enter_state(State::Attack);
            return;
        }
    } else if (in_ftilt_range(self, opp) && self.is_grounded()) {
        if (rand_percent() < difficulty_.attack_frequency) {
            c.ftilt_pressed = true;
            enter_state(State::Attack);
            return;
        }
    } else if (in_nair_range(self, opp)) {
        if (rand_percent() < difficulty_.attack_frequency) {
            c.nair_pressed = true;
            enter_state(State::Attack);
            return;
        }
    }

    // Walk toward the opponent. We face them by setting motion to the
    // closer horizontal direction. If they are above us and airborne,
    // chance to jump toward them.
    const float my_cx   = self.body().x + self.body().w * 0.5f;
    const float opp_cx  = opp.body().x + opp.body().w * 0.5f;
    const float my_cy   = self.body().y + self.body().h * 0.5f;
    const float opp_cy  = opp.body().y + opp.body().h * 0.5f;

    if (my_cx < opp_cx) {
        m.move_right = true;
    } else if (my_cx > opp_cx) {
        m.move_left = true;
    }

    // Jump aggression: if the opponent is above us and we're grounded, jump
    // some fraction of the time. If we're already in the air, we can't
    // jump (jumps are finite); skip the request and rely on the platform
    // layout to bring us back.
    if (self.is_grounded() && opp_cy < my_cy - 16.0f
        && rand_percent() < difficulty_.jump_aggression) {
        m.jump_pressed = true;
    }
}

void AiController::tick_attack(const Fighter& self,
                               const Fighter& opp,
                               const Arena& arena,
                               PlayerIntent& m, CombatIntent& /*c*/) {
    // Stay in Attack state for the full swing. While a swing is in progress
    // the Fighter ignores new combat intents, so we just stop issuing them.
    // (The CombatIntent fields stay false from the previous tick; the
    // Fighter won't see them as a new rising edge.)
    if (self.state() == Fighter::State::Attacking) {
        return;  // let the swing finish
    }

    // Off-stage after a swing (we were launched) -> Recover.
    if (is_off_stage(self, arena)) {
        enter_state(State::Recover);
        return;
    }

    // Swing finished. If the opponent is now very close, consider retreating
    // (we just got a hit in, give them room to whiff). Otherwise go back to
    // Idle (which after reaction_delay will re-Approach).
    if (in_jab_range(self, opp) && self.is_grounded()) {
        // 50% chance to retreat after a landed-in-range swing; otherwise
        // press advantage and re-Approach.
        if (rand_percent() < 50) {
            enter_state(State::Retreat);
            return;
        }
    }
    enter_state(State::Idle);
    m.move_left = false; m.move_right = false;
}

void AiController::tick_retreat(const Fighter& self,
                                const Fighter& opp,
                                const Arena& arena,
                                PlayerIntent& m, CombatIntent& c) {
    // Off-stage interrupts everything -> Recover.
    if (is_off_stage(self, arena)) {
        enter_state(State::Recover);
        return;
    }

    // Walk away from the opponent.
    const float my_cx  = self.body().x + self.body().w * 0.5f;
    const float opp_cx = opp.body().x + opp.body().w * 0.5f;
    if (my_cx < opp_cx) {
        m.move_left = true;
    } else {
        m.move_right = true;
    }

    // If we're at the edge of the playable area, stop walking so we don't
    // SDI off the side. The cheap version: stop moving once we're more than
    // 80 px past the centre of the stage, since the main floor is the only
    // safe ground and it has ends.
    const float stage_centre = (arena.spawn_x(0) + arena.spawn_x(1)) * 0.5f;
    if ((m.move_left  && my_cx < stage_centre - 80.0f)
     || (m.move_right && my_cx > stage_centre + 80.0f)) {
        m.move_left = false;
        m.move_right = false;
    }

    // After the retreat window, re-Idle.
    if (state_ticks_ >= ai::kRetreatDurationTicks) {
        enter_state(State::Idle);
    }
    (void)c; (void)opp;  // unused
}

void AiController::tick_recover(const Fighter& self,
                                const Fighter& opp,
                                const Arena& arena,
                                PlayerIntent& m, CombatIntent& /*c*/) {
    // Aim at the nearest platform centre. recovery_priority gates the
    // jump: at 100% we always jump the first frame we're airborne; at 0%
    // we never jump and the fighter SDs. Default 80.
    if (!self.is_grounded() && state_ticks_ == 0) {
        // Already airborne the moment we entered Recover; just steer.
    }
    if (self.is_grounded()) {
        // We made it back to a platform; transition to Idle.
        enter_state(State::Idle);
        return;
    }

    // Find the nearest platform centre (x and y).
    const float my_cx = self.body().x + self.body().w * 0.5f;
    float best_dx = 0.0f, best_dy = 0.0f;
    float best_d2 = 1.0e18f;
    for (const Platform& p : arena.platforms()) {
        const float px = p.aabb.x + p.aabb.w * 0.5f;
        const float py = p.aabb.y;  // top surface
        const float dx = px - my_cx;
        const float dy = py - self.body().y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_dx = dx;
            best_dy = dy;
        }
    }
    // Steer horizontally toward the platform.
    if (best_dx > 1.0f) m.move_right = true;
    else if (best_dx < -1.0f) m.move_left = true;

    // If we're below the platform and falling, jump to clear the edge.
    // recovery_priority is the per-tick probability of issuing the jump.
    if (best_dy > 0.0f && self.velocity().y >= -0.1f
        && rand_percent() < difficulty_.recovery_priority) {
        m.jump_pressed = true;
    }
    (void)opp;  // unused
}

void AiController::tick_defend(const Fighter& self,
                               const Fighter& opp,
                               const Arena& arena,
                               PlayerIntent& m, CombatIntent& c) {
    // Stand still and wait out the windup. After the defend window, go
    // back to Idle so the FSM re-evaluates.
    (void)self; (void)opp; (void)m; (void)c;
    if (is_off_stage(self, arena)) {
        enter_state(State::Recover);
        return;
    }
    if (state_ticks_ >= ai::kDefendDurationTicks) {
        enter_state(State::Idle);
    }
}

} // namespace ember
