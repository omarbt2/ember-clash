#include "Gameplay/Fighter.h"
#include "World/Arena.h"

#include <cassert>
#include <cmath>

using namespace ember;

int main() {
    Arena arena;

    // DI changes the launch trajectory only in airborne hitstun.
    Fighter neutral(0, arena);
    Fighter influenced(0, arena);
    const Aabb airborne{500.0f, 180.0f, Fighter::kWidth, Fighter::kHeight};
    for (Fighter* fighter : {&neutral, &influenced}) {
        fighter->body() = airborne;
        fighter->apply_hit(1, Vec2{4.0, -2.0}, 60);
        fighter->set_grounded_for_test(false);
    }
    PlayerIntent di{};
    di.move_right = true;
    di.up_held = true;
    influenced.update(di, CombatIntent{}, arena);
    neutral.update(PlayerIntent{}, CombatIntent{}, arena);
    assert(std::abs(influenced.velocity().x - neutral.velocity().x) > 1e-4);
    assert(influenced.velocity().y < neutral.velocity().y);
    assert(influenced.state() == Fighter::State::Hitstun);

    // Repeated DI cannot rotate farther than 15 degrees from the original
    // launch vector. Subtract the final gravity step to inspect the velocity
    // immediately after the DI adjustment.
    Fighter angle_limited(0, arena);
    angle_limited.body() = airborne;
    angle_limited.apply_hit(1, Vec2{4.0, -2.0}, 60);
    angle_limited.set_grounded_for_test(false);
    PlayerIntent steer_up_left{};
    steer_up_left.move_left = true;
    steer_up_left.up_held = true;
    constexpr double kPi = 3.14159265358979323846;
    const double launch_angle = std::atan2(-2.0, 4.0);
    const double max_di_angle = physics::kDirectionalInfluenceMaxRotationDegrees
                              * kPi / 180.0;
    for (int tick = 0; tick < 12; ++tick) {
        angle_limited.update(steer_up_left, CombatIntent{}, arena);
        const double x = angle_limited.velocity().x;
        const double y_before_gravity = angle_limited.velocity().y
                                      - physics::kGravity;
        const double angle = std::atan2(y_before_gravity, x);
        const double delta = std::remainder(angle - launch_angle, 2.0 * kPi);
        assert(std::abs(delta) <= max_di_angle + 1e-8);
    }

    // Holding down cannot make an upward launch point more toward the floor.
    Fighter floor_guarded(0, arena);
    Fighter floor_neutral(0, arena);
    for (Fighter* fighter : {&floor_guarded, &floor_neutral}) {
        fighter->body() = airborne;
        fighter->apply_hit(1, Vec2{4.0, -2.0}, 60);
        fighter->set_grounded_for_test(false);
    }
    PlayerIntent steer_down{};
    steer_down.fast_fall_held = true;
    floor_guarded.update(steer_down, CombatIntent{}, arena);
    floor_neutral.update(PlayerIntent{}, CombatIntent{}, arena);
    const double floor_guarded_y_before_gravity = floor_guarded.velocity().y
                                                - physics::kGravity;
    assert(floor_guarded_y_before_gravity <= 0.0);
    assert(floor_guarded.velocity().y > floor_neutral.velocity().y);

    // A shield edge during airborne hitstun is buffered and becomes a tech
    // on the landing tick. A dodge edge alone must not trigger a tech.
    Fighter tech(0, arena);
    tech.body() = Aabb{500.0f, 574.0f, Fighter::kWidth, Fighter::kHeight};
    tech.apply_hit(1, Vec2{0.0, 2.0}, 60);
    tech.set_grounded_for_test(false);
    CombatIntent tech_input{};
    tech_input.shield_pressed = true;
    tech.update(PlayerIntent{}, tech_input, arena);
    assert(tech.is_grounded());
    assert(tech.state() == Fighter::State::Tech);
    assert(tech.hitstun_frames() == 0);
    assert(tech.tech_buffer_ticks() == 0);

    // A shield press on the final hitstun tick still techs when the fighter
    // lands on that same tick (state expiry happens before collision).
    Fighter final_tick_tech(0, arena);
    const float floor_y = arena.platforms()[0].aabb.y;
    final_tick_tech.body() = Aabb{500.0f, floor_y - 7.9f - Fighter::kHeight,
                                  Fighter::kWidth, Fighter::kHeight};
    final_tick_tech.apply_hit(1, Vec2{0.0, 0.0}, combat::kMinHitstunFrames);
    final_tick_tech.set_grounded_for_test(false);
    for (int tick = 0; tick < combat::kMinHitstunFrames - 1; ++tick) {
        final_tick_tech.update(PlayerIntent{}, CombatIntent{}, arena);
    }
    final_tick_tech.update(PlayerIntent{}, tech_input, arena);
    assert(final_tick_tech.is_grounded());
    assert(final_tick_tech.state() == Fighter::State::Tech);

    // The buffered input also survives hitstun expiring shortly before the
    // landing itself, as long as the eight-tick buffer has not expired.
    Fighter post_hitstun_tech(0, arena);
    post_hitstun_tech.body() = Aabb{500.0f, floor_y - 18.0f - Fighter::kHeight,
                                   Fighter::kWidth, Fighter::kHeight};
    post_hitstun_tech.apply_hit(1, Vec2{0.0, 0.0}, combat::kMinHitstunFrames);
    post_hitstun_tech.set_grounded_for_test(false);
    for (int tick = 0; tick < combat::kMinHitstunFrames - 1; ++tick) {
        post_hitstun_tech.update(PlayerIntent{}, CombatIntent{}, arena);
    }
    post_hitstun_tech.update(PlayerIntent{}, tech_input, arena);
    assert(post_hitstun_tech.state() == Fighter::State::Idle);
    assert(post_hitstun_tech.tech_buffer_ticks() > 0);
    for (int tick = 0; tick < 4 && !post_hitstun_tech.is_grounded(); ++tick) {
        post_hitstun_tech.update(PlayerIntent{}, CombatIntent{}, arena);
    }
    assert(post_hitstun_tech.is_grounded());
    assert(post_hitstun_tech.state() == Fighter::State::Tech);

    Fighter tech_window(0, arena);
    tech_window.body() = Aabb{500.0f, 180.0f, Fighter::kWidth, Fighter::kHeight};
    tech_window.apply_hit(1, Vec2{0.0, 0.5}, 60);
    tech_window.set_grounded_for_test(false);
    tech_window.update(PlayerIntent{}, tech_input, arena);
    assert(tech_window.tech_buffer_ticks() == physics::kTechInputBufferTicks);
    for (int tick = 0; tick < physics::kTechInputBufferTicks - 1; ++tick) {
        tech_window.update(PlayerIntent{}, CombatIntent{}, arena);
    }
    assert(tech_window.tech_buffer_ticks() == 1);

    Fighter dodge_only(0, arena);
    dodge_only.body() = Aabb{500.0f, 574.0f, Fighter::kWidth, Fighter::kHeight};
    dodge_only.apply_hit(1, Vec2{0.0, 2.0}, 60);
    dodge_only.set_grounded_for_test(false);
    CombatIntent dodge_input{};
    dodge_input.dodge_pressed = true;
    dodge_only.update(PlayerIntent{}, dodge_input, arena);
    assert(dodge_only.state() == Fighter::State::Hitstun);

    // Falling beside an elevated platform catches its ledge. Up climbs onto
    // the platform; jump launches back inward and starts the regrab lockout.
    const Aabb& platform = arena.platforms()[0].aabb;
    Fighter ledge(0, arena);
    ledge.body() = Aabb{platform.x - Fighter::kWidth - 10.0f,
                        platform.y + 20.0f,
                        Fighter::kWidth, Fighter::kHeight};
    ledge.velocity() = Vec2{0.0, 4.0};
    ledge.set_grounded_for_test(false);
    ledge.update(PlayerIntent{}, CombatIntent{}, arena);
    assert(ledge.is_ledge_hanging());
    assert(ledge.is_invulnerable());
    assert(ledge.invuln_frames() == physics::kLedgeInvulnerabilityTicks);
    assert(ledge.ledge_invulnerability_used());
    assert(std::abs(ledge.velocity().x) < 0.001);
    PlayerIntent climb{};
    climb.up_held = true;
    ledge.update(climb, CombatIntent{}, arena);
    assert(ledge.state() == Fighter::State::Idle);
    assert(ledge.is_grounded());
    assert(std::abs(ledge.body().bottom() - platform.y) < 0.01f);
    assert(!ledge.ledge_invulnerability_used());

    // Ledge grabs require facing toward the stage. P2 starts facing left, so
    // it cannot catch the left edge while looking away from it.
    Fighter facing_away(1, arena);
    facing_away.body() = Aabb{platform.x - Fighter::kWidth - 10.0f,
                              platform.y + 20.0f,
                              Fighter::kWidth, Fighter::kHeight};
    facing_away.velocity() = Vec2{0.0, 4.0};
    facing_away.set_grounded_for_test(false);
    facing_away.update(PlayerIntent{}, CombatIntent{}, arena);
    assert(!facing_away.is_ledge_hanging());

    Fighter jump(0, arena);
    jump.body() = Aabb{platform.x - Fighter::kWidth - 10.0f,
                       platform.y + 20.0f,
                       Fighter::kWidth, Fighter::kHeight};
    jump.velocity() = Vec2{0.0, 4.0};
    jump.set_grounded_for_test(false);
    jump.update(PlayerIntent{}, CombatIntent{}, arena);
    assert(jump.is_ledge_hanging());
    PlayerIntent ledge_jump{};
    ledge_jump.jump_pressed = true;
    jump.update(ledge_jump, CombatIntent{}, arena);
    assert(jump.state() == Fighter::State::Idle);
    assert(jump.velocity().y < 0.0);
    assert(jump.ledge_lockout_ticks() == physics::kLedgeRegrabLockoutTicks);
    assert(jump.ledge_invulnerability_used());
    const int invulnerability_before_regrab = jump.invuln_frames();
    for (int tick = 0; tick < physics::kLedgeRegrabLockoutTicks; ++tick) {
        jump.update(PlayerIntent{}, CombatIntent{}, arena);
    }
    jump.body() = Aabb{platform.x - Fighter::kWidth - 10.0f,
                       platform.y + 20.0f,
                       Fighter::kWidth, Fighter::kHeight};
    jump.velocity() = Vec2{0.0, 4.0};
    jump.set_grounded_for_test(false);
    jump.update(PlayerIntent{}, CombatIntent{}, arena);
    assert(jump.is_ledge_hanging());
    assert(jump.ledge_invulnerability_used());
    assert(jump.invuln_frames() < invulnerability_before_regrab);

    return 0;
}
