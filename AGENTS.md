# Ember Clash - Project Memory

> Read this first if you are an AI agent picking up this project.

## What this is

A 2D 1v1 platform fighter ("Smash-style" damage-and-knockback), written from
scratch in **C++20 + SDL3 + CMake**. No game engine. Target: Windows first,
then Android, then WebAssembly (Emscripten).

## Authoritative project spec

The full project vision, architecture principles, design doc, roadmap
(M1-M8), and operating policy live in:

> `EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md`

That file is the design authority. This `AGENTS.md` is the
session-bootstrap memory; when the two disagree, the orchestrator doc wins
for design questions, and the actual code/tests in the repo win for "what is
true right now." If the repo and the doc disagree, the discrepancy is
itself a finding worth flagging to the owner.

## Current milestone

**M4 — AI opponent (this milestone).** Player 2 is now driven by
`ember::ai::AiController`, a deterministic finite-state machine with
six states: `Idle` / `Approach` / `Attack` / `Retreat` / `Recover` /
`Defend`. All tuning lives in `ember::ai::Difficulty` (defined in
`src/Gameplay/AiConfig.h`): reaction delay, attack frequency, recovery
priority, defensive threshold, jump aggression, RNG seed. RNG is
xorshift32 so a given seed always produces the same bot trace (good
for replays, tests, and bug reports). The bot is wired in by default
for P2 through `Match::set_ai(...)`; M4 ships the `kStandard` preset
and the headless test harness (`tests/ember_test_ai.cpp` built via
`build/run_ai_test.bat`) covers six scenarios / 14 assertions, all
green as of the M4 tag.

M3 — Combat + Hitboxes — is complete. Three attacks (Jab / Forward
Tilt / Neutral Air), hitbox/hurtbox, damage %, knockback formula,
hitstun, fighter state machine (`Idle` / `Attacking` / `Hitstun` /
`Dead`) all ship and are exercised by the M3 headless combat harness.

M5 (Match flow: stocks, KO, respawn timer, rematch, damage-% UI,
KO banner) is next. M6+ covers final art, animation, audio, then the
Android and Emscripten ports. We do not auto-start M5 without an
explicit owner request (per orchestrator doc §34).

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
  main.cpp                 SDL_main entry; constructs ember::App and runs it
  Core/
    App.{h,cpp}            orchestrator: SDL init, owns
                           window/renderer/input/match, drives the main loop
    Config.h               tuning constants (window, timing, palette)
    Logger.{h,cpp}         thread-safe console logger (no SDL_Log)
    Math.h                 Vec2 + Aabb primitives (header-only)
    Time.{h,cpp}           capped delta-time clock
  Gameplay/
    Attack.{h,cpp}         attack table (startup/active/recovery, hitbox,
                           damage, knockback, hitstun) + active-window
                           helpers; kAttacks[] is the single source of
                           attack tuning
    AiConfig.h             AI difficulty struct (ember::ai::Difficulty)
                           + kStandard / kEasy / kHard presets; all
                           bot tuning lives here
    AiController.{h,cpp}   deterministic FSM (Idle/Approach/Attack/
                           Retreat/Recover/Defend); owns its
                           Difficulty; xorshift32 RNG; integrates
                           into Match via set_ai(idx, unique_ptr)
    CombatConfig.h         knockback formula + cap constants
    Fighter.{h,cpp}        fighter entity: state machine (Idle/Attacking/
                           Hitstun/Dead), motion step, collision, render
    InputMap.{h,cpp}       keyboard -> PlayerIntent + CombatIntent (only
                           place SDLK lives outside InputManager)
    Match.{h,cpp}          owns 2 fighters + the arena; per-frame update
                           runs fighter updates, resolves hits, respawns
  Graphics/
    Window.{h,cpp}         RAII wrapper over SDL_Window
    Renderer.{h,cpp}       RAII wrapper over SDL_Renderer; draw helpers
  Input/
    InputManager.{h,cpp}   held/pressed/released keyboard state
  Physics/
    Motion.{h,cpp}         pure per-tick step_motion(); reads
                           PhysicsConfig.h for all tuning
    PhysicsConfig.h        tuning constants (gravity, walk, jump, fast-fall,
                           air control, terminal velocity)
  World/
    Arena.{h,cpp}          platform list + AABB collision + render
