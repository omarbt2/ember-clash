# Work Package P6: Advanced AI and Training

| Field | Value |
|---|---|
| Purpose | Implementation plan for P6. No implementation is done by this document. |
| Status | IN PROGRESS - PARTIALLY VERIFIED (bounded input record/replay and four presets; advanced planner/UI calibration remains) |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P3 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Advanced AI and Training mode.

## Entry criteria
Phases complete: P3; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
12_AI_DESIGN: perception, 4 levels, personalities, training tools.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Perception layer
2. Utility planner
3. EXPERT/personalities
4. Training UI/overlay
5. Matchup/difficulty tests
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/AI, src/UI (training), tests/

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
5-8 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: AI suites, difficulty ordering, no-hidden-info. Manual: human play at each level; calibration. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 12 s4 criteria
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Calibration table committed. Git checkpoint: tag `rebirth-phase-P6-done` only after the exit gate passes.

## Rollback / recovery
Keep kStandard AI as fallback; never force-push; keep previous tags.

## Next phase dependencies
P9
