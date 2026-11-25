# Ember Clash - Rebirth Recovery Snapshot

> **Historical snapshot:** the first section below records the earlier `rebirth/phase0` reset state. It is not the latest recovered source state. The later source bundle contains branch `dev/rebirth-continue` at `2eeceae`; the production plan and the 2026-11-25 recovery checks are recorded under `docs/rebirth/`.

**Honest summary:** this is a snapshot of the real working directory at the moment of recovery.
It is NOT the full Rebirth build. A sandbox reset destroyed the earlier Rebirth work
(local commits, never pushed) and it has not been recovered. Only a partial re-application of the
combat work survived here, uncommitted.

## Git state at snapshot time
- Branch: `rebirth/phase0` (local only, no upstream)
- Base commit: `bd71c89` (original master as first cloned)
- Remote `origin/master` at snapshot: `365348a` (4 docs-only commits ahead of the base:
  README rewrite, AGENTS.md + orchestrator prompt removed, .gitignore). A trial merge was conflict-free.
- Commits created locally since the reset: **0**
- GitHub push: **NOT PUSHED** (no credentials in the sandbox; browser login was started but not completed when this ZIP was built)
- The `.git` directory is included, so the branch, base commit and uncommitted state are preserved.

## What is included
Complete project tree (tracked + untracked), including `.git`, minus build outputs:
src/, tests/, docs/, web/, tools/, CMakeLists.txt, scripts, all assets in the repo.

### Uncommitted modifications (9 files)
CMakeLists.txt (CTest integration + ember_screenshot target), src/main.cpp (portable main()),
src/Gameplay/Attack.{h,cpp} (13 new moves, new intent fields), src/Gameplay/CombatConfig.h (guard/parry/dodge/Surge constants),
src/Gameplay/Fighter.{h,cpp} (guard, parry, shield, dodge, Ember Surge, directional attack resolution, single-hit-per-swing),
src/Physics/Motion.{h,cpp} (up_held, MotionTuning).
### Untracked
tools/ember_screenshot.cpp - currently a **placeholder stub** (`int main(){return 0;}`), not the real screenshot tool.

## Incomplete / not done
Match.cpp + InputMap integration for the new combat fields (so the new moves are not reachable from input yet),
roster, stages, character art, audio mixer/music, VFX, modes, menus, 13-key rebinding, gamepad control of fighters.
All of these existed and passed tests before the reset but are NOT in this snapshot.
No Windows or WebAssembly build was verified. No audible-audio or real-gamepad verification.

## Build and test (Linux; verified at snapshot)
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ctest --test-dir build --output-on-failure
Result at snapshot: build succeeds with no errors; **13/13 existing test suites pass** (the original 13).
No new tests exist in this snapshot. Needs CMake >= 3.20, a C++20 compiler; SDL3 is fetched by CMake (network needed on first configure).

## Later recovery state (2026-11-25)

The supplied recovery package also contains a later source bundle at `dev/rebirth-continue` (`2eeceaea2452164e4fe8c064d8e0572ce6678b5e`) and the complete Production Bible. That later source already wires the 13-key layout into gameplay and includes the rebirth input regression suite; therefore the "not yet wired" list above describes only the earlier reset snapshot.

The 2026-11-25 recovery execution prepared additional fixes for persistent key state, invalid keycode handling, the complete 13-action Settings UI, dynamic Controls labels, and live binding refresh. Its isolated checks passed, but full SDL3 configuration/build and CTest were blocked by unavailable GitHub DNS. See `docs/rebirth/evidence/2026-11-25-input-and-menu-recovery.md` for exact evidence. The production roadmap remains unfinished; do not treat this snapshot note as a completion claim.
