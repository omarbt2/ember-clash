# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: foundation (M1).** The project builds and runs a
> minimal main loop with placeholder rendering. Combat, physics, AI, and
> mobile ports arrive in later milestones.

---

## Requirements

* **C++20** compiler (MSVC 19.29+ / clang 14+ / gcc 11+)
* **CMake 3.20+**
* **Ninja** (recommended) or any CMake-supported generator
* **Windows 10+**, **Linux**, or **macOS**
* On Windows: the **Windows 10/11 SDK** and either **Visual Studio 2019
  Build Tools** or any newer MSVC toolchain

SDL3 itself is fetched at configure time by CMake's `FetchContent`, so there
is no separate SDL install step.

---

## Build (Windows, recommended)

From a *x64 Native Tools Command Prompt for VS* (or after running
`vcvars64.bat`):

```bat
cd path\to\EmberClash
cmake -S . -B build -G Ninja
cmake --build build
```

The executable is written to `build\bin\EmberClash.exe` and the SDL3 runtime
DLL is copied next to it automatically.

## Build (Windows, multi-config MSVC)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The resulting binary lives under `build\Release\EmberClash.exe` (the
post-build step still copies `SDL3.dll` next to it).

## Build (Linux / macOS)

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/EmberClash
```

---

## Run

```bat
build\bin\EmberClash.exe
```

Controls (placeholder; rebinding lands later):

| Key        | Action                                  |
| ---------- | --------------------------------------- |
| `Esc`      | Quit                                    |
| `F11`      | Toggle fullscreen *(reserved, M1 stub)* |
| Any key    | Used to verify input handling           |

You should see a dark blue background, a light-blue floor with a yellow
outline, and two coloured "fighter" boxes bobbing gently on either side —
proof that the loop, delta time, renderer, and input system are wired up.

---

## Project layout

```
.
├── CMakeLists.txt            Top-level CMake build script
├── cmake/CopySDL3DLL.cmake   Windows post-build helper (copies SDL3.dll)
├── README.md                 This file
├── .gitignore
└── src/
    ├── main.cpp              Entry point
    ├── Core/
    │   ├── App.{h,cpp}       Subsystem orchestrator + main loop
    │   ├── Config.h          Window/timing/palette constants
    │   ├── Logger.{h,cpp}    Tiny thread-safe console logger
    │   └── Time.{h,cpp}      Clock + capped delta time
    ├── Graphics/
    │   ├── Window.{h,cpp}    SDL_Window RAII wrapper
    │   └── Renderer.{h,cpp}  SDL_Renderer wrapper + draw helpers
    ├── Input/
    │   └── InputManager.{h,cpp}  Keyboard state, polled each frame
    └── Physics/
        └── PhysicsConfig.h   Tuning constants (wired in M2)
```

Future modules (`Gameplay`, `World`, `Audio`, `UI`) will land in their own
milestones and only when they have content.

---

## Roadmap (high-level)

1. **M1 — Foundation** (this milestone): build, window, renderer, loop,
   delta time, input, placeholder scene.
2. **M2 — Physics & arena**: gravity, jumping, platforms, collision,
   death zones, the main arena layout.
3. **M3 — Combat**: hitboxes, hurtboxes, damage %, knockback, one or two
   sample moves.
4. **M4 — AI opponent**: deterministic state machine.
5. **M5 — Match flow**: KO, respawn, rematch.
6. **M6+**: final art, animation, audio, mobile (Android) port, web
   (Emscripten) port.

See `docs/ROADMAP.md` once it exists; until then this README is canonical.

---

## License

Proprietary / TBD — set this before publishing the codebase.