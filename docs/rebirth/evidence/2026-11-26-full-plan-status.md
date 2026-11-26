# Full Production Plan Execution Ledger — 2026-11-26

| Field | Value |
|---|---|
| Purpose | Evidence-based status of every phase in the supplied Arabic Master Completion Plan |
| Status | IN PROGRESS — no production phase is declared complete by this ledger |
| Source of truth | `docs/rebirth/MASTER_COMPLETION_PLAN_AR.md` and the user's supplied `EMBER_CLASH_REBIRTH_MASTER_PLAN_AR.md` |
| Revision date | 2026-11-26 |
| Source revision inspected | Recovered bundle HEAD `42fe5ad`; implementation snapshot `1e3175a`; see `evidence/build-recheck-2026-11-26.md` |
| Build environment | Linux container; GCC 14.2.0; CMake 3.31.6; Ninja 1.12.1 |

## Verification rules

- `VERIFIED` means the stated test/build/manual gate was actually executed and passed in this run, with a reproducible command or evidence.
- `PARTIALLY VERIFIED` means a subset passed but the full phase exit criteria remain open.
- `BLOCKED` means the required verification could not run because of a concrete environment or external dependency.
- `MISSING` means the planned system/content is not implemented or not present as a real production deliverable.
- A source-code presence check, procedural placeholder, isolated test, or old build report does not satisfy a full phase gate.
- No `rebirth-phase-Pn-done` tags are created until each phase's own exit gate passes.

## Phase ledger

| Phase | Plan exit gate (summary) | Current evidence | Status | Remaining gate |
|---|---|---|---|---|
| P0 Recovery/Audit | One reconciled source, clean history, clean Release build, all baseline tests, platform/hardware limits recorded | Newest ZIP/bundle verified; 181/181 files match; bundle fsck passes; local `1e3175a` descends from remote Phase 0 and master history; docs checker 61 files, asset tests 7/7 and four strict isolated C++ tests pass; SDL3 `release-3.2.10` fetch still fails DNS | PARTIALLY VERIFIED / BUILD BLOCKED | Obtain network or a provenance/hash-verified SDL3 cache; clean Release build + full CTest; reconcile the non-identical remote recovery ref; verify Windows/WASM/hardware as applicable |
| P1 Design/Technical Plan | Owner-approved production bible, decisions resolved/deferred with reasons, shared constants and no contradictions | Master plan and 29 design/technical docs present; user explicitly authorized execution; decision log still contains unresolved/proposed items | PARTIALLY VERIFIED | Close or explicitly defer remaining D- decisions; update constants and decision log; verify consistency with implementation |
| P2 Physics/Movement | Deterministic integer-tick model or approved representation; full movement set; replay gate and 20 replay comparison | Jump buffer, coyote time, double jump, run/dash and drop-through foundations exist; motion tests pass isolated | PARTIALLY VERIFIED | Complete ledge/DI/tech/blast-zone rules; convert remaining timers as decided; prove replay determinism and no-tunnel fuzz |
| P3 Combat Engine | Data-driven moves; complete attack states, grabs/throws, projectiles/traps, armor/priority/staleness/DI; frame-exact tests and snapshot completeness | Attack table validation and combat-resolution foundation exist; isolated attack-table test passes | PARTIALLY VERIFIED | Missing major move families and lifecycle/state snapshot coverage; full scenario harness and tick-exact suite |
| P4 Two Fully Playable Fighters | Kaelen and Torga each have complete, distinct move sets/hooks, matchup balance and playtest evidence | Two fighter slots and shared combat foundation exist; fighter design documents present | PARTIALLY VERIFIED | Unique hooks and all moves playable, matchup simulator, human playtests and balance evidence |
| P5 Real Art/Animation Pipeline | Production character art and all 47 clips/fighter; rig/atlas/animation data; ≤2 px hitbox alignment; licensing; real assets in build | Art manifest validator and seven validator tests pass; current runtime still uses procedural placeholder sprites; no complete authored production atlas/rig assets in repo | MISSING production art / PARTIAL tooling | Approve art method; deliver Kaelen/Torga 47 clips each first; import into runtime; review hand/arm/action poses and licenses; no placeholder release assets |
| P6 Advanced AI/Training | Four difficulty tiers; fair perception/reaction; personalities/targeting; full training UI/dummy; ≥95% recovery benchmark; 1000-match fuzz | Easy/Standard/Hard/Expert presets and bounded recorder/playback foundations exist; isolated AI preset and recorder tests pass | PARTIALLY VERIFIED | Add personality/FFA/team logic and training controls; benchmark difficulty and recovery; run long fuzz/no soft-lock |
| P7 Three Complete Arenas | Three finished visual/gameplay arenas, blast zones, camera, collision/spawn tests, hazards toggle and ambience | Three arena layouts and boundary/collision tests exist; isolated arena test passes; rendering remains procedural | PARTIALLY VERIFIED | Production art/parallax layers, camera coverage with all fighters, hazards rules, ambience and visual/manual stage QA |
| P8 Gamepad/Local Multiplayer 4P | Multi-device registry, controller setup, independent 4 slots, join/character/team/side/stage flow; hardware and virtual-pad tests | SDL event-pump ownership/hotplug snapshot fix and primary gamepad adapter exist; input manager key mapping probe passes | PARTIALLY VERIFIED | Gamepad class still opens one primary device; Match remains 2 fighters; no four independent controller slots or complete setup/join flow; SDL virtual/hardware tests |
| P9 Full UI/Modes | Complete menus/HUD/pause/results/settings/tutorial/profile; every action functional; keyboard/gamepad navigation and usability tests | Existing menu/settings and partial mode/stage-selection foundations exist | PARTIALLY VERIFIED | Implement full player join/character/team/stage/match settings, results/rematch, training HUD/tutorial/profile; test every flow and no dead ends |
| P10 Eight-Fighter Roster | Eight complete fighters; V1 wave of four complete; unique hooks, 47 clips each, balance matrix | Roster design documents name eight fighters; code/content currently does not contain eight fully playable production fighters | MISSING / PARTIAL design | Complete V1 four fighters with all moves, unique gameplay, animation/audio/VFX and matchup tests; then post-launch four |
| P11 Arenas 4–6 | Six finished arenas and hazard/collision/performance QA | Three procedural arena layouts are present; no complete production set of six authored arenas | MISSING | Build three additional stages, assets, data, hazards and tests |
| P12 Online 1v1 | Rollback/input history/transport/rooms/reconnect; 1000 net simulations and two-device test | Only local state hash/fingerprint foundations; no production online stack | MISSING | Snapshot completeness and determinism gates first; then netcode, transport, service/rooms and real-device latency QA |
| P13 Ranked/Competitive | Server resimulation, validated results, rating/seasons/leaderboards and abuse checks | No ranked service or verified competitive result pipeline | MISSING | Implement only after P12 stable; security, simulation verification, closed beta and abuse testing |
| P14 Optimization/Release | CI release matrix, packaging, reference-hardware budgets, 1000-match fuzz, external playtest, no placeholders/unlicensed assets | Recovered local workflow defines docs/Linux/Windows/Emscripten 4.0.23 jobs and YAML parses; remote `.github/workflows/ci.yml` fetch returned 404 on all three current refs; no Actions run or full Release build verified | PARTIALLY VERIFIED / BUILD BLOCKED | Publish the reconciled workflow/source branch, verify Actions runs, complete reproducible platform builds, benchmark/profiling, packaging, crash/save migration, external playtests, credits/licenses/store assets |

