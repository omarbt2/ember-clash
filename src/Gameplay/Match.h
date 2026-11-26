#pragma once

#include "Core/Config.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/TrainingRecorder.h"
#include "World/Arena.h"

#include <array>
#include <cstddef>
#include <cstdint>
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

    // One fixed simulation tick. All logical timers are integer tick counts;
    // render-frame timing is handled only by App's fixed-step accumulator.
    void update(const InputManager& input);

    // Draws the arena + fighters + M5 HUD in render-space coordinates.
    void render(Renderer& renderer) const;

    // For tests and the rematch flow. Resets fighters, the arena, the AI
    // controllers, and the match-flow state.
    void reset();
    void set_arena(Arena::Id id) { arena_id_ = id; reset(); }
    Arena::Id arena_id() const { return arena_id_; }

    // Per-fighter AI control. Setting a controller for an index makes that
    // fighter AI-driven; the default (no controller) means human-controlled.
    void set_ai(int fighter_index, std::unique_ptr<AiController> controller);
    // Keeps the common detach call usable without requiring callers to
    // include the complete AiController definition.
    void set_ai(int fighter_index, std::nullptr_t);
    bool is_ai(int fighter_index) const;

    // P6 training slice: record and replay P1's platform-independent intents.
    // Playback consumes the same Fighter::update path as live input.
    void start_training_recording() { training_recorder_.start_recording(); }
    void stop_training_recording() { training_recorder_.stop_recording(); }
    void play_training_recording() { training_recorder_.start_playback(); }
    void clear_training_recording() { training_recorder_.clear(); }
    bool training_is_recording() const { return training_recorder_.recording(); }
    bool training_is_playing() const { return training_recorder_.playing(); }
    bool training_recording_overflowed() const { return training_recorder_.overflowed(); }
    std::size_t training_recorded_ticks() const { return training_recorder_.size(); }

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
    // P15: test-only mutable accessor so the test can set
    // test seams (e.g. set_skip_grace_for_test). Production
    // code uses the const ai() overload so it cannot
    // mutate AI state.
    AiController*       ai_mutable(int fighter_index);

    // M5 accessors for the HUD and tests.
    Phase phase() const { return phase_; }
    // Integer simulation ticks since the match ended. 0 while Fighting.
    int ko_banner_timer_ticks() const { return ko_banner_ticks_; }
    int respawn_timer_ticks_for_test(int fighter_index) const {
        if (fighter_index < 0 || fighter_index >= kFighterCount) return -1;
        return respawn_timer_[fighter_index];
    }
    // Position of the most recent hit (for the hit-spark HUD overlay).
    HitEvent last_hit_position_for_test() const { return last_hit_; }

    // P15-playability: intro sequence. match_startup_ticks() is the
    // frame counter since the match began. 0 = freshly reset;
    // 0..kIntroBannerReadyFrames-1 = "READY..."; next
    // kIntroBannerFightFrames = "FIGHT!"; afterwards the match is
    // live and the Hud blanks the banner. is_intro_active() returns
    // true while the banner is on (i.e. physics is frozen). The
    // banner_state() returns 0/1/2 for the Hud to use as an index.
    int  match_startup_ticks() const { return match_startup_ticks_; }
    int  intro_banner_state()  const { return intro_banner_state_; }
    bool is_intro_active()     const {
        return intro_banner_state_ != 2;
    }
    // P15: live-play age, in frames. Starts at 0 the FIRST frame
    // after the intro sequence ends (banner state transitions to
    // 2) and increments by 1 every subsequent update. The AI
    // uses this to decide whether the post-FIGHT grace window
    // is still active. Note: match_startup_ticks_ is frozen
    // at the end of the intro, so it cannot be used as a
    // monotonic age counter for post-intro decisions; live_ticks_
    // is the correct value for anything that needs to count
    // "frames since play started".
    int  live_ticks() const { return live_ticks_; }

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
    std::uint32_t shake_rng_state_for_test() const { return shake_rng_state_; }
    void set_shake_rng_state_for_test(std::uint32_t state) {
        shake_rng_state_ = state == 0 ? kDefaultShakeRngSeed : state;
    }
    void set_hit_stop_frames_for_test(int frames) {
        hit_stop_frames_ = frames > 0 ? frames : 0;
    }

    // M11: simulation-state fingerprint. Folds exact IEEE-754 bit patterns
    // plus fighter resources/timers, AI runtime + tuning + RNG, training
    // playback, match flow and hit feedback state. Identical simulations
    // within the same build/configuration should hash identically. This is a
    // desync diagnostic, not a promise of cross-platform FP determinism.
    std::uint64_t state_hash() const;

    // Test-only convenience: respawn a specific fighter immediately and
    // grant the standard post-respawn invuln window. Production code uses
    // the death-zone path; this is for the harness so it can set up
    // specific scenarios without waiting for natural deaths.
    void respawn_fighter_for_test(int fighter_index) {
        if (fighter_index < 0 || fighter_index >= kFighterCount) return;
        respawn_timer_[fighter_index] = -1;
        fighters_[fighter_index]->respawn(arena_);
        fighters_[fighter_index]->set_respawn_invuln_ticks(
            combat::kRespawnInvulnTicks);
    }

    // P15: test-only convenience. Forces the intro sequence to end
    // immediately. The next update() will see intro_banner_state_ == 2
    // and run live play on the first call, so the existing M5 / M6-C /
    // AI harnesses (which were written before the intro existed and
    // assume Match::update runs the gameplay on tick 0) keep their
    // semantics. Production code never calls this; the only legitimate
    // path to live play is the real intro gate. live_ticks_ is set to
    // 0 so any post-intro gate (e.g. the AI startup grace) is measured
    // from the call site forward, exactly as if the intro had just
    // finished naturally.
    void force_live_state_for_test() {
        intro_banner_state_ = 2;
        match_startup_ticks_ = combat::kIntroBannerReadyFrames
                             + combat::kIntroBannerFightFrames;
        live_ticks_ = 0;
    }

