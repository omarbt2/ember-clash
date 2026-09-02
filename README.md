# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: M3 — Combat + Hitboxes.** Three attacks are live
> (Jab, Forward Tilt, Neutral Air), each with startup / active / recovery
> frames, a hitbox, and the damage-and-knockback formula. The fighter
> runs a real state machine (Idle / Attacking / Hitstun / Dead) and the
> match resolves AABB overlaps between the attacker's hitbox and the
> defender's hurtbox. Knockback scales with accumulated damage. The red
> outline during an attack is the active hitbox; the green outline is the
> hurtbox. Both are debug overlays and will be removed in M6.

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

Controls (keyboard, two players on one keyboard):

| Action          | Player 1 (red) | Player 2 (blue) |
| --------------- | -------------- | --------------- |
| Move left       | `A`            | `←`             |
| Move right      | `D`            | `→`             |
| Jump (also double-jump) | `W` or `Space` | `↑`        |
| Fast-fall       | `S` (in air)   | `↓` (in air)    |
| Jab             | `J`            | Numpad `1`      |
| Forward Tilt    | `K`            | Numpad `2`      |
| Neutral Air     | `L` (in air)   | Numpad `3` (in air) |
| Quit            | `Esc`          | `Esc`           |

You should see a dark navy background, a light-blue arena (main floor plus
two elevated side platforms) outlined in yellow, and a red and a blue
fighter. Both fighters can walk, jump (and double-jump), and fast-fall.
The three attacks land on the opponent and launch them with knockback
that scales with their damage %. Active hitboxes (red outline) and
hurtboxes (green outline) are drawn as debug overlays; they'll be removed
when real art lands in M6.

---

## Project layout

```
.
├── CMakeLists.txt            Top-level CMake build script
├── cmake/CopySDL3DLL.cmake   Windows post-build helper (copies SDL3.dll)
├── README.md                 This file
├── .gitignore
└── src/
    ├── main.cpp                 Entry point
    ├── Core/
    │   ├── App.{h,cpp}          Subsystem orchestrator + main loop
    │   ├── Config.h             Window/timing/palette constants
    │   ├── Logger.{h,cpp}       Tiny thread-safe console logger
    │   ├── Math.h               Vec2 + Aabb primitives
    │   └── Time.{h,cpp}         Clock + capped delta time
    ├── Gameplay/
    │   ├── Attack.{h,cpp}       Attack data table + active-window helpers
    │   ├── CombatConfig.h       Knockback formula + cap constants
    │   ├── Fighter.{h,cpp}      Fighter entity (state machine, motion, collide, render)
    │   ├── InputMap.{h,cpp}     Keyboard → PlayerIntent + CombatIntent
    │   └── Match.{h,cpp}        Owns arena + two fighters, resolves hits
    ├── Graphics/
    │   ├── Window.{h,cpp}       SDL_Window RAII wrapper
    │   └── Renderer.{h,cpp}     SDL_Renderer wrapper + draw helpers
    ├── Input/
    │   └── InputManager.{h,cpp} Keyboard state, polled each frame
    ├── Physics/
    │   ├── Motion.{h,cpp}       Pure per-tick motion step
    │   └── PhysicsConfig.h      Tuning constants (gravity, walk, jump, ...)
    └── World/
        └── Arena.{h,cpp}        Platform list + collision + render
```

`Audio/` and `UI/` will land in their own milestones and only when they
have content.

---

## Roadmap (high-level)

1. **M1 — Foundation** *(complete)*: build, window, renderer, loop,
   delta time, input, placeholder scene.
2. **M2 — Movement + Physics + Arena** *(complete)*: gravity, walk,
   single & double jump, fast-fall, platform collision, death zone
   with respawn, the main arena layout, keyboard input wired through
   the input layer.
3. **M3 — Combat + Hitboxes** *(this milestone)*: hitboxes, hurtboxes,
   damage %, knockback, three attacks (Jab, Forward Tilt, Neutral Air),
   fighter state machine (Idle / Attacking / Hitstun / Dead),
   knockback scales with damage.
4. **M4 — AI opponent**: deterministic state machine.
5. **M5 — Match flow**: KO, stocks, respawn timer, rematch.
6. **M6+**: final art, animation, audio, mobile (Android) port, web
   (Emscripten) port.

See `docs/ROADMAP.md` once it exists; until then this README is canonical.

---

## License

Proprietary / TBD — set this before publishing the codebase.