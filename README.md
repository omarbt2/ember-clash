# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: M8 - WebAssembly / release integration.** Desktop
> build is still the reference; the codebase is now also buildable for
> the browser via Emscripten. The per-tick body was pulled out of the
> blocking main loop into `App::step(double dt)`, and a tiny
> `src/main_emscripten.cpp` drives that from `emscripten_set_main_loop()`.
> No gameplay code is platform-specific. The CMake build now branches
> on `EMSCRIPTEN` and links `-sUSE_SDL=3` (plus `ALLOW_MEMORY_GROWTH=1`
> and `EXIT_RUNTIME=1`) when targeting the web. ASYNCIFY is **not**
> enabled: the gameplay loop is plain synchronous C++ and ASYNCIFY would
> just bloat the .wasm. The desktop build is still green and the full
> headless test suite (M4 AI 14/14 + M5 match flow 14/14 + M8 surface
> compile-time checks) passes.

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

## Build (WebAssembly / Emscripten)

> **Note.** This section documents the build commands but the actual
> `emcc` link step was **not exercised in this repository's CI / build
> environment** (Emscripten is not installed on the Windows machine that
> runs the desktop build). The CMake Emscripten branch was validated by
> a configure-time dry-run with a stub toolchain (see `tests/` history);
> the real `emcmake cmake -B build-web ...` step should be re-run on a
> machine with the Emscripten SDK installed.

Prerequisites: install the Emscripten SDK from <https://emscripten.org/docs/getting_started/downloads.html>
or via `pip install emsdk`. We recommend a recent stable release
(3.1.50+; the SDL3 3.2.10 backend targets that range).

```bash
# One-time SDK install (in a fresh shell, after git clone of emsdk):
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.51
./emsdk activate 3.1.51
source ./emsdk_env.sh   # bash; for cmd.exe use emsdk_env.bat instead

# Configure + build the web target from the project root:
cd ../EmberClash
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

Artifacts are written to `build-web/bin/`:

* `EmberClash.html` - the HTML5 shell (loads the .js and .wasm)
* `EmberClash.js`   - the JavaScript glue
* `EmberClash.wasm` - the WebAssembly module

To run locally, you need to serve `build-web/bin/` over HTTP (the browser
will not load `.wasm` from a raw `file://` URL). From the project root:

```bash
cd build-web/bin
python -m http.server 8000
# then open http://localhost:8000/EmberClash.html
```

Or any other static-file server. Once the page loads, click the canvas
to give it keyboard focus, then use the same controls as the desktop
build.

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
    ├── main.cpp                 Desktop entry point (int SDL_main)
    ├── main_emscripten.cpp      Emscripten-only entry point (em_set_main_loop)
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
    │   ├── run_m5_test.bat      Build + run script for the M5 harness (Windows)
    │   ├── ember_test_m8.cpp    M8 surface test: static_asserts on App::step / run / request_exit
    │   └── run_m8_test.bat      Build + run script for the M8 harness (Windows)
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
6. **M6 — Art + Audio + UI + Polish** *(deferred)*: final character
   art, animation, arena art, audio, menus, performance cleanup.
7. **M7 — Android** *(deferred)*: Android build, touch controls,
   aspect-ratio handling, packaging.
8. **M8 — WebAssembly / release integration** *(this milestone)*:
   Emscripten build target (`-sUSE_SDL=3`, `ALLOW_MEMORY_GROWTH=1`,
   `EXIT_RUNTIME=1`, no ASYNCIFY), per-tick refactor of `App::run` into
   `App::step(dt)` + `App::request_exit()` so the web entry point can
   drive the loop from `emscripten_set_main_loop()` without touching
   any gameplay code. Desktop build preserved. M8 surface test
   (compile-time static_asserts on `App::step` / `run` /
   `request_exit`) passes alongside the M4 + M5 regression suites.
   The actual `emcc` link was not exercised in this repo's build
   environment (Emscripten is not installed locally); see the "Build
   (WebAssembly / Emscripten)" section above for the commands to run
   on a machine with the Emscripten SDK.

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

   4. Same drill for the M8 surface test (compile-time static_asserts
      that the web entry point's `App::step` / `App::request_exit` /
      `App::run` signatures have not drifted):

       cd build
       run_m8_test.bat

You should see six numbered scenarios for M4 — wiring, custom
difficulty, real-match simulation, FSM determinism, retreat on damage,
and off-stage recovery — each printing `PASS`, ending with
`== ALL PASS (0 failure(s)) ==`. The M5 harness prints a similar
banner. The M8 harness is pure compile-time and prints nothing on
success (exit code 0 = pass).

The harness reuses the same `AiController` and `Difficulty` types the
game does, so passing tests are a real signal that the bot will behave
the same way in the actual game. The M5 harness reuses `Match` +
`Fighter` + the bitmap font + the HUD. The M8 harness only includes
`Core/App.h`; its job is to catch an accidental signature change in
the per-tick API before it breaks the Emscripten build.

---

## License

Proprietary / TBD — set this before publishing the codebase.