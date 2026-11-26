// -----------------------------------------------------------------------------
// Emscripten-only entry point. This translation unit is compiled only when
// CMake is configuring a build with the Emscripten toolchain. The desktop
// entry point in src/main.cpp (int SDL_main) is not compiled in that case.
//
// Why a separate file: the desktop and the browser have different ideas of
// "the main loop." Desktop applications block in a while(running) loop. The
// browser cannot block - it must yield back to the event loop after every
// render frame. The callback drives App::step_frame(), which shares the
// desktop fixed-step simulation clock but renders once per requestAnimationFrame.
// This adapter also keeps App alive after main() returns to the browser.
// -----------------------------------------------------------------------------

#include "Core/App.h"
#include "Core/Config.h"
#include "Core/Logger.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace {

// Browser render cadence is not the simulation cadence. step_frame() uses
// the same fixed 60 Hz accumulator and edge-input consumption as desktop.
ember::App* g_app = nullptr;
double      g_last_ticks_seconds = 0.0;

void main_loop_tick() {
    if (!g_app) {
        return;
    }

    // emscripten_get_now() returns wall-clock milliseconds. Convert to
    // seconds and clamp to the same cap the desktop Clock applies.
    const double now = emscripten_get_now() * 1e-3;
    double dt = now - g_last_ticks_seconds;
    g_last_ticks_seconds = now;

    // App owns the shared accumulator, frame clamp, catch-up budget and
    // single-render-per-rAF policy. Do not update gameplay once per rAF.
    g_app->step_frame(dt);
}

} // namespace

// On Emscripten we provide a plain int main(void) - SDL3's SDL_main macro
// is not used in the web build (the browser does not have a command line).
// We also disable SDL_main by NOT including <SDL3/SDL_main.h>.
int main(void) {
    ember::Logger::info("Ember Clash (web) starting...");

    // simulate_infinite_loop=false allows main() to return after registering
    // the callback. A stack App would therefore be destroyed immediately,
    // leaving g_app dangling before the first requestAnimationFrame. Static
    // storage keeps the application alive for the lifetime of the browser
    // main loop without depending on Emscripten's stack-unwinding mode.
    static ember::App app;
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

    // With simulate_infinite_loop=0 this call returns normally; the browser
    // invokes main_loop_tick asynchronously on subsequent animation frames.
    // The static App remains alive after this function returns.
    return 0;
}