```

`Audio/` and `UI/` are **not** created yet - we add them only when they
have content. Do not create empty stub headers just to "reserve" the
namespace.

## Operating policy (from the orchestrator doc)

Key rules condensed for fast recall during a session:

* **Source of truth**: the actual files in the repo + tested behavior.
  When the spec and the code diverge, the divergence is a finding to flag.
* **No fabrication** (spec ┬¦24): never claim a model was consulted, a build
  was run, or a test passed unless it actually happened in this session.
* **When to ask the owner** (spec ┬¦30): external credentials missing,
  genuine subjective product decision, destructive action, conflicting
  requirements, or environment blocking progress. Otherwise, decide and
  continue.
* **Milestone discipline** (spec ┬¦31, ┬¦34): stop at the end of every
  milestone. Do not auto-start the next one without an explicit owner
  request.
* **Smallest coherent change** (spec ┬¦25): don't rewrite working sections
  without a strong technical reason. Preserve M1.
* **Code/test policy** (spec ┬¦25, ┬¦26): build, test, verify, then commit.
  Compilation success is not sufficient evidence of correctness.
* **Don't scatter tuning values** (spec ┬¦9): keep all gameplay-tuning
  constants centralized in `src/Physics/PhysicsConfig.h` /
  `src/Core/Config.h`. Future M2-M5 modules read from there.
* **Don't hard-code platform input into gameplay** (spec ┬¦11): all keyboard
  queries go through `InputManager`. Future gamepad/touch plug in there.

## Multi-model usage (spec ┬¦21)

The orchestrator doc names a primary "master" and several specialists.
The runtime exposes corresponding profiles:

* `gpt-5.6-sol` ŌĆö closest to the "master" role (architecture, final
  decisions, hard debugging). Use `switch_llm` when a hard call has to be
  made and an independent pass would help.
* `nemotron-3-super-120b-a12b-free` ŌĆö code review, edge cases, QA pass
  (spec ┬¦22 Rule B / Rule D).
* `nemotron-3-ultra-550b-a55b-free` ŌĆö long-horizon architecture problems
  (use sparingly).
* `laguna-s-2.1-free` ŌĆö backup.

Per spec ┬¦22 Rule E, **never pretend a model was consulted** when it
wasn't. If `switch_llm` fails or the chosen profile is rate-limited, fall
back transparently: say so internally and keep going.

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
* **Tuning constants live in one place.** All physics values
  (`kGravity`, `kWalkSpeed`, `kFirstJumpVelocity`, `kDoubleJumpVelocity`,
  `kAirControl`, `kTerminalVelocity`, `kFastFallMultiplier`) are inlined
  in `src/Physics/PhysicsConfig.h`. Window/timing/palette live in
  `src/Core/Config.h`. Don't duplicate them anywhere else.
* **SDLK_* is forbidden outside InputManager + InputMap.** When
  designing new features, take a `PlayerIntent` (or an equivalent
  platform-agnostic struct) at the boundary; never reach into
  `InputManager::held(SDLK_*)` from gameplay / AI / render code.
* **Instance-member naming.** All non-static members of game classes
  end in a trailing underscore (`velocity_`, `grounded_`, `body_`,
  `jumps_remaining_`, ...). Don't drop the underscore - it will silently
  fail to compile (the parser reports "undeclared identifier" at the
  call site, not a typo diagnostic).
* **No `ember::gameplay::` sub-namespace.** M2 initially had one and
  removed it because it forced qualification across layers without
  buying any clarity. Keep types like `PlayerIntent`, `MotionState`,
  `MotionResult`, and `step_motion` directly in `ember::`.

## Testing the build

After meaningful changes:

```bat
cmake --build build --target EmberClash
```

Then either launch `build\bin\EmberClash.exe` and confirm visually that
the two fighters are sitting on the main platform and respond to WASD /
arrows, or run a non-interactive smoke test:

```powershell
Start-Process "build\bin\EmberClash.exe"
Start-Sleep -Seconds 2
Get-Process EmberClash   # should be running
Stop-Process -Name EmberClash -Force
```

For M2 the visual baseline is: dark navy background, light-blue arena
(main floor across the whole stage, two side platforms 160 px above
it, all outlined in yellow), a red P1 on the left and a blue P2 on the
right, both sitting on the main platform. Moving and jumping should
work; falling off the bottom should respawn at the starting position.

For M3 the visual baseline adds: a green hurtbox outline on each
fighter body, plus a red active-hitbox outline on whichever fighter is
currently mid-swing. Press `J`/`K` (P1) or numpad `1`/`2` (P2) on the
ground to throw a Jab or Forward Tilt, and `L` / numpad `3` in the air
for a Neutral Air. The opponent should flinch (knockback) and gain
damage %. After enough accumulated damage, a single attack will launch
them off the stage; they auto-respawn for now (the M5 respawn timer
replaces that).

For M4 the visual baseline is unchanged from M3. The difference is
that Player 2 now moves on its own even when no one is touching the
arrow keys — it's driven by the `ember::ai::AiController` state
machine. If you just let the game sit for a few seconds after launch,
P2 should walk toward P1, start swinging jabs / ftilt / nair, and
occasionally back off when it has taken damage.

### Headless test harness (scratch tests)

For purely-logical changes (state machines, formulas, geometry), the
production binary's smoke test isn't enough — there's no easy way to
press specific keys at specific frames. The pattern that works on this
toolchain is to write a small `.cpp` file that includes the gameplay
headers, compile it against the same sources with
`cl /std:c++latest /EHsc ... SDL3.lib`, and assert in `main()`. The
M3 scratch test (45 cases) and M4 AI test (6 scenarios / 14 assertions)
were written this way. They both caught real bugs that the smoke
test had missed because the smoke test never actually moves fighters.

M3's test lived under `build/` and was deleted before commit. Starting
with M4, the headless tests are checked in under `tests/`:

```
tests/
  ember_test_ai.cpp    M4 AI test (6 scenarios / 14 assertions, all green)
  run_ai_test.bat      build/runner script (Windows)
