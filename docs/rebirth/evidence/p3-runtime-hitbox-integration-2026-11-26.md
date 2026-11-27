# P3 Evidence: Match runtime consumes validated move content

| Field | Value |
|---|---|
| Purpose | Connect the schema-1 move catalog and keyframe evaluator to the fixed-tick Match hit resolver. |
| Status | PARTIALLY VERIFIED — strict compiler checks and standalone content tests pass; SDL-linked Match integration tests remain blocked by the runner's incomplete SDL stub. |
| Revision date | 2026-11-26 |

## Implemented

- `Match` loads `assets/data/moves.json` at construction (working-directory path first, source-tree fallback for developer/test builds) and reports a warning with legacy hitbox fallback if loading fails.
- Added an explicit `AttackId` → content ID map for the 16 currently implemented moves. No missing moves were fabricated.
- During each fixed simulation tick, Match derives the active keyframe hitboxes from the attack frame and fighter pose, then uses the existing ordered attacker/defender loop and defensive outcome logic. AABBs and circles use the shared evaluator.
- Hit damage, hitstun, base knockback and knockback growth are read from the contacted keyframe. Knockback uses the documented content formula with neutral reference weight 100 because per-fighter weights are not implemented yet.
- Content bytes receive an FNV-1a fingerprint. Match state hashes include it, and snapshot format v2 rejects snapshots made against a different move catalog.
- Updated the starter catalog's `kb_scale` values to retain the legacy base-plus-damage-growth tuning at reference weight 100.

## Verification

- `Match.cpp` and the updated snapshot test compile in strict warning-as-error syntax checks with GCC and Clang.
- The move catalog + hitbox-engine test was rebuilt from current sources and passed with GCC and Clang.
- `tools/check_rebirth_docs.py` passed (67 files checked).
- Attempted CMake build of `ember_test_combat_resolution`; it stopped before project compilation because the temporary SDL stub lacks `SDL3/SDL_gamepad.h`. This is not a successful Match integration test and must be rerun with the real SDL3 development package.

## Open limitations

- Runtime data is now used, but multi-hit groups/rehit windows, per-fighter weight, cancel execution, full 19-move roster, projectiles/traps/grabs/throws, armor/clank/staleness and scenario DSL are not implemented.
- The app's packaged asset-root strategy must be validated on Windows and Web builds; the source-tree fallback is for development/test use.
- D-010 remains PROPOSED; this implementation does not mark the decision approved.
- P3 exit gate and full CTest remain open.

## Runtime asset-root continuation (2026-11-26)

- Added deterministic move-catalog path discovery in this order: launch working directory, executable directory, then compiled source-tree fallback. The resolver returns the working-directory candidate when nothing exists so the loader retains a useful error path.
- `Match` now asks SDL3 for the executable base path and uses the resolver instead of assuming its current directory is the repository root.
- Desktop post-link and install layouts copy `assets/data/moves.json` beside the executable under `assets/data/`. The web build embeds the JSON into the Emscripten virtual filesystem and copies the same file into `web/assets/data/` for the deploy bundle.
- Added real temporary-file tests for each resolver precedence branch and a 16-move frame scenario test covering exact active start/end boundaries, geometry, payload fields, contact, edge miss, and facing mirror.

Verification in this continuation:

- `ember_test_move_content` rebuilt and run with GCC 14 and Clang: PASS.
- `ember_test_p3_move_scenarios` rebuilt and run with GCC and Clang: PASS.
- `cmake -P cmake/CopyMoveCatalog.cmake` copied the catalog to a temporary runtime layout; byte comparison passed (SHA-256 `b76c28f320fcdba31bce8897f039c70ce4beb2efe13e6a1bdef565ffb85335b7`).
- `tools/check_rebirth_docs.py`: 68 files checked, PASS; `git diff --check`: PASS.
- Full SDL-linked application / CTest remains unverified in this runner because SDL3 headers and the fetched SDL source are unavailable. The new `Match.cpp` SDL base-path call therefore still requires a real SDL3 build before it is considered compile-verified.
