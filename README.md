# Ember Clash

A 2D 1v1 platform fighter inspired by the *Smash Bros.* damage-and-knockback
formula. Built from scratch in **C++20** with **SDL3**, no traditional game
engine, on **CMake**.

> **Current milestone: M6 - Art, Animation, Audio, UI & Polish (M6-A
> slice complete; M6-B..F in progress).** Desktop is the reference
> platform; the codebase is also buildable for the browser via
> Emscripten. The M6-A slice replaces the placeholder colored-AABB
> fighters with a procedurally-generated pixel-art sprite (no external
> assets, no asset file I/O), gives the arena a vertical gradient
> background, and adds a death-zone tint band. The new art lives in
> `src/Gameplay/{ArtConfig.h, Sprite.{h,cpp}}` and is exercised by
> `tests/ember_test_m6.cpp`. All previous headless regressions still
> pass: **M4 AI 14/14, M5 match flow 14/14, M6-A art 5/5, M8 surface
> compile-time checks**. See "Roadmap" below for what is left in M6,
> M7, M11, and M12.

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

You should see a dark navy background with a vertical depth gradient,
a blue arena (main floor + two elevated side platforms) with a yellow
rim on the main floor, a thin red death-zone band just below the
floor, and two pixel-art fighters - one warm (P1, human) and one cool
(P2, bot). Player 1 (warm) is the human; **Player 2 (cool) is the bot
by default** - driven by the M4 deterministic state machine. Both can
walk, jump (and double-jump), and fast-fall. The three attacks land on
the opponent and launch them with knockback that scales with their
damage %. M6-A introduced the procedural pixel-art sprite (see
"Current milestone" above); the M3 hitbox / hurtbox debug outlines are
still on top of the sprite and will be removed in M6-C. There is no
second-human input yet (M9 is MISSING); for now just play against the
bot.

---

## Project layout

```
.
├── CMakeLists.txt                Top-level CMake build script
├── cmake/CopyWebArtifacts.cmake  Emscripten post-build helper (copies .js+.wasm into web/)
├── web/                          WebAssembly deploy bundle (index.html + .js + .wasm)
├── README.md                     This file
├── AGENTS.md                     Session-bootstrap memory
├── EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md  Design authority
├── .gitignore
├── src/
│   ├── main.cpp                 Desktop entry point (int SDL_main)
│   ├── main_emscripten.cpp      Emscripten-only entry point (em_set_main_loop)
│   ├── Core/
│   │   ├── App.{h,cpp}          Subsystem orchestrator + main loop
│   │   ├── Config.h             Window/timing/palette constants
│   │   ├── Logger.{h,cpp}       Tiny thread-safe console logger
│   │   ├── Math.h               Vec2 + Aabb primitives
│   │   └── Time.{h,cpp}         Clock + capped delta time
│   ├── Gameplay/
│   │   ├── Attack.{h,cpp}       Attack data table + active-window helpers
│   │   ├── AiConfig.h           AI difficulty tuning (reaction delay, etc.)
│   │   ├── AiController.{h,cpp} Deterministic FSM (Idle/Approach/Attack/Retreat/Recover/Defend)
│   │   ├── ArtConfig.h          M6 palette + sprite sizing constants
│   │   ├── CombatConfig.h       Knockback formula + cap constants
│   │   ├── Fighter.{h,cpp}      Fighter entity (state machine, motion, collide, render)
│   │   ├── InputMap.{h,cpp}     Keyboard -> PlayerIntent + CombatIntent
│   │   ├── Match.{h,cpp}        Owns arena + two fighters, resolves hits
│   │   └── Sprite.{h,cpp}       M6 procedural pixel-art fighter sprite (64x64 RGBA)
│   ├── Graphics/
│   │   ├── Window.{h,cpp}       SDL_Window RAII wrapper
│   │   └── Renderer.{h,cpp}     SDL_Renderer wrapper + draw helpers + sprite cache
│   ├── Input/
│   │   └── InputManager.{h,cpp} Keyboard state, polled each frame
│   ├── Physics/
│   │   ├── Motion.{h,cpp}       Pure per-tick motion step
│   │   └── PhysicsConfig.h      Tuning constants (gravity, walk, jump, ...)
│   ├── UI/
│   │   ├── BitmapFont.{h,cpp}   4x6 monospace font drawn with filled rects
│   │   └── Hud.{h,cpp}          HUD, KO banner, hit spark, damage-% text
│   └── World/
│       └── Arena.{h,cpp}        Platform list + collision + render
└── tests/
    ├── ember_test_ai.cpp        Headless M4 AI test harness         (6 scenarios, 14/14 PASS)
    ├── run_ai_test.bat          Windows runner for the M4 harness
    ├── ember_test_m5.cpp        Headless M5 match-flow harness      (6 scenarios, 14/14 PASS)
    ├── run_m5_test.bat          Windows runner for the M5 harness
    ├── ember_test_m6.cpp        Headless M6-A art test harness      (5 scenarios, 12/12 PASS)
    ├── run_m6_test.bat          Windows runner for the M6-A harness
    ├── run_m6_test.cmd          Wrapper that loads vcvars64 then runs the M6 runner
    ├── ember_test_m8.cpp        M8 surface test: static_asserts on App::step / run / request_exit
    ├── run_m8_test.bat          Windows runner for the M8 harness
    ├── verify_desktop_build.bat Loads vcvars64 then rebuilds the main game
    └── verify_all_desktop_tests.bat  Loads vcvars64, rebuilds, runs M4+M5+M6+M8 harnesses
```

