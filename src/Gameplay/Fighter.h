#pragma once

#include "Core/Math.h"
#include "Core/Config.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/Attack.h"
#include "Gameplay/Sprite.h"
#include "Gameplay/Animation.h"
#include "Physics/Motion.h"
#include "Gameplay/CombatConfig.h"

#include <cstdint>
#include <memory>

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
        Guard,       // Rebirth: shielding on the ground
        Dodge,       // Rebirth: spot / directional dodge with invuln window
    };

    // Result of an incoming attack, reported to Match.
    enum class HitOutcome { Ignored, Hit, Blocked, Parried, Dodged };

    // 0 = P1, 1 = P2. Determines spawn position (via Arena) and palette.
    Fighter(int player_index, const Arena& arena);

    int player_index() const { return player_index_; }
    const Aabb& body() const { return body_; }
    bool is_grounded() const { return grounded_; }
    bool is_dead() const { return dead_; }
    bool is_invulnerable() const { return invuln_frames_ > 0; }
    int  stocks() const { return stocks_; }
    int  damage_percent() const { return damage_percent_; }
    int  jumps_remaining() const { return jumps_remaining_; }
    int  facing() const { return facing_; }                 // +1 or -1
    State state() const { return state_; }
    AttackId current_attack_id() const { return current_attack_id_; }
    int  attack_frame() const { return attack_frame_; }
    int  hitstun_frames() const { return hitstun_frames_; }
    int  invuln_frames() const { return invuln_frames_; }
    const Vec2& velocity() const { return velocity_; }     // read-only (M4 AI)
    // --- Rebirth Phase 2 accessors ---
    int  guard_frames() const { return guard_frames_; }
    int  shield() const { return shield_; }
    int  dodge_frame() const { return dodge_frame_; }
    int  dodge_cooldown() const { return dodge_cooldown_; }
    bool dodge_invulnerable() const {
        return state_ == State::Dodge && dodge_frame_ >= combat::kDodgeInvulnStart
            && dodge_frame_ < combat::kDodgeInvulnEnd;
    }
    bool in_parry_window() const {
        return state_ == State::Guard && guard_frames_ <= combat::kParryWindowFrames;
    }
    int  surge_meter() const { return surge_meter_; }
    int  surge_frames() const { return surge_frames_; }
    bool surge_active() const { return surge_frames_ > 0; }
    bool attack_has_hit() const { return attack_has_hit_; }
    int  parry_flash() const { return parry_flash_; }
    void mark_attack_hit() { attack_has_hit_ = true; }
    void add_surge(int amount);
    void stun_for(int frames);   // generic stun (parried attacker, shield break)
    void set_surge_meter_for_test(int m) { surge_meter_ = m; }

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

    // Applies the post-respawn invulnerability window. Match calls this right
    // after a respawn so the freshly-spawned fighter can't be combo'd
    // immediately by an opponent camping the spawn platform.
    void set_respawn_invuln_seconds(double seconds);

    // Wipes per-match progression (stocks, damage, attack/hitstun state) so
    // Match::reset() can start a rematch without touching the body/position
    // reset that respawn() already handled.
    void reset_for_rematch();

    // P15: rebuild the per-fighter sprite from the current color-
    // blind palette flag. Called by App when the user toggles the
    // COLOR BLIND setting so the visual change is immediate. The
    // sprite is otherwise built once in the ctor using the palette
    // that was current at construction time; without this hook the
    // setting only took effect on the NEXT rematch, which read as
    // "the toggle does nothing" during play.
    void rebuild_sprite_for_palette();

    // Test-only setters. Used by the headless harness to inject state
    // without going through combat or the death zone. Production code
    // should never call these; the public update/respawn path is the only
    // sanctioned way to mutate a fighter.
    void set_dead_for_test(bool dead)  { dead_ = dead; }
    void set_stocks_for_test(int stocks) { stocks_ = stocks; }
    void set_invuln_frames_for_test(int f) { invuln_frames_ = f; }

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

    // Rebirth Phase 2: full hit resolution including guard, parry, dodge.
    HitOutcome receive_hit(int damage, Vec2 knockback_velocity,
                           int hitstun_frames, int attacker_facing);

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
    int   jump_buffer_ticks_{0};
    int   coyote_ticks_{0};
    bool  dead_{false};
    int   damage_percent_{0};
    int   facing_{1};
    config::Color color_{};

    // M6-B: per-fighter procedural sprite with an animation strip.
    // Built from art::kPalette{P1,P2} at construction via
    // Sprite::build_animation_strip, which produces 6 frames covering
    // Idle (1) + Walk (2) + Jump (1) + Attack (1) + Hitstun (1). The
    // sprite's current frame is selected by anim_driver_ once per
    // render() call. The Renderer uploads whichever frame is current
    // into a cached SDL_Texture (see Renderer::draw_sprite).
    std::unique_ptr<Sprite> sprite_;

    // M6-B: animation driver. Picks the pose (Idle/Walk/Jump/Attack/
    // Hitstun) from (grounded, velocity, state), advances the per-pose
    // frame timer, and exposes the global frame index for the sprite.
    // Headless-testable via src/Gameplay/Animation.{h,cpp}.
    anim::Driver anim_driver_{};

    // Combat state
    State     state_           {State::Idle};
    AttackId  current_attack_id_{AttackId::None};
    int       attack_frame_    {0};
    bool      attack_has_hit_  {false};
    int       hitstun_frames_  {0};

    // M5 match-flow state
    int   stocks_{0};
    int   invuln_frames_{0};

    // Internal: attempt to start a new attack based on combat_intent and
    // current grounded state. Returns true if an attack was started.
    int   shield_{600};
    int   guard_frames_{0};
    int   dodge_frame_{0};
    int   dodge_cooldown_{0};
    int   dodge_dir_{0};
    int   surge_meter_{0};
    int   surge_frames_{0};
    int   parry_flash_{0};
    bool  up_special_used_{false};
    bool  air_dodge_used_{false};

    bool try_start_attack(const PlayerIntent& motion_intent,
                          const CombatIntent& combat_intent);

    // Internal: advance the attack clock; return to Idle when the swing ends.
    void advance_attack();
};

} // namespace ember
