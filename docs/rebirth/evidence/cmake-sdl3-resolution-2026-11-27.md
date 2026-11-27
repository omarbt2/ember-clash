# CMake SDL3 Dependency Resolution — 2026-11-27

| Field | Value |
|---|---|
| Purpose | Make the SDL3 dependency usable with an installed package or a local exact-version source tree, without changing the dependency pin. |
| Revision date | 2026-11-27 |
| Status | PARTIALLY VERIFIED — CMake dependency-selection branches pass fixture validation; full SDL3 application build remains blocked until the real SDL3 source/package is available. |
| Required dependency | SDL3 `3.2.10` (`release-3.2.10`) |

## Root cause

A clean configure stopped inside CMake `FetchContent_MakeAvailable(SDL3)` while cloning `https://github.com/libsdl-org/SDL.git`. The environment returned `Could not resolve host: github.com`. CMake had not reached any Ember Clash C++ translation unit, so this failure is dependency acquisition/network DNS, not evidence of a source compilation error.

## Changes made

- Native builds now look for an exact `SDL3 3.2.10` CMake package first. An arbitrary newer SDL3 package is not accepted by this path.
- `EMBER_SDL3_SOURCE_DIR` allows an existing SDL source checkout to be used without network access.
- The source-directory override is checked for the expected CMake project/header layout and the `SDL_MAJOR_VERSION`, `SDL_MINOR_VERSION`, and `SDL_MICRO_VERSION` values `3`, `2`, and `10` before FetchContent is configured.
- If neither an exact native package nor an explicit source tree is available, the original pinned FetchContent path remains the fallback.
- The verified-archive build helper now uses the public `EMBER_SDL3_SOURCE_DIR` option after hash validation.
- README build guidance documents installed-package and offline-source workflows.

## Verification performed

| Check | Result | Scope |
|---|---|---|
| CMake configure with a fixture SDL source tree reporting 3.2.10 | PASS | Exercises the explicit local-source branch and project generation; fixture is not the SDL implementation. |
| CMake configure with a fixture installed SDL3 3.2.10 package | PASS | Exercises exact-version package discovery and project generation; fixture is not the SDL implementation. |
| CMake rejects a fixture source tree reporting 3.2.11 | PASS | Confirms an incompatible source version is rejected before FetchContent. |
| `ember_test_move_content` GCC strict build and run | PASS | `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror` |
| `ember_test_p3_move_scenarios` GCC strict build and run | PASS | Same strict flags |
| Both P3 content tests with Clang strict flags | PASS | Same strict flags |
| `bash -n tools/build_with_verified_sdl.sh` | PASS | Shell syntax only |
| `python3 tools/check_rebirth_docs.py` | PASS | 69 Markdown files checked in the final run |
| `git diff --check` | PASS | Whitespace validation |

## Remaining build gate

No real SDL3 3.2.10 source archive, installed SDL3 package, or SDL3 headers/library were present in this execution environment. A normal no-cache configure still cannot download SDL3 while GitHub DNS is unavailable. Therefore a full application compile, full CTest run, and desktop runtime test have **not** been verified by this change.

On an offline machine, provide the exact verified SDL 3.2.10 source tree:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DEMBER_SDL3_SOURCE_DIR=/path/to/SDL-release-3.2.10
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Alternatively, place the previously measured codeload archive at a local path and run `tools/build_with_verified_sdl.sh /path/to/SDL-release-3.2.10.tar.gz`. The script enforces the recorded archive SHA-256 before extracting or building. The recorded hash is a transfer-integrity fingerprint measured for that archive, not a claim that SDL publishes it as its official checksum.
