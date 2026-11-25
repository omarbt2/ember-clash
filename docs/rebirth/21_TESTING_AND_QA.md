# Testing and QA

| Field | Value |
|---|---|
| Purpose | Quality strategy and evidence rules. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 22, 24 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline: 14 headless CTest suites, 14/14 pass (VERIFIED). They do not prove visuals, audio, hardware, or networking.
| System | Acceptance | Automated | Manual | Evidence | Known limit |
|---|---|---|---|---|---|
| Movement/physics | 04 section 5 | unit + scenario + replay | feel session | test log + notes | feel is subjective |
| Collision | no tunnelling | fuzz | - | log | - |
| Combat timing | frame-exact | per-move scenarios | overlay review | log + screenshots (when display exists) | |
| Determinism | equal hashes across platforms | replay in CI | - | CI artifacts | needs Windows/WASM CI |
| AI | 12 section 10 | match sims | play each level | win-rate table | |
| Local MP | 15 section 6 | virtual-device tests | 4-player session | checklist | needs hardware |
| Network | 16 section 9 | netsim | 2 machines | logs | needs real machines |
| Assets | validator passes | validate_assets.py | visual review | report | |
| UI flow | no dead ends | scripted menu walks | UX pass | recordings | |
| Performance | 20 | perf suite | HW run | numbers | reference HW needed |
| Crash/regression | no crash in 1000-match fuzz | fuzz job | - | | |
Rules: bugs get regression tests first; a milestone gate requires listed evidence files in `docs/rebirth/evidence/` (screenshots, logs, checklists with date/device/tester). Never claim visual, audible, or hardware verification from a headless run.
Tools to add: frame-exact scenario DSL, replay recorder, determinism runner, matchup simulator, menu flow driver.
