# Ember Clash - Rebirth Recovery Snapshot

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

## How to resume
1. Unzip, `cd Ember-Clash-Rebirth-Recovery`, `git status` to see the uncommitted work.
2. `git fetch origin && git merge origin/master` (conflict-free at snapshot time).
3. Commit, then push to a branch: `git push -u origin rebirth/phase0`  (never force-push master).
4. Re-apply the lost Rebirth work in small tested commits, pushing after each.