private:
    static constexpr std::uint32_t kDefaultShakeRngSeed = 0x6D2B79F5u;

    Arena arena_;
    Arena::Id arena_id_{Arena::Id::TrainingRing};
    std::array<std::unique_ptr<Fighter>, kFighterCount> fighters_;
    std::array<std::unique_ptr<AiController>, kFighterCount> ai_;
    training::Recorder training_recorder_{};

    // M5 match-flow timers are integer ticks. A negative respawn value means
    // the fighter is alive and no respawn is pending.
    std::array<int, kFighterCount> respawn_timer_;
    int ko_banner_ticks_{0};
    Phase  phase_{Phase::Fighting};
    HitEvent last_hit_{0.0f, 0.0f, false};
    int  hit_spark_age_{0};  // frames remaining for the active spark

    // P15-playability: match intro sequence. The match does NOT start
    // in live play - it begins in a "READY..." / "FIGHT!" countdown
    // (combat::kIntroBannerReadyFrames + kIntroBannerFightFrames total).
    // During this window Match::update is a no-op for the gameplay
    // (no AI, no motion, no hit resolution), and the Hud draws the
    // banner. The 0..kIntroBannerReadyFrames-1 range shows "READY...";
    // kIntroBannerReadyFrames..(kIntroBannerReadyFrames +
    // kIntroBannerFightFrames)-1 shows "FIGHT!"; after that the
    // counter continues to increment so the Hud can blank the banner
    // and the AI can begin its 1.5s post-FIGHT startup grace
    // (kAiStartupGraceTicks). The counter resets to 0 on every
    // match->reset() so a rematch re-runs the intro.
    int  match_startup_ticks_{0};
    // Cached snapshot of the intro banner's logical state so the Hud
    // can decide what to draw without re-deriving the threshold math
    // every frame. 0 = "READY...", 1 = "FIGHT!", 2 = live (no banner).
    int  intro_banner_state_{0};
    // P15: live-play age in frames. Starts at 0 on the first frame
    // AFTER the intro sequence (i.e. when intro_banner_state_ first
    // becomes 2) and increments by 1 every subsequent update. Unlike
    // match_startup_ticks_ (which is frozen at the end of the intro)
    // this is a monotonic counter for the duration of live play.
    // The AI's kAiStartupGraceTicks gate uses this value, not
    // match_startup_ticks_.
    int  live_ticks_{0};

    // M6-C: combat feel state. Set on hit / KO, ticked each update, and
    // consumed by App to drive the renderer.
    int   hit_stop_frames_{0};     // > 0 means motion + AI are frozen
    int   ko_shake_frames_{0};     // > 0 means the shake is still playing
    float ko_shake_offset_x_{0.0f};// current shake offset, render pixels
    float ko_shake_offset_y_{0.0f};
    std::uint32_t shake_rng_state_{kDefaultShakeRngSeed};
};

} // namespace ember
