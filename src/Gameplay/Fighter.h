#pragma once

#include "Core/Math.h"
#include "Core/Config.h"
#include "Gameplay/Attack.h"
#include "Physics/Motion.h"

#include <cstdint>

namespace ember {

class Arena;
class Renderer;

// One fighter on the stage. Holds its own physics + combat state and exposes
// a single update() entry point. All game-facing mutations go through
// update() so the per-tick story is consistent.
class Fighter {
public:
    static constexpr float kWidth  = 48.0f;
    static constexpr float kHeight = 64.0f;

    enum class State {
        Idle,        // default: motion is fully driven by input
        Attacking,   // startup / active / recovery, frame is attack_frame_
        Hitstun,     // locked out; velocity came from a hit
        Dead,        // off-stage; Match handles the respawn transition
    };

    // 0 = P1, 1 = P2. Determines spawn position (via Arena) and palette.
    Fighter(int player_index, const Arena& arena);

    int player_index() const { return player_index_; }
    const Aabb& body() const { return body_; }
    bool is_grounded() const { return grounded_; }
    bool is_dead() const { return dead_; }
    int  damage_percent() const { return damage_percent_; }
    int  facing() const { return facing_; }                 // +1 or -1
    State state() const { return state_; }
    AttackId current_attack_id() const { return current_attack_id_; }
    int  attack_frame() const { return attack_frame_; }
    int  hitstun_frames() const { return hitstun_frames_; }

    // Mutable accessors are intentionally not part of the public surface;
    // they're for the headless combat test and for engine glue that needs
    // to teleport a fighter (e.g. cinematic intros, debug overlays). The
    // gameplay update path never uses them.
    Aabb& body() { return body_; }
    Vec2& velocity() { return velocity_; }

    // World-space hitbox for the swing currently in the active window, or
    // an empty AABB if no attack is active. Match uses this for hit tests.
    Aabb active_hitbox() const;

    // Resets the fighter to full health at its designated spawn position.
    void respawn(const Arena& arena);

    // One simulation tick. motion_intent drives walk / jump / fast-fall;
    // combat_intent drives attack initiation. Both are sampled by InputMap
    // and are platform-agnostic. dt is passed through to the motion step
    // (in seconds) and is also used to scale attack-frame timing in Match.
    void update(const PlayerIntent& motion_intent,
                const CombatIntent& combat_intent,
                const Arena& arena,
                double dt);

    // Applies a hit from an external attacker. Sets damage, knockback
    // velocity, hitstun, and transitions to Hitstun. No effect on a dead
    // fighter. Knockback vector is in world space (caller has already
    // applied the attacker's facing).
    void apply_hit(int damage,
                   Vec2 knockback_velocity,
                   int hitstun_frames);

    // Renders the fighter using its assigned palette color. When in an
    // active attack frame, also draws the active hitbox outline in red so
    // combat is visible during M3 development. The hitbox overlay is a
    // debug aid; remove it when M6 ships real sprites.
    void render(Renderer& renderer) const;

private:
    int   player_index_{0};
    Aabb  body_{};
    Vec2  velocity_{};
    bool  grounded_{false};
    int   jumps_remaining_{2};
    bool  dead_{false};
    int   damage_percent_{0};
    int   facing_{1};
    config::Color color_{};

    // Combat state
    State     state_           {State::Idle};
    AttackId  current_attack_id_{AttackId::None};
    int       attack_frame_    {0};
    bool      attack_has_hit_  {false};
    int       hitstun_frames_  {0};

    // Internal: attempt to start a new attack based on combat_intent and
    // current grounded state. Returns true if an attack was started.
    bool try_start_attack(const CombatIntent& combat_intent);

    // Internal: advance the attack clock; return to Idle when the swing ends.
    void advance_attack();
};

} // namespace ember
