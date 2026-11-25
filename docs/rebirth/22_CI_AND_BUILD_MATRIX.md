# CI and Build Matrix

| Field | Value |
|---|---|
| Purpose | Planned CI. Not configured or run in this task (UNVERIFIED). |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 21, 23 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

| Job | Runner | Steps | Artifacts |
|---|---|---|---|
| linux-gcc | ubuntu-24.04 | configure Release, build, ctest (dummy drivers) | binaries, logs |
| linux-clang-asan | ubuntu-24.04 | ASan/UBSan debug build + ctest | sanitizer logs |
| windows-msvc | windows-latest | cmake -G "Visual Studio 17 2022", build, ctest | .exe zip |
| wasm | ubuntu + emsdk | emcmake build, run determinism test in node | .wasm bundle |
| determinism | combos above | compare replay hashes | hash report |
| docs | ubuntu | `tools/check_rebirth_docs.py` | report |
Dependency handling: SDL3 pinned (`release-3.2.10`) through FetchContent with a cache keyed by version. Warnings: `-Wall -Wextra` as errors in CI. Static analysis: clang-tidy (selected checks), cppcheck. Release artifacts: tagged builds with packaged zip per platform. Compatibility check: the existing CMake already has Emscripten entry (`main_emscripten.cpp`) - build UNVERIFIED.
