# P2 Evidence: DI, tech, ledge recovery and MatchState snapshots

| Field | Value |
|---|---|
| Purpose | Record the P2 recovery mechanics and deterministic Match snapshot/restore implementation. |
| Status | PARTIALLY VERIFIED — isolated headless tests pass; official SDL3 build remains blocked |
| Revision date | 2026-11-26 |

## Implemented

- **Directional influence (DI):** while airborne in `Hitstun`, digital input can redirect the launch vector by at most 15° from the original knockback direction. It preserves current speed and cannot increase the downward component toward the stage. No input means no DI. It is not applied during ordinary movement, grounded hitstun, or unrelated states.
- **Tech:** a shield-button press edge during airborne hitstun is buffered for eight simulation ticks. If the fighter lands while the buffer is live, the fighter enters a ten-tick grounded tech recovery and can roll in the held horizontal direction. A dodge edge alone does not trigger a tech. Walking off the supporting surface ends the grounded tech state.
- **Ledge recovery:** only platforms explicitly marked `ledge_grabbable` can be caught, and the fighter must be falling while facing inward. Built-in arenas currently mark the main stage floor as grabbable and elevated side platforms as non-grabbable. The first grab per airtime grants 40 ticks of invulnerability. A hang can be exited by jump, up/climb, steering away, or a 180-tick timeout. Jump/drop starts a 12-tick regrab lockout.
- **Value snapshots:** `Fighter::Snapshot` and `Match::Snapshot` are trivially-copyable value types. Match snapshots include the arena id, both fighter states (including tech/ledge timers and animation-driver state), attached AI difficulty/runtime/RNG, training recorder contents/cursor/flags, respawn timers, match phase and intro/live counters, hit feedback, hit-stop, and KO-shake RNG/state.
- **Restore safety:** `Match::restore_state` checks the version, enum/range/finite-value constraints, creates and validates replacement arena/fighters/AI/recorder first, then swaps them into the live match. A rejected snapshot leaves the current match unchanged.
- **State hash:** includes DI launch reference vectors, ledge/tech/invulnerability state, animation driver pose/timing, recorded shield-press edges, and both one-way/ledge-grabbable platform properties. AI restore preserves the exact captured RNG state, including zero.

## Verification performed

Strict C++20 builds used `-O2 -Wall -Wextra -Wpedantic -Werror`. Match-based tests used a temporary minimal SDL3 header shim and test-only `InputManager` adapter because SDL3 is unavailable in this runner.

| Check | GCC | Clang |
|---|---:|---:|
| New DI/tech/ledge headless test (15° DI cap, no groundward steer, shield tech window/final hitstun tick, ledge facing/invulnerability/regrab) | PASS | PASS |
| New Match snapshot/restore + same-next-tick hash test (atomic invalid-version/animation rejection, AI RNG, recorder, next-tick replay hash) | PASS | PASS |
| Existing blast-zone test | PASS | PASS |
| Existing no-tunnelling test | PASS | PASS |
| Existing one-way/drop-through test | PASS | PASS |
| Existing motion test | PASS | PASS |

Additional GCC integration regressions passed on the updated sources: combat-resolution, M5 (all scenarios), M6-C (67 assertions), M7-AI (14 assertions), training-match, and P15-playability (17 assertions). The existing blast-zone, no-tunnelling, one-way/drop-through and motion tests passed with both GCC and Clang; attack-table and training-recorder tests also passed with both compilers. `python3 tools/check_rebirth_docs.py` passed with 66 files checked.

This is isolated source/link evidence, **not** the official SDL3 build and does not replace CMake/CTest. The full project configure/build remains unverified because the runner cannot resolve the SDL3 FetchContent host.

## Still open

- Full CMake configure/build and full CTest against the project SDL3 dependency.
- Real Windows and Emscripten/WASM CI runs and cross-platform hash comparison.
- Review of D-001 / D-002 and owner approval.
- Two-person movement feel session and tuning review.
- P2 exit gate remains open; do not tag P2 complete yet.