## Tests executed in the 2026-11-26 recheck

| Command/test | Result | Limitation |
|---|---|---|
| `python3 tools/check_rebirth_docs.py` | PASS — 61 Markdown files | Documentation consistency only |
| `python3 -m unittest discover -s tools/tests -v` | PASS — 7 tests | Asset-manifest validator only |
| `ember_test_attack_table` (strict GCC C++20) | PASS — compile and execute | Attack-table logic only |
| `ember_test_ai_presets` (strict GCC C++20) | PASS — compile and execute | AI preset data only |
| `ember_test_training_recorder` (strict GCC C++20) | PASS — compile and execute | Bounded recorder utility only |
| `ember_test_rebirth_motion` (strict GCC C++20) | PASS — compile and execute | Platform-neutral movement step logic only |
| Local CI workflow YAML parse | PASS — docs/Linux/Windows/WebAssembly jobs present | Syntax only; workflow not verified on GitHub |
| Bundle integrity / ZIP tree comparison | PASS — complete bundle, fsck clean, 181 files match | Recovery integrity only |
| CMake Release configure | BLOCKED — SDL3 fetch DNS failure | Stops before project compilation |
| Full CMake build / CTest | NOT RUN | Configure did not complete |
| Windows/WASM runtime, display/audio/gamepad/4P | UNVERIFIED | Requires successful target build and runtime/device tests |

See `evidence/build-recheck-2026-11-26.md` for exact SHAs, command, logs, artifact hashes and remote ref comparison.

## Current checkpoint and next order

- Recovered bundle/document HEAD: `42fe5ad9b667f0aca1d2f61a13d7382be0e3fd08`; underlying implementation snapshot: `1e3175aba6e6d6fcf6237016ad3fe35119165efe`.
- Current local work branch: `work/s0-build-unblock-2026-11-26`, based on the verified recovered bundle. No remote branch was changed.
- Current GitHub refs: `master=365348a2dacea4e03362bfd1fd718582361ba693`, `rebirth/phase0=7fddfde7b909b2fa338bc9095b29f30bb0031bf9`, and `reconstruction/rebirth-input-recovery=3026c02757a5bffc6e894fdb1309eb7219d6f1c7`. The local `1e3175a` snapshot descends from the first two through its merge history; the remote reconstruction ref with the same name is a separate, non-identical line.
- Direct Git network access still fails DNS. No push, PR or merge was attempted. The recovered `.github/workflows/ci.yml` parses locally, but GitHub returned 404 for that path on all three current remote refs; no workflow run for the local SHA is established.
- Immediate engineering order: obtain reachable GitHub networking or a provenance/hash-verified SDL3 `release-3.2.10` cache → clean Release configure/build → full CTest → complete P0 tree/ancestry reconciliation and safe new-branch publication.

This ledger is a progress/evidence artifact, not a declaration that the plan or any phase is complete.
