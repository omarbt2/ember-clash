# Production Roadmap

| Field | Value |
|---|---|
| Purpose | Expanded P0-P14 roadmap with realistic schedule. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 25, 26, work-packages/ |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Phase summary (P0-P14 retained, SC-10)
Effort in weeks for ONE full-time developer (about 40 h/week) including engineering, tuning, and the art labor assumptions in 10; S <= 1 wk, M 1-3, L 3-6, XL 6+ weeks.

| Phase | Name | FT weeks (low-high) | First-release weeks | Depends on |
|---|---|---|---|---|
| P0 | Recovery and Audit | 0-0 | 0-0 | - |
| P1 | Complete Game Design and Technical Plan | 1-2 | 1-2 | P0 |
| P2 | Physics and Movement | 5-8 | 5-8 | P1 |
| P3 | Combat Engine | 6-9 | 6-9 | P2 |
| P4 | Two Fully Playable Fighters | 6-10 | 6-10 | P3 |
| P5 | Real Art and Animation Pipeline | 14-26 | 14-26 | P1, P4 |
| P6 | Advanced AI and Training | 5-8 | 5-8 | P3 |
| P7 | Three Complete Arenas | 5-8 | 5-8 | P3, P5 |
| P8 | Local Multiplayer | 4-7 | 4-7 | P2, P7 |
| P9 | Modes and Full UI | 8-12 | 8-12 | P8 |
| P10 | Eight-Fighter Roster | 24-42 | 8-14 | P4, P5 |
| P11 | Six Arenas | 5-8 | post-launch | P7 |
| P12 | Online 1v1 | 14-24 | post-launch | P2, P3, P4, P9 |
| P13 | Ranked and Competitive Features | 10-18 | post-launch | P12 |
| P14 | Optimization and Release Preparation | 8-14 | 6-10 | P9, P10 |
| Sum | | 115-196 | 68-114 | |

## 2. Calendar estimate and arithmetic
- Serial sum: 115-196 weeks. Parallel work (art alongside code, AI alongside art, arenas alongside fighters) is modelled as a 15% reduction; contingency 25% (unknowns: art pipeline, determinism gate, platform issues).
- **Full scope (8 fighters, 6 arenas, online 1v1 + ranked):** 122-208 weeks = 28-48 months, one full-time developer.
- **Recommended first release (4 fighters, 3 arenas, local up to 4, no online):** serial 68-114 weeks -> 72-121 weeks = 17-28 months.
- **Part-time (about 20 h/week):** multiply calendar time by 2.3 (2.0 for hours, +15% context switching): full scope 65-110 months; first release 38-64 months.
- The preliminary "12-18 months" is not credible for the full scope even for the low end (see 3). Uncertainty range is wide (about -30%/+50%) until the art spike reports.

## 3. Workload check (why)
- 8 fighters x 47 clips = 376 clips, about 267 clip-equivalents of unique animation (10 section 1) = about 800 hours animation labor alone (20 weeks) before concept, modeling, VFX, audio.
- 8 fighters with hooks (projectiles, traps, stances, glide, counters) each need engine support and balance cycles.
- 6 arenas with 5 parallax layers each; online with relay, backend, anti-abuse; 3 platform targets; QA and playtests.
These imply well over a year of full-time effort. Art labor is the largest uncertainty and is the main reason to stage content.

## 4. Critical path and parallelism
Critical path: P1 -> P2 -> P3 -> P4 -> P5 (spike runs in parallel from P1) -> P10 wave A -> P9 -> P14. Parallelizable: art spike with P2/P3; P6 with P5; P7 art with P4; P11 with P10; backend prototype with P8-P9. Cannot start until proven: P12 (needs deterministic sim, snapshot-complete state from P3/P4); P13 (needs P12 and a validation model); fighters 3-8 (need P3 hooks and P5 pipeline).

## 5. Contingency and gates
Contingency 25% is applied above. Gates (evidence in `docs/rebirth/evidence/`): G2 determinism gate result; G3 frame-exact combat tests green; G4 two fighters playable + AI-vs-AI balance table; G5 art spike go/no-go; G8 four-player hardware session; G12 netsim 1000 matches clean.

## 6. Per-phase details
Every phase has its own work package: [work-packages/](work-packages/). Each has: objective, entry criteria, scope and non-goals, ordered tasks with subtasks, deliverables, estimate, risk, automated and manual tests, required tools and assets, completion evidence, exit gate, Git checkpoint (a tag named `rebirth-phase-Pn-done` only when the phase is actually done; this documentation branch uses no such tag).
