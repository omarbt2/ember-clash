#pragma once

#include <memory>

namespace ember {

class Clock;
class InputManager;
class Match;
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

    std::unique_ptr<Clock>        clock_;
    std::unique_ptr<InputManager> input_;
    std::unique_ptr<Match>        match_;
    std::unique_ptr<Window>       window_;
    std::unique_ptr<Renderer>     renderer_;
    bool                          sdl_initialized_{false};
    bool                          running_{false};
};

} // namespace ember