# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: M4 - AI opponent.** Player 2 is now driven by a
> deterministic state machine (Idle / Approach / Attack / Retreat /
> Recover / Defend) with reaction-delay, attack-frequency, recovery-
> priority, defensive-threshold, and jump-aggression knobs. The bot
> plays a real match: approaches, commits to jabs / ftilt / nair, walks
> back when damaged, and tries to recover when launched off-stage.
> Difficulty is a single `ai::Difficulty` struct in
> `src/Gameplay/AiConfig.h`; the M4 build ships the `kStandard`
> preset. The state trace is reproducible for a given seed
> (xorshift32), verified by the headless test harness (14/14 PASS).

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
two elevated side platforms) outlined in yellow, a red fighter and a blue
fighter. Player 1 (red) is the human; **Player 2 (blue) is the bot by
default** — driven by the M4 deterministic state machine. Both can walk,
jump (and double-jump), and fast-fall. The three attacks land on the
opponent and launch them with knockback that scales with their damage %.
Active hitboxes (red outline) and hurtboxes (green outline) are drawn as
debug overlays; they'll be removed when real art lands in M6. To disable
the bot and use a second human on the keyboard, pass `-p2-human` (not yet
implemented — for now just play against the bot and see how you do).

---

## Project layout

```
.
├── CMakeLists.txt            Top-level CMake build script
├── cmake/CopySDL3DLL.cmake   Windows post-build helper (copies SDL3.dll)
├── README.md                 This file
├── .gitignore
├── src/
    ├── main.cpp                 Entry point
    ├── Core/
    │   ├── App.{h,cpp}          Subsystem orchestrator + main loop
    │   ├── Config.h             Window/timing/palette constants
    │   ├── Logger.{h,cpp}       Tiny thread-safe console logger
    │   ├── Math.h               Vec2 + Aabb primitives
    │   └── Time.{h,cpp}         Clock + capped delta time
    ├── Gameplay/
    │   ├── Attack.{h,cpp}       Attack data table + active-window helpers
    │   ├── AiConfig.h            AI difficulty tuning (reaction delay, attack freq, etc.)
    │   ├── AiController.{h,cpp}  Deterministic state machine (Idle/Approach/Attack/Retreat/Recover/Defend)
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
└── tests/
    │   ├── ember_test_ai.cpp    Headless M4 AI test harness (6 scenarios, 14/14 PASS)
    │   └── run_ai_test.bat      Build + run script for the AI test harness (Windows)
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
3. **M3 — Combat + Hitboxes** *(complete)*: hitboxes, hurtboxes,
   damage %, knockback, three attacks (Jab, Forward Tilt, Neutral Air),
   fighter state machine (Idle / Attacking / Hitstun / Dead),
   knockback scales with damage.
4. **M4 — AI opponent** *(this milestone)*: deterministic state machine
   for Player 2 (Idle / Approach / Attack / Retreat / Recover / Defend),
   seeded RNG, full difficulty table, AI integrated into Match
   (P1 = human, P2 = bot by default), headless test harness (14/14 PASS).
5. **M5 — Match flow**: KO, stocks, respawn timer, rematch.
6. **M6+**: final art, animation, audio, mobile (Android) port, web
   (Emscripten) port.

See `docs/ROADMAP.md` once it exists; until then this README is canonical.

---

## Testing

The project ships with a headless AI test harness so the M4 state
machine can be verified without launching a window.

1. Build the main game once so SDL3 is fetched and the
   `build/CMakeFiles/EmberClash.dir/src/...` object files exist:

       cd build
       cmake -G Ninja ..
       ninja

2. Build and run the AI test (Windows; uses MSVC `cl.exe` directly so
   it doesn't depend on a fresh CMake project for the test itself):

       cd build
       run_ai_test.bat

You should see six numbered scenarios — wiring, custom difficulty,
real-match simulation, FSM determinism, retreat on damage, and
off-stage recovery — each printing `PASS`, ending with
`== ALL PASS (0 failure(s)) ==`.

The harness reuses the same `AiController` and `Difficulty` types the
game does, so passing tests are a real signal that the bot will behave
the same way in the actual game.

---

## License

Proprietary / TBD — set this before publishing the codebase.