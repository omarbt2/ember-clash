# Ember Clash - Project Memory

> Read this first if you are an AI agent picking up this project.

## What this is

A 2D 1v1 platform fighter ("Smash-style" damage-and-knockback), written from
scratch in **C++20 + SDL3 + CMake**. No game engine. Target: Windows first,
then Android, then WebAssembly (Emscripten).

## Current milestone

**M1 - foundation (complete).** Subsequent milestones are tracked in
`README.md`'s "Roadmap" section.

## Build & run

```bat
:: from the repo root, in a Visual Studio "x64 Native Tools" command prompt
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\bin\EmberClash.exe
```

- Output binary lives at `build\bin\EmberClash.exe`; SDL3.dll is copied next
  to it by the post-build step.
- Configured with `/W4 /permissive- /std:c++20 /utf-8`.
- SDL3 3.2.10 is fetched at configure time via `FetchContent` - no manual
  SDL install is required.
- All code lives in the `ember::` namespace.

## Module layout

```
src/
  main.cpp              SDL_main entry; constructs ember::App and runs it
  Core/
    App.{h,cpp}         orchestrator: SDL init, owns window/renderer/input,
                        drives the main loop
    Config.h            tuning constants (window, timing, palette)
    Logger.{h,cpp}      thread-safe console logger (no SDL_Log)
    Time.{h,cpp}        capped delta-time clock
  Graphics/
    Window.{h,cpp}      RAII wrapper over SDL_Window
    Renderer.{h,cpp}    RAII wrapper over SDL_Renderer; draw helpers
  Input/
    InputManager.{h,cpp}  held/pressed/released keyboard state
  Physics/
    PhysicsConfig.h     tuning constants from the design brief (M2 wires up)
```

`Audio/`, `Gameplay/`, `World/`, `UI/` are **not** created yet - we add them
only when they have content. Do not create empty stub headers just to
"reserve" the namespace.

## Conventions & gotchas

* **C++20**, MSVC first. No `&&` in shell commands (we're on PowerShell).
* **No traditional game engine.** No third-party deps beyond SDL3.
* **No global state.** Subsystems are owned by `ember::App` and passed by
  reference.
* **SDL3 entry point.** `main.cpp` defines `int SDL_main(int, char**)`. Do
  **not** also `#include <SDL3/SDL_main.h>` from other translation units -
  it `#define`s `main` to `SDL_main` and causes duplicate-symbol link errors.
* **Logical presentation.** The renderer targets `config::kRenderWidth` x
  `kRenderHeight`; SDL3 letterboxes it to the actual window.
* **Renderer basics.** `Renderer::draw_filled_rect` / `draw_rect_outline`
  use logical (render-space) coordinates, not raw pixel coordinates.
* **Input is platform-agnostic.** All gameplay code must query
  `InputManager` (held/pressed/released) so gamepad / touch can be added
  later without touching gameplay.

## Testing the build

After meaningful changes:

```bat
cmake --build build --target EmberClash
```

Then either launch `build\bin\EmberClash.exe` and watch for the placeholder
arena, or run a non-interactive smoke test:

```powershell
Start-Process "build\bin\EmberClash.exe"
Start-Sleep -Seconds 2
Get-Process EmberClash   # should be running
Stop-Process -Name EmberClash -Force
```

## Git

Single branch: `master`. Commits use `openhands` as author.
Working commits to date:
* `004ab5a` - M1 foundation
