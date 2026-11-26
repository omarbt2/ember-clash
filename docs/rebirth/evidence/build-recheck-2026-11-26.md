# S0 Build Unblock and Source Reconciliation Recheck — 2026-11-26

| Field | Value |
|---|---|
| Purpose | Record the current S0 build gate and source/remote reconciliation evidence. |
| Revision date | 2026-11-26 |
| Status | `BLOCKED` — SDL3 FetchContent cannot reach GitHub because DNS resolution fails |
| Working branch | `work/s0-build-unblock-2026-11-26` (local only) |
| Bundle/document HEAD | `42fe5ad9b667f0aca1d2f61a13d7382be0e3fd08` |
| Selected code snapshot | `1e3175aba6e6d6fcf6237016ad3fe35119165efe` |
| Runtime/toolchain | Linux; Git 2.47.3; CMake 3.31.6; Ninja 1.12.1; GCC 14.2.0; Clang 17.0.0 |

## 1. Recovery artifact integrity

Selected the newest matching 2026-11-26 artifact set from the Library:

- `Ember-Clash-Rebirth-Execution-Roadmap-2026-11-26.zip` — 3,919,403 bytes; SHA-256 `611a78ee95f6155b23cc21c34ee1b9947efe9904c787953d604dd740193825ac`.
- `Ember-Clash-Rebirth-Execution-Roadmap-2026-11-26.bundle` — 7,239,955 bytes; SHA-256 `868f7e9395fa2a7bf9a39d42893d69bc91b3f8b93b9e5e253c2c32f4df8cd5c3`.
- `Ember-Clash-Rebirth-Execution-Roadmap-2026-11-26.patch` — 133,407 bytes; SHA-256 `9c5ea13929e9803af894e0222781ea4b756f9d11424494b55b5ff3079e02489f`.

Verification:

- `unzip -t ...zip` — PASS; no archive errors.
- `git bundle verify ...bundle` — PASS; Git reports a complete-history bundle.
- `git bundle list-heads ...bundle` — branch and HEAD resolve to `42fe5ad...`.
- `git fsck --full` on a clone from the bundle — PASS; no reported object errors.
- ZIP extraction and the bundle's HEAD tree contain the same 181 files, with zero content mismatches.
- `42fe5ad` is documentation-only. The implementation checkpoint remains `1e3175a` (`ci: add reproducible wasm release build gate`); later bundle commits update roadmap/evidence documentation, not gameplay implementation.

Patch handling:

- `git apply --check <patch>` — not applicable forward: files already exist and hunks overlap the recovered tree.
- `git apply --reverse --check <patch>` — PASS.
- Therefore the patch's changes are already represented in the selected bundle. It was not applied again.

## 2. Current GitHub refs and ancestry reconciliation

The GitHub connector and public Git refs returned:

| Remote branch | Current SHA |
|---|---|
| `master` | `365348a2dacea4e03362bfd1fd718582361ba693` |
| `rebirth/phase0` | `7fddfde7b909b2fa338bc9095b29f30bb0031bf9` |
| `reconstruction/rebirth-input-recovery` | `3026c02757a5bffc6e894fdb1309eb7219d6f1c7` |

Important lineage findings:

- The local `1e3175a` source snapshot is descended from the remote `rebirth/phase0` commit `7fddfde` and contains the remote `master` commit `365348a` in its ancestry through merge commit `9f05a04`.
- Local checks `git merge-base --is-ancestor 7fddfde... 1e3175a` and `git merge-base --is-ancestor 365348a... 1e3175a` both passed.
- The local code work after the `rebirth/phase0` checkpoint changes 112 files (`6,504 insertions`, `2,762 deletions`), including input recovery, movement, combat/AI/training foundations, production documentation, tests, asset validation and CI configuration.
- The current remote branch named `reconstruction/rebirth-input-recovery` at `3026c02` is not present as a commit object in the bundle and is not the same ref/history as the locally recovered branch with that name. Comparing remote `rebirth/phase0` and remote `reconstruction/rebirth-input-recovery` shows divergence from merge base `bd71c89`; branch names alone are not a safe basis for pushing or merging.
- No remote branch was modified. No push, PR or merge was attempted.

The local branch is the selected continuation source because it has the complete verified bundle history and descends from both the Phase 0 recovery snapshot and the master history. Preserve the remote `3026c02` branch separately until a full tree/ancestry reconciliation is available.

## 3. CI workflow inspection

The recovered bundle contains `.github/workflows/ci.yml` with four jobs:

- `docs` — production bible consistency check.
- `linux-gcc` — Linux Release configure/build and headless CTest.
- `windows-msvc` — Windows Release build and CTest.
- `webassembly` — Emscripten 4.0.23 Release build and artifact checks.

