#pragma once

#include "Core/Settings.h"

#include <memory>

namespace ember {

class Audio;
class Clock;
class Gamepad;
class Haptics;
class InputManager;
class Match;
class Menu;
class Renderer;
class Window;

// Orchestrates subsystems and owns the main loop. Constructing an App
// initializes SDL, opens a window, and prepares the renderer. Call run()
// to enter the loop and exit() (or close the window) to leave it.
class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    // Initialize subsystems. Returns false on any failure (already logged).
    bool initialize();

    // Desktop: enters the blocking main loop until quit is requested or a
    // fatal error occurs. Returns the process exit code.
    //
    // Web (Emscripten): does not block. Initializes the loop and returns 0;
    // the per-frame work is driven by emscripten_set_main_loop(), which calls
    // step() from a separate translation unit. The blocking form is
    // intentionally not used on the web - browsers cannot block.
    int run();

    // Single per-tick step: handle_events + update + render. The argument
    // is the desired frame delta in seconds; the Emscripten adapter passes
    // the desired fixed step, the desktop run() uses Clock::delta_seconds().
    void step(double dt);

    // Politely request the desktop loop to exit at the next iteration.
    // (No-op for the Emscripten adapter; the web page simply goes away.)
    void request_exit();

private:
    void shutdown();
    void handle_events();
    void update(double dt);
    void render();

    // ---- M12 wiring -----------------------------------------------------
    // handle_menu_action() runs the side effects implied by the
    // current menu action (start a match, save settings, toggle
    // fullscreen, etc.). App::update() calls this once per frame
    // after the menu has been ticked.
    void handle_menu_action();
    // start_match_if_idle() / end_match_to_rematch() / pause_match()
    // are the three App-side state transitions the menu can
    // trigger. They push events into the menu and rebuild Match
    // when a rematch is requested.
    void start_match();
    void end_match();
    void pause_match();
    void resume_match();
    void reset_match();
    void flush_settings();

    std::unique_ptr<Clock>        clock_;
    std::unique_ptr<InputManager> input_;
    std::unique_ptr<Gamepad>      gamepad_;
    std::unique_ptr<Haptics>      haptics_;
    std::unique_ptr<Audio>        audio_;
    std::unique_ptr<Menu>         menu_;
    std::unique_ptr<Match>        match_;
    std::unique_ptr<Window>       window_;
    std::unique_ptr<Renderer>     renderer_;

    Settings                      settings_{};
    bool                          settings_dirty_{false};
    bool                          local_two_player_{false};
    bool                          free_training_{false};
    double                        last_settings_flush_{0.0};

    bool                          sdl_initialized_{false};
    bool                          running_{false};
    // P17: fixed-timestep accumulator. The simulation now runs at
    // exactly config::kTargetFps (60 Hz) regardless of how fast the
    // desktop loop spins. The previous "one update per render frame"
    // code made the game feel hyper-fast on a 144 Hz monitor (and
    // disgustingly slow on a 30 Hz one) - every per-tick constant in
    // PhysicsConfig.h and CombatConfig.h assumes 60 Hz, so the only
    // way to honour that contract on arbitrary hardware is to cap
    // the simulation rate.
    double                        sim_accumulator_{0.0};
    // Hard cap on the number of catch-up sim ticks per render frame.
    // If the desktop stalled for 250 ms (focus loss, breakpoint, etc.)
    // we do not want to run 15 sim ticks in one go and "fast forward"
    // the game state. Three ticks is enough to absorb a single missed
    // frame; the rest of the wall time is dropped (the menu re-syncs
    // when the user clicks back in).
    static constexpr int          kMaxSimTicksPerFrame = 3;

    // P17 camera state. The previous build had no camera: the entire
    // 1280x720 stage was letterboxed into the window so the 96x128
    // fighter sprites were small dots, especially when they were on
    // opposite sides of the stage at spawn time. The camera below
    // tracks the midpoint of the two live fighters and translates
    // the world so the action is always centred. It does NOT scale
    // (we keep the simulation pixels per render pixel ratio intact
    // so a future zoom does not invalidate any test).
    double                        camera_x_{0.0};
    double                        camera_y_{0.0};
};

} // namespace ember