`UI/` ships in M5 (bitmap font + HUD). `Audio/` lands in M6-D. The
`Gameplay/Sprite` module added in M6-A contains the procedurally-drawn
fighter silhouette; no asset files are shipped.


---

## Roadmap (high-level)

Status as of this commit. The active sequence is M6 -> M7 -> M11 -> M12
(set by the master orchestrator in `EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md`).
M9 (local multiplayer) and M10 (online) are **not** in the active sequence
and are listed as **MISSING** below so the gap is honest. They are
prerequisites for M11 online hardening; until they exist, M11 can only
audit deterministic simulation - it cannot harden real online paths.

1. **M1 — Foundation** *(COMPLETE)*: build, window, renderer, loop,
   delta time, input, placeholder scene.
2. **M2 — Movement + Physics + Arena** *(COMPLETE)*: gravity, walk,
   single & double jump, fast-fall, platform collision, death zone
   with respawn, the main arena layout, keyboard input wired through
   the input layer.
3. **M3 — Combat + Hitboxes** *(COMPLETE)*: hitboxes, hurtboxes,
   damage %, knockback, three attacks (Jab, Forward Tilt, Neutral Air),
   fighter state machine (Idle / Attacking / Hitstun / Dead),
   knockback scales with damage.
4. **M4 — AI opponent** *(COMPLETE)*: deterministic state machine for
   Player 2 (Idle / Approach / Attack / Retreat / Recover / Defend),
   seeded RNG, full difficulty table, AI integrated into Match
   (P1 = human, P2 = bot by default), headless test harness (14/14 PASS).
5. **M5 — Match flow** *(COMPLETE)*: 3 stocks each, KO decrements
   stocks and schedules a respawn, post-respawn invulnerability, KO
   banner with rematch prompt on `R`, HUD with stock pips + damage %,
   hit-spark on each landed hit, bitmap-font UI (no SDL_ttf
   dependency), 6 headless scenarios passing (14/14).
6. **M6 — Art, Animation, Audio, UI & Polish** *(PARTIAL: M6-A
   complete, M6-B..F MISSING)*. M6-A: procedural pixel-art sprite
   (P1 warm / P2 cool, no external assets), arena background gradient,
   death-zone tint band, Sprite unit test (5/5 PASS). M6-B animation,
   M6-C combat feel, M6-D audio subsystem, M6-E UI polish, M6-F
   integration + M6 tag are still pending.
