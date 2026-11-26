# D-002 constrained-float replay probe — 2026-11-26

| Field | Value |
|---|---|
| Purpose | Capture the current local D-002 replay evidence without prematurely deciding the numeric representation. |
| Revision date | 2026-11-26 |
| Decision | D-002: constrained float vs fixed-point |
| Status | PARTIAL LOCAL EVIDENCE — decision remains OPEN |
| Replay | 100,000 simulation ticks, deterministic randomized input, Standard AI seed `0xD0022026` |
| Probe | `tests/ember_test_d002_float_replay.cpp` |
| Compiler matrix | GCC 14.2.0 and Clang 17; `-O0` and `-O2` |
| Floating-point flag | `-ffp-contract=off`; no fast-math enabled |

## Result

All four local compiler/optimization configurations emitted the same digest:

```text
D002_FLOAT_REPLAY_HASH=3c7689ffdc9e9abb TICKS=100000
```

The simulation-critical sources (Match, Fighter, AI, movement, arena, attack/input mapping and animation) were rebuilt for each compiler/optimization combination. The run used the local headless SDL declaration shims and prebuilt presentation/platform objects; it was not a full SDL-linked application build.

## Limits and next gate

This is evidence for repeatability on this Linux runner only. It does **not** cover Windows/MSVC, Emscripten/WASM/Node, three independent builds per platform, or a clean full Release + CTest build. It also does not compare Q16.16/Q24.8 implementations or establish cross-platform hashes. No numeric representation is approved from this result alone.

D-002 remains **OPEN**. Continue with clean native/WASM CI and repeat the probe on the specified platforms/builds; if hashes diverge, capture the first divergent tick and decide whether to constrain float further or prototype fixed-point. Keep the current simulation float representation until the decision is evidence-backed; do not silently convert the whole physics stack based on this partial probe.
