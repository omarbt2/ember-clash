#pragma once

#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "World/Arena.h"

#include <array>
#include <memory>

namespace ember {

class AiController;
class InputManager;
class Renderer;

// Owns the static arena and the two fighters; per-frame orchestration lives
// here so App can stay a thin driver. M5 adds a match-phase state machine
// (Fighting -> MatchOver) and the per-fighter respawn timers, hit sparks,
// and stock counters that the M5 HUD reads.
class Match {
public:
    static constexpr int kFighterCount = 2;

    // High-level match state. Fighting is the normal play loop. MatchOver
    // is entered when one fighter's stocks_remaining hits 0; the player
    // can press R (handled in App) to reset the match back to Fighting.
    enum class Phase { Fighting, MatchOver };

    Match();
    ~Match();

    Match(const Match&)            = delete;
    Match& operator=(const Match&) = delete;

    // One match tick. `input` provides keyboard state; dt is the elapsed
    // seconds since the last update. Fighters with an attached AiController
    // are driven by the AI instead of InputMap.
    void update(const InputManager& input, double dt);

    // Draws the arena + fighters + M5 HUD in render-space coordinates.
    void render(Renderer& renderer) const;

    // For tests and the rematch flow. Resets fighters, the arena, the AI
    // controllers, and the match-flow state.
    void reset();

    // Per-fighter AI control. Setting a controller for an index makes that
    // fighter AI-driven; the default (no controller) means human-controlled.
    void set_ai(int fighter_index, std::unique_ptr<AiController> controller);
    bool is_ai(int fighter_index) const;

    // True when a hit connected this frame. The HUD reads this through the
    // internal accessor; tests read it via last_hit_position_for_test().
    struct HitEvent {
        float x;
        float y;
        bool valid;
    };

    const Arena& arena() const { return arena_; }
    const Fighter& fighter(int index) const;
    // Non-const fighter accessor. Used by the headless test harness to
    // inject damage / position / state without going through combat. Not
    // used in the production update path.
    Fighter&       fighter(int index)       { return *fighters_[index]; }
    const AiController* ai(int fighter_index) const;

    // M5 accessors for the HUD and tests.
    Phase phase() const { return phase_; }
    // Seconds elapsed since the match ended. 0 while Fighting.
    double ko_banner_timer_seconds() const { return ko_banner_timer_; }
    // Position of the most recent hit (for the hit-spark HUD overlay).
    HitEvent last_hit_position_for_test() const { return last_hit_; }

    // M6-C: combat feel state. All read by App to drive the renderer
    // (shake offset) and the test harness to verify the feel is firing.
    // True while the match is in its hit-stop pause (motion + AI frozen,
    // hit-spark still ages).
    bool is_in_hit_stop() const { return hit_stop_frames_ > 0; }
    // Frames remaining in the current hit-stop window. 0 when not stopped.
    int  hit_stop_frames() const { return hit_stop_frames_; }
    // True while the post-KO screen shake is still decaying.
    bool is_in_ko_shake() const { return ko_shake_frames_ > 0; }
    // Frames remaining in the post-KO shake. 0 when fully decayed.
    int  ko_shake_frames() const { return ko_shake_frames_; }
    // Current shake offset in render pixels. Magnitude decays linearly to
    // 0 as the shake ages. x and y are signed; App forwards this to the
    // renderer's view offset.
    float ko_shake_offset_x() const { return ko_shake_offset_x_; }
    float ko_shake_offset_y() const { return ko_shake_offset_y_; }

    // Test-only convenience: respawn a specific fighter immediately and
    // grant the standard post-respawn invuln window. Production code uses
    // the death-zone path; this is for the harness so it can set up
    // specific scenarios without waiting for natural deaths.
    void respawn_fighter_for_test(int fighter_index) {
        if (fighter_index < 0 || fighter_index >= kFighterCount) return;
        respawn_timer_[fighter_index] = -1.0;
        fighters_[fighter_index]->respawn(arena_);
        fighters_[fighter_index]->set_respawn_invuln_seconds(
            combat::kRespawnInvulnSeconds);
    }

private:
    Arena arena_;
    std::array<std::unique_ptr<Fighter>, kFighterCount> fighters_;
    std::array<std::unique_ptr<AiController>, kFighterCount> ai_;

    // M5 match-flow state. Per-fighter respawn timers are stored in seconds
    // (count down to 0) so the test harness can read them directly without
    // doing frame-rate math. A negative value means "the fighter is alive
    // and respawn timing isn't active."
    std::array<double, kFighterCount> respawn_timer_;
    double ko_banner_timer_{0.0};
    Phase  phase_{Phase::Fighting};
    HitEvent last_hit_{0.0f, 0.0f, false};
    int  hit_spark_age_{0};  // frames remaining for the active spark

    // M6-C: combat feel state. Set on hit / KO, ticked each update, and
    // consumed by App to drive the renderer.
    int   hit_stop_frames_{0};     // > 0 means motion + AI are frozen
    int   ko_shake_frames_{0};     // > 0 means the shake is still playing
    float ko_shake_offset_x_{0.0f};// current shake offset, render pixels
    float ko_shake_offset_y_{0.0f};
};

} // namespace ember
