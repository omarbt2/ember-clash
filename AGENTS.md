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

**M2 — Movement + Physics + Arena (complete).** Two fighters walk, jump
(with a double jump), fast-fall, and collide with the platform layout
through `Arena::resolve_collision`. Keyboard input flows from
`InputManager` → `InputMap::sample` → `PlayerIntent` → `Fighter::update`,
with no `SDLK_*` references anywhere outside `InputMap.cpp` and
`InputManager.cpp`. Fighters that fall off the stage respawn at their
starting position; there is no KO counter yet (M5).

M3 (Combat + Hitboxes) is next; the orchestrator doc defines it in
section 36. We do not auto-start M3 - per section 34 of the orchestrator
doc, we wait for an explicit owner request.

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
    Fighter.{h,cpp}        fighter entity: motion step, collision, render
    InputMap.{h,cpp}       keyboard -> PlayerIntent (only place SDLK lives
                           outside InputManager)
    Match.{h,cpp}          owns 2 fighters + the arena; per-frame update
                           and render
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

## Git

Single branch: `master`. Commits use `openhands` as author.
Working commits to date:
* `004ab5a` - M1 foundation
* `388682a` - AGENTS.md bootstrap
* `d80ce00` - master orchestrator doc adoption
* *(this session)* - M2 movement + physics + arena
