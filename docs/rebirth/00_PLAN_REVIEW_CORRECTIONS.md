# Review of the Preliminary Plan: Corrections Log

| Field | Value |
|---|---|
| Purpose | For every significant correction to the six preliminary documents: original, problem, recommendation, alternatives, trade-offs, verification, approval need. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 27_DECISION_LOG.md |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Legend: Approval = does the owner need to approve (Y/N). IDs map to 27_DECISION_LOG.md.

| # | Original proposal | Problem found | Recommended | Alternatives / trade-offs | Verification | Approval |
|---|---|---|---|---|---|---|
| C1 | "`step_motion` ignores `dt` - inconsistent; remove dt" (Tech doc) | Overstated. Reading `App::run`, `Fighter::update` (passes constant `1.0/60.0`) shows runtime dt is always 1/60; the unused parameter is a stale-comment hazard, not a live bug. The real hazard is `double`-seconds timers (respawn 0.5 s, KO banner, invuln) and variable dt in `Match::update`/AI | Convert all gameplay timers to integer ticks; sim API takes no dt; keep dt only in render interpolation. Prove with a before/after replay equivalence test (04 section 2) | Keep dt, assert dt == 1/60 (cheap, leaves float timers). Rejected for rollback needs | Replay test: scripted inputs, old vs new produce same positions within 1 px and same events; P2 | Y (D-001) |
| C2 | AI reaction delay EASY 20-30 f, EXPERT 6-8 f ("human floor 6") | Code already has kEasy=70, kHard=18 ticks. Human visual reaction is about 12-15 frames, not 6, so 6-8 f is superhuman and unfair | Calibrated bands in 12 section 4: EASY 45-70, NORMAL 28-36, HARD 18-24, EXPERT 12-16, never below 12 | Per-action reaction times; adaptive difficulty | Measure the opening-to-response latency distribution in AI tests; human playtest calibration | Y (D-009) |
| C3 | Ranked result = both peers' signed reports + hash agree | Insufficient: two colluding or one-sided cheating clients can submit agreed fake results; a relay cannot verify gameplay; Supabase cannot validate a 60 Hz sim | Ranked deferred past first release (SC-16). Unranked online first. Ranked requires server-side deterministic re-simulation of submitted input logs (the sim is SDL-free C++) plus anomaly detection - 16, 17 | Dedicated authoritative server (costly); trust-based casual leaderboards | Threat model review; server resim test on recorded matches | Y (D-014) |
| C4 | "input -> move <= 3 frames" | Ambiguous; mixes sim, vsync, OS latency, netcode | Offline requirement: input sampled at tick N produces its first state change in tick N+1 (1 tick); budget 0 extra ticks for buffering of attacks except explicit buffer windows. Display latency budgeted separately in 20; online delay in 16 | - | Frame-exact scenario tests; high-speed camera for display chain (manual) | N |
| C5 | Timeline 12-18 months, 8 fighters, 6 arenas, online ranked | Not credible, see 24 section 3: full scope 28-48 months FT solo | First release = 4 fighters, 3 arenas, local up to 4P, unranked online 1v1 only if gates pass; remaining fighters/arenas/ranked post-launch (25) | Keep full scope and extend timeline; hire artist | Schedule arithmetic checked by `tools/check_rebirth_docs.py` | Y (D-016) |
| C6 | Arena "blast zones" assumed | Code has only a bottom kill plane; sides/top missing; full-width floor means lateral KOs need falling past floor edge | Define 4 blast zones per arena as data (07, 19, P2/P7) | Keep bottom-only | Collision/KO scenario tests | N |
| C7 | Roster waves 2->4->6->8 as launch | Largest cost driver | 4 fighters must-have, 8 should/post-launch | 2-fighter launch ("Early Access") | Art spike measurements before committing | Y (D-016) |
| C8 | Online (P12) depends on P2, P9 | Snapshot must include projectiles/traps/stance introduced in P3-P4; late discovery is the biggest rollback risk | P12 depends on P2, P3 (snapshot-complete sim), P4; every new state field must be added to snapshot + completeness test | - | Snapshot completeness test (18 section 6) | N |
| C9 | Art spike in P5 after P4 | Art has the longest lead time | Run the art spike (09) in parallel with P2/P3; decision gate before P4 | - | Spike deliverables checklist | Y (D-004) |
| C10 | Fixed-point Q16.16 "yes" | Presented as settled; not proven necessary | PROPOSED with a test gate (18 section 4): constrained float vs Q16.16 across Linux/Windows/WASM | Q24.8, int64 Q32.32 | Cross-platform determinism test gate in P2 | Y (D-002) |
| C11 | Single-dev timeline only | No part-time guidance | FT and PT (20 h/wk) estimates in 24 | - | - | N |
| C12 | `state_hash` described as "int32-cast" | Code calls `mix_f32/mix_f64` in places; comment/code mismatch | Replace by hash over the POD snapshot bytes of integer/fixed fields (18) | - | m11 test extended | N |
| C13 | Supabase as relay/validator | Supabase has no UDP relay; edge functions cannot run the sim at 60 Hz per match cheaply | Supabase for accounts, profiles, matchmaking tickets, history; separate relay service; resim worker for ranked (17) | Firebase/PlayFab/Nakama | Prototype load test | Y (D-012) |
| C14 | Grab/throw "yes" | Reasonable but adds states | Keep in P3 as DECIDED-PROPOSED | Shield-break-only counter | Playtest | Y (D-011) |
| C15 | Gamepad "support" implicit | No hardware evidence | Treat as UNVERIFIED until a pad session; P8 gate | - | Hardware test checklist | N |