```

The runner is invoked as `build/run_ai_test.bat` (it lives under
`build/` because it needs to find the per-file `*.cpp.obj` files that
CMake produces there; the `tests/` source is passed in as a command-
line argument). All test artifacts (`ember_test_ai.exe`, `*.ilk`,
`*.pdb`) live under `build/` and are gitignored.

### AI conventions (M4)

* `ember::ai::AiController` is a value type held by `Match` through
  `std::unique_ptr<AiController>`; it never owns fighters directly.
* The bot emits `PlayerIntent` + `CombatIntent` (the same types the
  human `InputMap` produces) so `Fighter` cannot tell whether an
  intent came from a human or the AI. This is intentional — the bot
  is just another `InputMap` from the Fighter's point of view.
* RNG is **xorshift32** (inside `AiController.cpp`) and is seeded by
  `Difficulty::seed`. Do not call `std::rand` / `<random>` directly
  inside the FSM — it would break determinism guarantees.
* All AI tuning numbers live in `src/Gameplay/AiConfig.h`. Do not
  hard-code reaction delays or attack thresholds inside
  `AiController.cpp`.
* New AI states are added by extending the `State` enum and the
  `tick_state` switch in `AiController.cpp`, and by adding the
  transition conditions in `update` (or a private helper). The
  test harness in `tests/ember_test_ai.cpp` should be extended to
  cover any new state.

## Git

Single branch: `master`. Commits use `openhands` as author.
Working commits to date:
* `004ab5a` - M1 foundation
* `388682a` - AGENTS.md bootstrap
* `d80ce00` - master orchestrator doc adoption
* `853062a` - M2 movement + physics + arena
* *(this session)* - M3 combat + hitboxes (Jab, FTilt, NAir) + M2
  resolve_collision bugfix