7. **M7 — Android, Touch, Gamepad & Mobile** *(MISSING)*: gamepad
   adapter, virtual touch controls, Android NDK build + APK, mobile
   lifecycle hooks, display/aspect policy.
8. **M8 — WebAssembly / release integration** *(PARTIALLY VERIFIED)*:
   Emscripten build target produces `web/EmberClash.js` + `.wasm`
   (verified by HTTP probe of a local `python -m http.server`; in-browser
   visual gameplay is NOT VERIFIED - no headless Chrome in the build
   environment). `-sUSE_SDL=3`, `ALLOW_MEMORY_GROWTH=1`, `EXIT_RUNTIME=1`,
   no ASYNCIFY. `App::run` -> `App::step(dt)` + `App::request_exit()` so
   the web entry point can drive the loop from
   `emscripten_set_main_loop()` without touching any gameplay code.
   M8 surface test (compile-time static_asserts on `App::step` / `run` /
   `request_exit`) passes alongside the M4 + M5 + M6-A regression suites.
9. **M9 — Local multiplayer** *(MISSING)*: second human-controlled
   fighter. Requires a gamepad path (M7-A). Currently P2 is the AI by
   default; P2 is hard-wired to the AI in `Match::update` and there is
   no InputMap for a second human.
10. **M10 — Online multiplayer** *(MISSING)*: zero networking code in
    the repo. No socket / udp / tcp / webrtc / lobby / matchmaking /
    netcode of any kind. Building this from scratch is a large piece
    of work; it is not in the active milestone sequence and is listed
    here only so the gap is documented.
11. **M11 — Online / competitive hardening** *(BLOCKED)*: the M11 work
    is a determinism + desync + disconnect audit of the multiplayer /
    online paths. Until M9 + M10 actually exist, M11 can only audit
    deterministic simulation and cannot harden any real online path.
    We will explicitly NOT claim M11 done on online features that do
    not exist.
12. **M12 — Final release / v1.0** *(BLOCKED)*: depends on M6 + M7 +
    M9 + M10 + M11 all being real. Today it is not.

See `EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md` for the design authority
on this roadmap and `AGENTS.md` for the session-bootstrap memory.

---

## Testing

The project ships with a headless test harness per milestone, all
buildable + runnable from a single batch file on Windows. There is
also a one-shot CI-style runner (`tests/verify_all_desktop_tests.bat`)
that builds the main game and runs every test in order.

1. Build the main game once so SDL3 is fetched and the
   `build/CMakeFiles/EmberClash.dir/src/...` object files exist:

       cmake -S . -B build -G Ninja
       cmake --build build

2. Build and run **all** the desktop regression tests at once:

       tests\verify_all_desktop_tests.bat

   This script loads `vcvars64.bat`, rebuilds, then runs:

   * `run_ai_test.bat`     (M4 AI,           14/14 PASS)
   * `run_m5_test.bat`     (M5 match flow,   14/14 PASS)
   * `run_m6_test.bat`     (M6-A art,         5/5  PASS)
   * `run_m8_test.bat`     (M8 surface,       compile-time)

   Each `run_*.bat` script can also be invoked directly from
   `build/`. Exit code 0 means every scenario in every harness passed.

You should see five numbered scenarios for the M4 harness, six for
the M5 harness, and five for the M6-A harness - each printing
`PASS`, ending with `== ALL PASS (0 failure(s)) ==`. The M8 harness
is pure compile-time and prints nothing on success.

The harnesses reuse the same `AiController` / `Match` / `Fighter` /
`Sprite` / `ArtConfig` / `App` types the game does, so passing tests
are a real signal that the gameplay / art / shutdown paths will
behave the same way in the actual binary.

---

## License

Proprietary / TBD — set this before publishing the codebase.