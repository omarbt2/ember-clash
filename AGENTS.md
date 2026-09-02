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

**M1 - foundation (complete).** M2 (Movement + Physics + Arena) is defined
in the orchestrator doc, sections 32 and 35. We do not auto-start M2 - per
section 34 of the orchestrator doc, we wait for an explicit owner request.

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

## Operating policy (from the orchestrator doc)

Key rules condensed for fast recall during a session:

* **Source of truth**: the actual files in the repo + tested behavior.
  When the spec and the code diverge, the divergence is a finding to flag.
* **No fabrication** (spec §24): never claim a model was consulted, a build
  was run, or a test passed unless it actually happened in this session.
* **When to ask the owner** (spec §30): external credentials missing,
  genuine subjective product decision, destructive action, conflicting
  requirements, or environment blocking progress. Otherwise, decide and
  continue.
* **Milestone discipline** (spec §31, §34): stop at the end of every
  milestone. Do not auto-start the next one without an explicit owner
  request.
* **Smallest coherent change** (spec §25): don't rewrite working sections
  without a strong technical reason. Preserve M1.
* **Code/test policy** (spec §25, §26): build, test, verify, then commit.
  Compilation success is not sufficient evidence of correctness.
* **Don't scatter tuning values** (spec §9): keep all gameplay-tuning
  constants centralized in `src/Physics/PhysicsConfig.h` /
  `src/Core/Config.h`. Future M2-M5 modules read from there.
* **Don't hard-code platform input into gameplay** (spec §11): all keyboard
  queries go through `InputManager`. Future gamepad/touch plug in there.

## Multi-model usage (spec §21)

The orchestrator doc names a primary "master" and several specialists.
The runtime exposes corresponding profiles:

* `gpt-5.6-sol` — closest to the "master" role (architecture, final
  decisions, hard debugging). Use `switch_llm` when a hard call has to be
  made and an independent pass would help.
* `nemotron-3-super-120b-a12b-free` — code review, edge cases, QA pass
  (spec §22 Rule B / Rule D).
* `nemotron-3-ultra-550b-a55b-free` — long-horizon architecture problems
  (use sparingly).
* `laguna-s-2.1-free` — backup.

Per spec §22 Rule E, **never pretend a model was consulted** when it
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
