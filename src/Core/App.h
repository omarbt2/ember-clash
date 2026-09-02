#pragma once

#include <memory>

namespace ember {

class Clock;
class InputManager;
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

    // Enters the main loop until quit is requested or a fatal error occurs.
    // Returns the process exit code.
    int run();

private:
    void shutdown();
    void handle_events();
    void update(double dt);
    void render();
    void draw_placeholder_scene(double elapsed);

    std::unique_ptr<Clock>        clock_;
    std::unique_ptr<InputManager> input_;
    std::unique_ptr<Window>       window_;
    std::unique_ptr<Renderer>     renderer_;
    bool                          sdl_initialized_{false};
    bool                          running_{false};
};

} // namespace ember