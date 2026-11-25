# Ember Clash Rebirth — recovery checkpoint

**Status: IN PROGRESS — NOT A FINISHED GAME OR RELEASE.**  
Date: 2026-11-25

## Recovery sources and baseline

The recovery archive contains the C++20/SDL3 source, Git history, and the separate 55-document Production Bible. The recovered development source tip is `2eeceaea2452164e4fe8c064d8e0572ce6678b5e` (13-action input-map work). The public GitHub `master` tip inspected during recovery was `365348a2dacea4e03362bfd1fd718582361ba693`.

## Work implemented in the companion recovery snapshot

- Completed the 13-action keyboard remapping path through menu display, capture, persistence, and validation for both player profiles.
- Distinguished extended SDL keycodes (arrows, keypad, modifiers) without index aliasing; made held-key queries reflect persistent held state.
- Recovered safely from malformed persisted bindings and corrected the `P` dodge / pause conflict.
- Made Escape and gamepad Start pause/resume behavior consistent across match and menu transitions.
- Added focused input/menu regression tests, removed a no-op screenshot target, updated control documentation, and added Linux/Windows CI configuration.
- Added a dated recovery evidence note and a Production Bible consistency checker.

The companion Git worktree archive and Git bundle contain these local commits:
- `054d1b4` — `fix: complete 13-action keyboard remapping and pause flow`
- `549efcc` — `docs: record recovery evidence and add release CI`

## Verification

- Production Bible consistency checker: PASS (56 Markdown files; links, headers, constants, phases, dependencies, estimates).
- Focused input/settings executable probes: PASS (17 assertions).
- Menu layout and current-binding probe: PASS (39 rows / 26 remappable actions).
- Pause/resume transition probe: PASS (Escape and gamepad Start).
- Strict syntax-only compilation of modified menu and test translation units with a local SDL compatibility shim: PASS.
- Full CMake/SDL3 configure, build, and CTest: **NOT RUN TO COMPLETION**. CMake FetchContent could not resolve `github.com` to download SDL3 in the execution environment. No full build result is claimed.
- CI workflow was added to the companion snapshot but has not yet run on GitHub.

## GitHub publication boundary

This checkpoint file is committed on `reconstruction/rebirth-input-recovery`. The source/test/CI changes above are present in the companion local Git bundle and patch archive, but have **not yet been applied to the GitHub branch tree**. Direct Git push from the execution environment was blocked by network DNS resolution. `master` was not modified.

## Remaining production work

The Production Bible is a plan, not proof of completion. Its P0–P14 roadmap remains open. Among the major remaining deliverables are combat-system breadth (grabs, throws, projectiles, DI, ledge/recovery mechanics), authored character/stage art and animation, multiple finished arenas, complete menus and match modes, local multiplayer verification, AI/training, controller verification, performance/release packaging, and the later online/ranked milestones. Owner decisions D-001 through D-020 also remain open/proposed in the recovered planning set.

Do not mark the game complete, release-ready, or any remaining milestone green until its acceptance criteria and manual verification are satisfied.
