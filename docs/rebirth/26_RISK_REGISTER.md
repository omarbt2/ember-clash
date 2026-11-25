# Risk Register

| Field | Value |
|---|---|
| Purpose | Risks with probability (L/M/H), impact (L/M/H), severity. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 24 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

| ID | Risk | Prob | Impact | Severity | Early warning | Mitigation | Contingency | Owner decision | Phases |
|---|---|---|---|---|---|---|---|---|---|
| R1 | Determinism fails across platforms | M | H | High | hash mismatch in CI | numeric gate, integer sim, CI job | choose Q16.16; delay online | D-002 | P2, P12 |
| R2 | Rollback retrofit harder than planned | H | H | Critical | snapshot test failures | POD state from P2, completeness test | delay-based netcode | D-013 | P2-P12 |
| R3 | Art pipeline underperforms | H | H | Critical | spike over 3 h/clip | spike, alt method B/C | reduce roster, hire/commission | D-004 | P5 |
| R4 | Asset inconsistency across fighters | M | M | Medium | silhouette test fails | art direction, shared rig | art review gate | - | P5, P10 |
| R5 | No generator/Blender in dev env | H | M | High | tool probe | owner-run scripts, future connector | owner supplies art | D-004 | P5 |
| R6 | Physics feel poor | M | H | High | tester feedback | frame tests + early feel sessions | retune P2 | - | P2 |
| R7 | Balance issues | H | M | Medium | win-rate outside 35-65% | matchup sim, patching | per-fighter buffs/nerfs | - | P4, P10 |
| R8 | Gamepad support unproven | M | M | Medium | pad cannot drive fighter | hardware session at P8 | keyboard-first release | - | P8 |
| R9 | Windows build unverified | M | M | Medium | CI failing | CI job early | fix portability | - | P2 |
| R10 | WASM build/perf issues | M | M | Medium | frame time > budget | CI wasm job | web demo only | - | P14 |
| R11 | Online result integrity | H | H | Critical | win trading reports | defer ranked; model C+D | unranked only | D-014 | P13 |
| R12 | Backend cost/ops burden | M | M | Medium | cost > budget | small relay, free tier | shut online | D-012 | P12 |
| R13 | Performance regressions | M | M | Medium | perf CI warns | budgets 20 | optimize/cut VFX | - | all |
| R14 | Schedule overrun | H | H | Critical | phase slips > 30% | stage scope, gates | cut roster to 2-3 | D-016 | all |
| R15 | Scope creep | H | M | High | new modes added early | MoSCoW, decision log | freeze | - | all |
| R16 | Solo-developer burnout/bus factor | M | H | High | slipping cadence | milestones, docs | pause/shrink | - | all |
| R17 | Licensing of generated/third-party assets | M | H | High | unlogged assets | 28 inventory | replace asset | - | P5 |
