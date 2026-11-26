# P2 fixed-step, input-edge and collision recheck — 2026-11-26

| Field | Value |
|---|---|
| Purpose | Evidence for the local P2 fixed-step, input-edge and collision continuation. |
| Status | PARTIALLY VERIFIED — D-001 replay gate not passed; P2 remains open |
| Working branch | `work/s0-build-unblock-2026-11-26` (local only) |
| Pre-change source checkpoint | `6ea466912036c808821b58a39cd0104d1860654a` |
| Source of original replay reference | `2eeceaea2452164e4fe8c064d8e0572ce6678b5e` |
| Toolchain | GCC 14.2.0, Clang 17.0.0; Linux x86_64 |
| Revision date | 2026-11-26 |

This note records a working-tree continuation. It does not constitute owner approval of D-001/D-002, a completed P2 gate, a full SDL3 build, or release certification.

## 1. Changes exercised

- Added `src/Core/FixedStep.h`: one bounded 60 Hz accumulator shared by Desktop and Web. It clamps invalid/large frame deltas, executes at most three simulation ticks per render frame, drops excess whole-tick backlog, and preserves fractional remainder.
- Added `src/Core/InputEdgeState.h`: keyboard and gamepad transitions remain latched until a simulation tick consumes them, including a brief press and release between render frames.
- Routed Desktop `App::run()` and Web `requestAnimationFrame` through `App::step_frame()`. Web `App` now has static storage because `emscripten_set_main_loop(..., simulate_infinite_loop=false)` returns from `main()` while browser callbacks continue.
- Removed `dt` from the current simulation API path (`step_motion`, `Fighter::update`, `Match::update`, AI `decide`). Respawn delay, post-respawn invulnerability, and KO banner timers now use integer tick counts (30, 90, and 90).
- Added four-sided, arena-owned blast-zone bounds and bounded movement/collision substeps (maximum 16 px) to prevent fast knockback from tunnelling through thin platforms. Added finite-value handling for invalid velocity inputs.
- Expanded `Match::state_hash()` to include exact IEEE-754 float/double representations and more movement, combat, AI/RNG, arena, training, and match-flow state. Replaced shared process RNG for KO shake with per-match deterministic xorshift state; reset clears hit-stop and KO-shake state.
- Added a `std::nullptr_t` overload to `Match::set_ai` so users can clear an AI controller without requiring its complete type in every including translation unit.
- Removed genuinely unused fields/constants exposed by Clang strict warnings (`Fighter::color_`, the HUD's `kCardHeight`, stale `Menu` settings fields, and dead test-only helpers/counters). No compiler-warning suppression was added for these findings.

## 2. Automated verification

The following 25 current headless C++ suites were rebuilt from the current source and executed successfully:

1. `ember_test_ai`
2. `ember_test_ai_presets`
3. `ember_test_attack_table`
4. `ember_test_combat_resolution`
5. `ember_test_fixed_step`
6. `ember_test_input_edge_latches`
7. `ember_test_m11`
8. `ember_test_m12`
9. `ember_test_m125_audio`
10. `ember_test_m14`
11. `ember_test_m5`
12. `ember_test_m6`
13. `ember_test_m6b`
14. `ember_test_m6c`
15. `ember_test_m7`
16. `ember_test_p15_playability`
17. `ember_test_p17_playability`
18. `ember_test_p2_blast_zones`
19. `ember_test_p2_no_tunnel`
20. `ember_test_p7_arenas`
21. `ember_test_p8_input_events`
22. `ember_test_rebirth_input`
23. `ember_test_rebirth_motion`
24. `ember_test_training_match`
25. `ember_test_training_recorder`

Results:

- GCC 14 strict C++20 build/run: **25/25 PASS** (`-Wall -Wextra -Wpedantic -Werror`).
- Clang 17 strict C++20 build/run: **25/25 PASS** with the same warning policy.
- GCC 14 AddressSanitizer + UndefinedBehaviorSanitizer build/run: **25/25 PASS**, leak detection enabled; no sanitizer report.
- Documentation consistency checker: PASS (updated repository count; see current run log).
- Asset/tool Python unit tests: PASS, 7/7.
- `App.cpp` strict compile and the Web entry-point syntax check passed against local declaration-only SDL/Emscripten compatibility shims. The M8 compile-time test passed.

**Shim boundary:** the local compatibility headers are test scaffolding for declarations and headless tests. They do not replace SDL3, prove the full link, or provide window/audio/gamepad runtime coverage. The Web syntax check is not an Emscripten WASM build; `emcc` is not installed in this runner.

## 3. D-001 replay gate — NOT PASSED

The documented requirement asks for 20 scripts (60–3600 ticks), per-tick position tolerance 0.01 px, and identical event ticks. A replay harness ran 20 scripted scenarios of 600–3600 ticks (37,200 rows total per run) against the pre-change local checkpoint `6ea4669` and the current source.

Two first divergences were identified:

- Initial KO/respawn scenario: the first run exposed a real ordering defect. The KO-detection update set the 30-tick timer and then immediately decremented it, effectively counting the KO tick as part of the wait. The fixed implementation records `died_this_tick` and skips that first decrement. The policy is now explicit: respawn occurs after **30 complete simulation ticks following the KO-detection tick**. The regression in `ember_test_m5` checks the timer is still 30 immediately after KO, remains pending through 29 subsequent ticks, and respawns on the 30th subsequent tick.
- Rebuilt the current replay harness after this correction and compared 20 scripted cases (37,200 rows) against the controlled legacy replay with equivalent blast-zone rules. Result: **0 event-field mismatches**; maximum difference across P1/P2 x/y positions was `0.0001220701 px`, below the required `0.01 px` tolerance. This controlled comparison now passes the documented positional/event criterion.
- A separate no-initial-KO replay against the unmodified legacy arena still diverges at tick 213: the new four-sided blast-zone rules cause a side ring-out/respawn, whereas the old implementation only had the lower death boundary. This is an intentional gameplay-rule expansion and must be reported separately from the fixed-timestep equivalence result; it is not evidence of a timer regression.

The plan's original replay reference `2eeceae` predates later P2/P3 code already present at the pre-change `6ea4669` checkpoint. The replay was therefore split into a controlled timer/fixed-step comparison and a separately documented blast-zone behavior comparison, rather than treating unrelated feature changes as timing regressions.

**Decision state:** the controlled replay criterion now passes for the tested harness and the respawn event policy is documented in code/tests. D-001 remains PROPOSED pending owner review/sign-off and the broader numeric/cross-platform gates. Do not merge or tag P2 complete based on this partial replay scope alone.

## 4. Remaining P2 requirements

- Owner review/sign-off of D-001's now-explicit convention: respawn after 30 complete simulation ticks following the KO-detection tick. Controlled legacy replay evidence passes the required tolerance/event check; four-sided blast-zone behavior is separately identified as an intentional rules delta.
- Complete D-002 across Windows/MSVC and Emscripten/WASM/Node, repeat the required independent builds, and only then decide constrained float versus fixed-point.
- Complete the POD `MatchState` snapshot and prove snapshot/hash completeness; `state_hash()` expansion is not a POD rollback snapshot.
- Implement/verify remaining ledge, DI, and tech mechanics; complete broader move/movement edge-case matrices.
- Run full Release configure/build/CTest with the actual pinned SDL3 dependency, then native Desktop/Web builds and manual feel tests with two testers.
- Validate on Windows and real SDL3/Web runtime, including actual input, window, audio, and controller devices.

## 5. Build boundary

A clean CMake Release configure was retried and remains blocked before project compilation while FetchContent tries to clone the pinned SDL3 `release-3.2.10` source. The environment reports `Could not resolve host: github.com`. No full CMake build or CTest result is claimed. The pinned version was not changed.