`python3` YAML parsing with `yaml.BaseLoader` — PASS; all four job names were present.

The GitHub content API returned HTTP 404 for `.github/workflows/ci.yml` on each of the three current remote refs (`master`, `rebirth/phase0`, and `reconstruction/rebirth-input-recovery`). The workflow in this bundle is therefore not verified as published on those remote refs. `fetch_commit_workflow_runs` was also checked for the three remote SHAs plus local `1e3175a` and bundle HEAD `42fe5ad`; it returned no PR-triggered workflow runs for any of them. That connector wrapper only returns PR-triggered runs and the first page, so this does not rule out other event types or historical runs outside its scope.

## 4. S0 — SDL3 Build Unblock

The build pins SDL3 to `release-3.2.10` in `CMakeLists.txt` via CMake `FetchContent`; the dependency version was not changed.

Environment checks:

- `pkg-config --modversion sdl3` — SDL3 not installed.
- Standard system SDL3 headers and CMake package paths checked — missing.
- No local SDL3 source checkout/cache was found in the inspected build/cache locations.

Exact configure command:

```bash
cmake -S /mnt/data/ember-clash-recovery-inspection/from-bundle \
  -B /mnt/data/ember-clash-recovery-inspection/build-s0-recheck \
  -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Result: exit code `1`. CMake identified GCC 14.2.0 and completed compiler ABI detection, then failed while cloning `https://github.com/libsdl-org/SDL.git` at `release-3.2.10`:

```text
fatal: unable to access 'https://github.com/libsdl-org/SDL.git/': Could not resolve host: github.com
CMake Error ... Failed to clone repository: 'https://github.com/libsdl-org/SDL.git'
```

A separate `git ls-remote https://github.com/omarbt2/ember-clash.git` also failed with `Could not resolve host: github.com` (exit `128`). This is a network/DNS dependency-fetch blocker, not a C++ compilation failure. Configure stopped before project compilation; full game build and CTest were not run.

## 5. Verification actually run in this recheck

| Command/test | Result | Scope/limitation |
|---|---|---|
| `python3 tools/check_rebirth_docs.py` | PASS — 61 Markdown files | Documentation consistency only |
| `python3 -m unittest discover -s tools/tests -v` | PASS — 7 tests | Asset-manifest validation only |
| `ember_test_attack_table` built with GCC C++20 strict warnings | PASS — compile and execute, exit 0 | Attack-table logic only |
| `ember_test_ai_presets` built with GCC C++20 strict warnings | PASS — compile and execute, exit 0 | AI preset data only |
| `ember_test_training_recorder` built with GCC C++20 strict warnings | PASS — compile and execute, exit 0 | Bounded recorder utility only |
| `ember_test_rebirth_motion` built with GCC C++20 strict warnings | PASS — compile and execute, exit 0 | Platform-neutral movement step logic only |
| Recovered CI YAML parse | PASS — four jobs found | Syntax only; no Actions run |
| `git bundle verify` + `git fsck --full` | PASS | Git object/bundle integrity |
| ZIP-to-bundle HEAD file comparison | PASS — 181/181 files, zero mismatches | Archive/source snapshot equivalence |
| CMake Release configure | BLOCKED — exit 1 | SDL3 fetch blocked by DNS before game compilation |
| Full CMake build / CTest | NOT RUN | Configure did not complete |
| Windows, WebAssembly runtime, real window/audio/gamepad and 4P | UNVERIFIED | Requires successful target build and runtime/device tests |

The strict standalone C++ test compile flags were `-std=c++20 -Wall -Wextra -Wpedantic -Werror -I src`. Their individual commands and logs are retained under `/mnt/data/ember-clash-recovery-inspection/isolated-tests/` in this session.

## 6. Next executable task

`S0` remains `BLOCKED`. Resume only when an environment with working GitHub DNS/network or a separately supplied, provenance- and hash-verified SDL3 `release-3.2.10` source cache is available. Then rerun clean Release configure, complete build, and `ctest --output-on-failure` while preserving the pin. Do not substitute another SDL release, call isolated tests a full build, or claim CI green.

After the build gate is reproducible, continue P0: compare full trees/ancestry for all remote refs, publish the entire recovered history to a new remote branch only after verifying the exact source tree and ancestry, then run GitHub Actions and open a review PR without merging.


## 7. Tool availability and execution boundaries

- `Flowork` is not present in the current connected tool catalogue.
- No dedicated DaVinci `Resolve` integration is present; the catalogue entries containing the word `resolve` are unrelated helpers for GitHub/Notion/Canva threads or plugin-specific identifiers.
- No subagent spawn/wait/list tools are available in this session. No parallel agent was claimed or launched.
- No external art generation, paid provider operation, or asset purchase was started. D-004 remains open and the project has not passed its source/build baseline gate.
