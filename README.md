# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: M5 - Match flow.** Three stocks per fighter; a KO
> decrements the stock, schedules a respawn after `kRespawnDelaySeconds`
> (`0.5s`), grants `kRespawnInvulnSeconds` (`1.5s`) of post-respawn
> invulnerability, and ends the match when one fighter's stock count
> hits 0. A `GAME!` banner shows for `kKoBannerInitialSeconds` (`1.5s`)
> and then the rematch prompt (`P1 WINS - PRESS R`) for the rest of
> the post-match time. Press `R` to rematch. The HUD is rendered with
> a 4x6 bitmap font (no SDL_ttf dependency) and shows stock pips
> plus a 2x-scaled damage % in the top corners of the screen, the
> damage % also floats above each fighter, and a hit-spark `+`
> flashes at the impact point for `kHitSparkFrames` frames.
> M4's AI opponent is still in place (P2 = bot by default). Verified
> by the headless M5 test harness (14/14 PASS) on top of the M4 AI
> harness (14/14 PASS).

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
| Rematch         | `R`            | `R`             |

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
    ├── UI/
    │   ├── BitmapFont.{h,cpp}   4x6 monospace font drawn with filled rects
    │   └── Hud.{h,cpp}          HUD, KO banner, hit spark, damage-% text
    └── World/
        └── Arena.{h,cpp}        Platform list + collision + render
└── tests/
    │   ├── ember_test_ai.cpp    Headless M4 AI test harness (6 scenarios, 14/14 PASS)
    │   ├── run_ai_test.bat      Build + run script for the AI test harness (Windows)
    │   ├── ember_test_m5.cpp    Headless M5 match-flow harness (6 scenarios, 14/14 PASS)
    │   └── run_m5_test.bat      Build + run script for the M5 harness (Windows)
```

`UI/` ships in M5 (bitmap font + HUD). `Audio/` lands in its own
milestone, when there is something to play.

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
5. **M5 — Match flow** *(complete)*: 3 stocks each, KO decrements
   stocks and schedules a respawn, post-respawn invulnerability, KO
   banner with rematch prompt on `R`, HUD with stock pips + damage %,
   hit-spark on each landed hit, bitmap-font UI (no SDL_ttf
   dependency), 6 headless scenarios passing (14/14).
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

   3. Same drill for the M5 match-flow harness:

       cd build
       run_m5_test.bat

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