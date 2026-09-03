// -----------------------------------------------------------------------------
// Ember Clash M8 headless test harness.
//
// Purpose
//   The M8 refactor pulled the per-tick body out of App::run() into a new
//   public App::step(double dt), and added App::request_exit(). The whole
//   point was to let the Emscripten entry point drive the loop without
//   duplicating any gameplay logic.
//
// What this test guards against
//   1. App::step(double) accidentally removed/changed in a future refactor
//      (would silently break the Emscripten build - the web entry point
//      calls app.step(dt) directly).
//   2. App::request_exit() accidentally removed/changed (would silently
//      break any future shutdown wiring on the web side).
//   3. App::run() being non-const (a no-op check, but documents the
//      surface area).
//
// The test is pure compile-time. It does not link App.cpp.obj, does not
// initialise SDL, and does not need a display. The static_asserts fire
// at template instantiation if the signatures ever change. The minimal
// `int main()` is just so cl.exe produces a runnable binary; if the
// asserts above pass, the binary prints nothing and exits 0.
// -----------------------------------------------------------------------------

#include "Core/App.h"

#include <type_traits>

// The exact shape we rely on at the Emscripten entry point. If these
// ever stop compiling, the web build's src/main_emscripten.cpp breaks
// and that is the bug we want this test to catch.
static_assert(std::is_same_v<decltype(&ember::App::step),
                             void (ember::App::*)(double)>,
              "App::step(double) signature changed - src/main_emscripten.cpp "
              "and the web build will break. Update both together.");

static_assert(std::is_same_v<decltype(&ember::App::request_exit),
                             void (ember::App::*)()>,
              "App::request_exit() signature changed - any future web-side "
              "shutdown wiring will break. Update both together.");

static_assert(std::is_same_v<decltype(&ember::App::run),
                             int (ember::App::*)()>,
              "App::run() signature changed - the desktop entry point in "
              "src/main.cpp and the test runners will break.");

int main() {
    return 0;
}

