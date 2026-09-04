// -----------------------------------------------------------------------------
// Ember Clash P17 headless test harness.
//
// Purpose
//   P17 ("Human Playability Recovery + Final System Completion") exposed a
//   single P0 root cause for the "game feels too fast, character hard to
//   see" complaint: the main loop ran at whatever rate the desktop was
//   willing to spin (often 200+ FPS on a fast machine) while every per-
//   tick gameplay constant assumes 60 Hz. This test pins the two
//   invariants that future refactors must keep:
//
//     1. Clock::fixed_step_seconds() == 1/60 s (the dt the App::run
//        accumulator feeds into update()).
//     2. The system has a render-scale knob that can be set above 1.0
//        without re-creating the renderer (the camera uses it to zoom
//        the match up to 1.3x while leaving the HUD at 1.0x).
//
// Both assertions are zero-side-effect: the test never calls SDL_Init,
// never opens a window, and never constructs a Window. The only
// dependency is the Clock and the Settings defaults, both of which are
// safe to instantiate headlessly.
//
// What this test guards against
//   * Clock::fixed_step_seconds() accidentally returning a value
//     different from 1/60 (would silently break every per-tick
//     tuning constant in PhysicsConfig.h, CombatConfig.h, etc.).
//   * Settings::render_scale being removed (would break the camera
//     zoom + every user-facing resolution setting).
//
// Layout follows the M8 pattern: tiny main(), static_asserts where
// the assertion is a compile-time property, runtime check() calls for
// numeric properties.
// -----------------------------------------------------------------------------

#include "Core/Config.h"
#include "Core/Settings.h"
#include "Core/Time.h"

#include <SDL3/SDL_timer.h>

#include <cmath>
#include <cstdio>
#include <type_traits>

namespace {

int g_total = 0;
int g_passed = 0;

void check(const char* name, bool cond) {
    ++g_total;
    if (cond) {
        ++g_passed;
        std::printf("[PASS] %s\n", name);
    } else {
        std::printf("[FAIL] %s\n", name);
    }
}

bool nearly_equal(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

} // namespace

int main() {
    // 1) Clock::fixed_step_seconds() must be 1 / kTargetFps. This is
    //    the dt the App::run accumulator forwards to update(); if
    //    it ever drifts, every per-tick constant in the game
    //    (kWalkSpeed, kGravity, kHitStopFrames, kIntroBannerReady
    //    Frames, etc.) will also drift with it.
    {
        ember::Clock clock;
        const double step = clock.fixed_step_seconds();
        const double expected =
            1.0 / static_cast<double>(ember::config::kTargetFps);
        check("Clock::fixed_step_seconds() == 1 / kTargetFps",
              nearly_equal(step, expected));
        check("kTargetFps is 60 (the design contract for tuning)",
              ember::config::kTargetFps == 60);
    }

    // 2) The render-scale knob must be a positive int the user can
    //    change. The camera's 1.3x match zoom multiplies on top of
    //    this, so removing the knob would also remove the zoom and
    //    regress the "character too small" complaint.
    {
        ember::Settings s;
        check("Settings::render_scale defaults to a positive int",
              s.render_scale >= 1);
        check("Settings::render_scale is mutable",
              std::is_same_v<decltype(s.render_scale), int>);
    }

    // 3) Headless double-check: Clock::delta_seconds() returns a
    //    non-negative, finite value. We don't enforce an exact
    //    value (the first update() of any clock has a degenerate
    //    delta of 0); we just need it not to be NaN or negative.
    {
        ember::Clock clock;
        clock.update();
        const double d = clock.delta_seconds();
        check("Clock::delta_seconds() is finite after first update()",
              std::isfinite(d));
        check("Clock::delta_seconds() is non-negative",
              d >= 0.0);
    }

    std::printf("\nP17: %d / %d checks passed\n", g_passed, g_total);
    return (g_passed == g_total) ? 0 : 1;
}
