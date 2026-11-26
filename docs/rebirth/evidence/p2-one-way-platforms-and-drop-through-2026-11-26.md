# P2 Evidence: One-way platforms and drop-through

| Field | Value |
|---|---|
| Purpose | Record implementation and focused verification for P2 one-way platform behavior |
| Status | PARTIALLY VERIFIED — isolated native tests pass; full application build remains blocked by SDL3 dependency acquisition |
| Revision date | 2026-11-26 |

## Change

- Added explicit `Platform::one_way` collision semantics.
- Elevated platforms in Training Ring, Ember Foundry, and Sky Temple are one-way; each arena's main floor remains solid.
- One-way platforms catch descending bodies only when their feet were above the platform at the start of the current collision substep.
- Upward movement from below passes through one-way platforms.
- A grounded fighter can press down + jump (the existing fast-fall/down binding plus the jump edge) to drop through an elevated one-way platform. The request suppresses the jump edge for that tick and ignores one-way platforms only during the initiating tick. The solid main floor remains collidable.
- The next tick does not snap the falling fighter back onto the platform because one-way collision checks the previous feet position.

## Verification performed

Commands run from the repository root:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  -ffunction-sections -fdata-sections -Isrc \
  tests/ember_test_p2_no_tunnel.cpp src/World/Arena.cpp \
  -Wl,--gc-sections -o /tmp/ember-p2-test/p2_no_tunnel
/tmp/ember-p2-test/p2_no_tunnel

clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  -ffunction-sections -fdata-sections -Isrc \
  tests/ember_test_p2_no_tunnel.cpp src/World/Arena.cpp \
  -Wl,--gc-sections -o /tmp/ember-p2-test/p2_no_tunnel-clang
/tmp/ember-p2-test/p2_no_tunnel-clang

g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc \
  -c src/Gameplay/Fighter.cpp -o /tmp/ember-p2-test/Fighter.o

clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc \
  -c src/Gameplay/Fighter.cpp -o /tmp/ember-p2-test/Fighter-clang.o

python3 tools/check_rebirth_docs.py
```

The commands above passed. In addition, `tests/ember_test_p2_drop_through.cpp` exercises the actual `Fighter::update()` path (down + jump on a supported elevated platform) and passed with both GCC and Clang using the same strict flags and dead-section elimination. The focused collision suite covers high-speed landing, one-way landing from above, upward pass-through, one-tick drop-through, no re-capture on the following tick, and continued collision with the solid main floor.

## Limits

This evidence does not establish a full CMake/CTest pass, a Windows/WASM pass, or a manual gameplay feel pass. The project-wide configure/build still needs the SDL3 dependency to be available in the build environment. P2 remains open for DI, tech, ledge behavior, MatchState snapshot/restore, replay completeness, cross-platform CI, and the two-tester feel gate.
