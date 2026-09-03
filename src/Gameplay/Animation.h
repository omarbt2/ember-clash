#pragma once

// =============================================================================
// Animation - the M6-B animation system.
//
// The previous milestone (M6-A) shipped a single-frame Sprite and no
// animation driver; the fighter body was one pose, always. M6-B adds
// a small frame-driven animation system that maps a fighter's current
// gameplay state (grounded, moving, attacking, hitstun'd, dead) to a
// concrete frame in the Sprite's frame strip.
//
// The driver is intentionally tiny and headless-testable. It owns no
// time of its own - the caller advances the timer by calling
// `tick()` once per simulation step (i.e. once per `App::step`).
// The driver holds:
//   * a chosen pose (which block of frames in the strip),
//   * a chosen frame index inside the pose,
//   * a counter that decides when to advance the frame.
//
// The driver never edits the Sprite. The fighter (or anyone else) is
// expected to call `sprite.set_current_frame(driver.frame())` before
// blitting. This separation lets the driver be unit-tested without
// SDL or a Renderer.
// =============================================================================

#include "Gameplay/Sprite.h"

#include <cstdint>

namespace ember::anim {

// How long one frame of the current pose shows before the driver
// advances to the next frame. In simulation ticks, not seconds - the
// game runs at a fixed 60 Hz, so "1 tick" == 1/60 s, and these
// constants just count ticks.
struct Timing {
    int idle_ticks_per_frame    = 30;  // ~0.5s per idle "breath"
    int walk_ticks_per_frame    = 6;   // ~0.1s per walk leg swing
    int jump_ticks_per_frame    = 12;  // ~0.2s on the jump pose
    int attack_ticks_per_frame  = 3;   // ~50ms per attack frame
    int hitstun_ticks_per_frame = 4;   // ~67ms per hitstun flicker
};

// A small POD that describes the current game-side state, in the
// animation driver's vocabulary. Fighter::render() builds one of these
// per frame and hands it to the driver. Keeping it in a separate type
// (instead of taking a Fighter& directly) means the driver is
// testable without instantiating a Fighter.
struct Context {
    bool   grounded       = true;
    bool   dead           = false;
    bool   attacking      = false;
    bool   in_hitstun     = false;
    float  horizontal_speed = 0.0f;  // px per tick, |vx|
};

// A single instance per fighter. The driver is a value type - no
// inheritance, no virtuals, no heap. Constructed with default
// timing. tick() advances the per-pose frame timer and may change the
// pose or advance the frame.
class Driver {
public:
    explicit Driver(Timing timing = Timing{}) : timing_(timing) {}

    // Pose currently selected. Useful for tests and for any debug
    // overlay that wants to show "current pose: WALK".
    Sprite::AnimationKind pose() const { return pose_; }

    // Frame index inside the current pose (0..frames_per_pose-1).
    // The Fighter calls sprite.set_current_frame(global_frame_index)
    // where global_frame_index == first_frame_of(pose) + frame_in_pose.
    int frame_in_pose() const { return frame_in_pose_; }

    // The global frame index in the Sprite's frame strip. Combines
    // pose and frame_in_pose; this is the value the Fighter feeds to
    // sprite.set_current_frame().
    int global_frame() const {
        return Sprite::first_frame_of(pose_) + frame_in_pose_;
    }

    // How many frames the current pose has. Walk = 2, the rest = 1.
    // Re-derived from the static table in Sprite so the driver can't
    // disagree with the Sprite.
    int frames_in_pose() const;

    // How many ticks left before the driver advances to the next
    // frame of the current pose. Tests can assert on this to confirm
    // the timing constants are honoured.
    int ticks_until_next_frame() const { return ticks_until_next_; }

    // Duration of one frame of the given pose under this driver's
    // timing. Public so the .cpp can use it from both the
    // pose-change branch and the per-tick-advance branch without
    // duplicating the switch.
    int ticks_per_frame_for(Sprite::AnimationKind pose) const;

    // Reset to a known state: pose Idle, frame 0, timer full. Called
    // by Fighter::respawn so a respawned fighter doesn't briefly
    // flash a walk or hitstun pose.
    void reset_to_idle() {
        pose_            = Sprite::AnimationKind::Idle;
        frame_in_pose_   = 0;
        ticks_until_next_ = timing_.idle_ticks_per_frame;
    }

    // Advance one simulation tick and update pose + frame as needed.
    // Idempotent within a single tick: calling tick() twice with the
    // same Context advances time twice (so the test can drive the
    // driver forward any number of ticks).
    void tick(const Context& ctx);

    // Allow tests to inject a different timing profile.
    void set_timing(Timing t) { timing_ = t; }

private:
    Timing timing_{};
    Sprite::AnimationKind pose_{Sprite::AnimationKind::Idle};
    int frame_in_pose_{0};
    int ticks_until_next_{30};
};

} // namespace ember::anim
