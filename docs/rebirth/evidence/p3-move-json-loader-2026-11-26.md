# P3 Evidence: schema-1 move JSON loader and validators

| Field | Value |
|---|---|
| Purpose | Evidence for the first ordered task in P3 Combat Engine: load and validate move content. |
| Status | PARTIALLY VERIFIED — strict headless tests pass; runtime migration and full SDL3 build remain open |
| Revision date | 2026-11-26 |

## Implemented

- Added a bounded C++20 JSON parser (`src/Data/Json.*`) with strict JSON number/array/object grammar, UTF-8 validation, escaped Unicode/surrogate decoding, duplicate-key rejection, raw UTF-8 validation, 4 MiB input limit, and 64-level nesting limit.
- Added schema-1 move catalog types and loader (`src/Data/MoveContent.*`). Validation covers required fields, IDs, categories, timing/ranges, active-window keyframes, shapes/geometry, damage/knockback fields, optional cancel windows, duplicate IDs, cancel-target resolution, and explicit warnings for unknown fields.
- Added a standalone keyframe evaluator (`src/Data/HitboxEngine.*`) with inclusive tick windows, facing-mirrored world geometry, AABB overlap, and true circle-vs-AABB distance checks.
- Parsing and file loading are transactional: failed loads do not overwrite the caller's existing catalog.
- Added `assets/data/moves.json` as a starter catalog containing the 16 move IDs currently implemented in `AttackId` (excluding `None`).
- Added `tests/ember_test_move_content.cpp` and registered the data-loader sources in CMake.

## Verification performed

- `ember_test_move_content` compiled with GCC using `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror` and passed.
- The same test compiled with Clang using the same strict flags and passed.
- The test loads the checked-in starter catalog, checks the current `AttackId` count, exercises cancel references and AABB/circle keyframes, and rejects unsupported schema, missing/invalid fields, duplicate IDs/keys, invalid UTF-8, malformed numbers/trailing commas, out-of-window hitboxes, and unresolved cancel targets.
- The test verifies that a failed parse preserves an already-loaded catalog.
- The same GCC/Clang test exercises keyframe start/end ticks, left/right mirroring, invalid facing rejection, AABB overlap, and circle-vs-AABB hit/miss cases.

## Explicit limitations / open items

- `Attack.cpp` remains the runtime source of truth. The new keyframe evaluator is tested standalone but is not yet wired into `Match::resolve_hits`; runtime catalog selection, attack stat sourcing, multi-hit groups/rehit rules, and deterministic rollback integration remain open.
- P3's stated target of 19 moves does not match the currently verified 16 executable `AttackId` entries. The three missing moves have not been invented.
- Unknown JSON fields are tolerated for forward compatibility and reported through `MoveCatalog::warnings`.
- D-010 (JSON content + C++ hooks) remains PROPOSED pending owner review.
- Full CMake/CTest and SDL3-linked application build remain blocked by the runner's inability to resolve the SDL3 FetchContent host. No full-build claim is made.
