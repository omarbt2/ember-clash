// -----------------------------------------------------------------------------
// Emscripten-only entry point. This translation unit is compiled only when
// CMake is configuring a build with the Emscripten toolchain. The desktop
// entry point in src/main.cpp (int SDL_main) is not compiled in that case.
//
// Why a separate file: the desktop and the browser have different ideas of
// "the main loop." Desktop applications block in a while(running) loop. The
// browser cannot block - it must yield back to the event loop after every
// frame. SDL3's recommended pattern is to call emscripten_set_main_loop()
// with a callback, and the callback drives a single App::step() per frame.
//
// The whole adaptation is this file. App::step() is the only per-frame entry
// the rest of the codebase knows about - the desktop App::run() just calls
// step() in a loop. So no gameplay code changes for the web build.
// -----------------------------------------------------------------------------

#include "Core/App.h"
#include "Core/Config.h"
#include "Core/Logger.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstdlib>

namespace {

// Fixed-timestep main loop. The browser's rAF cadence varies (typically
// 60Hz, sometimes 120Hz); we clamp the visible delta to the configured
// frame budget so gameplay speed matches the desktop build.
ember::App* g_app = nullptr;
double      g_last_ticks_seconds = 0.0;

void main_loop_tick(void* /*userdata*/) {
    if (!g_app) {
        return;
    }

    // emscripten_get_now() returns wall-clock milliseconds. Convert to
    // seconds and clamp to the same cap the desktop Clock applies.
    const double now = emscripten_get_now() * 1e-3;
    double dt = now - g_last_ticks_seconds;
    g_last_ticks_seconds = now;

    constexpr double kMaxFrameSeconds = 1.0 / 15.0;  // matches config::kMaxDeltaSeconds
    if (dt < 0.0)   dt = 0.0;
    if (dt > kMaxFrameSeconds) dt = kMaxFrameSeconds;

    g_app->step(dt);
}

} // namespace

// On Emscripten we provide a plain int main(void) - SDL3's SDL_main macro
// is not used in the web build (the browser does not have a command line).
// We also disable SDL_main by NOT including <SDL3/SDL_main.h>.
int main(void) {
    ember::Logger::info("Ember Clash (web) starting...");

    ember::App app;
    if (!app.initialize()) {
        ember::Logger::error("App::initialize() failed on web entry point.");
        return 1;
    }
    g_app = &app;
    g_last_ticks_seconds = emscripten_get_now() * 1e-3;

    // Hand control to the browser. 0 = use rAF, 1 = simulate infinite loop
    // (don't use simulate_infinite_loop with the SDL3 Emscripten backend;
    // it expects to return and be re-invoked by rAF).
    emscripten_set_main_loop(&main_loop_tick, 0, /*simulate_infinite_loop=*/0);

    // emscripten_set_main_loop never returns under simulate_infinite_loop=0
    // in normal operation; this return is only reached on a runtime abort.
    return 0;
}
