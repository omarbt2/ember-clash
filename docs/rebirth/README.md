# Ember Clash Rebirth - Production Bible Index

| Field | Value |
|---|---|
| Purpose | Entry point and conventions for the documentation package. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | - |
| Related | - |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

**Current execution plan:** [MASTER_EXECUTION_ROADMAP_2026-11-26_AR.md](MASTER_EXECUTION_ROADMAP_2026-11-26_AR.md) — use this as the active order of work and acceptance gates; the detailed production bible and P0–P14 work packages remain the specifications.

---

This package replaces the preliminary six documents (`docs/REBIRTH_*.md`, commit `644bd8d`), which are kept for history and marked superseded. At commit `d48fe96`, it was planning documentation only: no source code was changed in that documentation commit and no roadmap phase beyond P0 was represented as complete. Subsequent recovery changes and their verification limits are recorded in [the 2026-11-25 evidence report](evidence/2026-11-25-input-and-menu-recovery.md).

Reading order: 00_SHARED_CONSTANTS -> 00_BASELINE_AUDIT -> 00_PLAN_REVIEW_CORRECTIONS -> 01..29 -> fighters/ -> work-packages/.

Conventions: evidence tags VERIFIED / PARTIALLY VERIFIED / UNVERIFIED / MISSING / BROKEN; decisions CONFIRMED / PROPOSED / OPEN / DEFERRED; numbers labelled "initial target" are untested. Run `python3 tools/check_rebirth_docs.py` to check links, constants, dependencies and estimate arithmetic.

## Files
- [00_BASELINE_AUDIT.md](00_BASELINE_AUDIT.md)
- [00_PLAN_REVIEW_CORRECTIONS.md](00_PLAN_REVIEW_CORRECTIONS.md)
- [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md)
- [01_PRODUCT_VISION.md](01_PRODUCT_VISION.md)
- [02_GAME_DESIGN_MASTER.md](02_GAME_DESIGN_MASTER.md)
- [03_COMBAT_SYSTEM.md](03_COMBAT_SYSTEM.md)
- [04_PHYSICS_AND_MOVEMENT.md](04_PHYSICS_AND_MOVEMENT.md)
- [05_FIGHTER_ROSTER.md](05_FIGHTER_ROSTER.md)
- [07_ARENA_DESIGN.md](07_ARENA_DESIGN.md)
- [08_ART_DIRECTION.md](08_ART_DIRECTION.md)
- [09_ART_AND_ASSET_PIPELINE.md](09_ART_AND_ASSET_PIPELINE.md)
- [10_ANIMATION_AND_VFX.md](10_ANIMATION_AND_VFX.md)
- [11_AUDIO_AND_MUSIC.md](11_AUDIO_AND_MUSIC.md)
- [12_AI_DESIGN.md](12_AI_DESIGN.md)
- [13_UI_UX_DESIGN.md](13_UI_UX_DESIGN.md)
- [14_INPUT_AND_ACCESSIBILITY.md](14_INPUT_AND_ACCESSIBILITY.md)
- [15_LOCAL_MULTIPLAYER.md](15_LOCAL_MULTIPLAYER.md)
- [16_ONLINE_NETCODE.md](16_ONLINE_NETCODE.md)
- [17_BACKEND_AND_SECURITY.md](17_BACKEND_AND_SECURITY.md)
- [18_TECHNICAL_ARCHITECTURE.md](18_TECHNICAL_ARCHITECTURE.md)
- [19_DATA_AND_CONTENT_SCHEMA.md](19_DATA_AND_CONTENT_SCHEMA.md)
- [20_PERFORMANCE_AND_MEMORY_BUDGETS.md](20_PERFORMANCE_AND_MEMORY_BUDGETS.md)
- [21_TESTING_AND_QA.md](21_TESTING_AND_QA.md)
- [22_CI_AND_BUILD_MATRIX.md](22_CI_AND_BUILD_MATRIX.md)
- [23_PLATFORM_AND_RELEASE.md](23_PLATFORM_AND_RELEASE.md)
- [24_PRODUCTION_ROADMAP.md](24_PRODUCTION_ROADMAP.md)
- [25_RELEASE_SCOPE_AND_MILESTONES.md](25_RELEASE_SCOPE_AND_MILESTONES.md)
- [26_RISK_REGISTER.md](26_RISK_REGISTER.md)
- [27_DECISION_LOG.md](27_DECISION_LOG.md)
- [28_ASSET_INVENTORY_AND_LICENSES.md](28_ASSET_INVENTORY_AND_LICENSES.md)
- [29_POST_LAUNCH_AND_LIVE_OPERATIONS.md](29_POST_LAUNCH_AND_LIVE_OPERATIONS.md)
- [fighters/bastian-wardenhold.md](fighters/bastian-wardenhold.md)
- [fighters/isolde-cinderveil.md](fighters/isolde-cinderveil.md)
- [fighters/kaelen-ashforge.md](fighters/kaelen-ashforge.md)
- [fighters/maren-tidewarden.md](fighters/maren-tidewarden.md)
- [fighters/rook-emberwick.md](fighters/rook-emberwick.md)
- [fighters/torga-stonehide.md](fighters/torga-stonehide.md)
- [fighters/vesper-quill.md](fighters/vesper-quill.md)
- [fighters/zephyr-kite.md](fighters/zephyr-kite.md)
- [work-packages/P0_Recovery_and_Audit.md](work-packages/P0_Recovery_and_Audit.md)
- [work-packages/P10_Eight-Fighter_Roster.md](work-packages/P10_Eight-Fighter_Roster.md)
- [work-packages/P11_Six_Arenas.md](work-packages/P11_Six_Arenas.md)
- [work-packages/P12_Online_1v1.md](work-packages/P12_Online_1v1.md)
- [work-packages/P13_Ranked_and_Competitive_Features.md](work-packages/P13_Ranked_and_Competitive_Features.md)
- [work-packages/P14_Optimization_and_Release_Preparation.md](work-packages/P14_Optimization_and_Release_Preparation.md)
- [work-packages/P1_Complete_Game_Design_and_Technical_Plan.md](work-packages/P1_Complete_Game_Design_and_Technical_Plan.md)
- [work-packages/P2_Physics_and_Movement.md](work-packages/P2_Physics_and_Movement.md)
- [work-packages/P3_Combat_Engine.md](work-packages/P3_Combat_Engine.md)
- [work-packages/P4_Two_Fully_Playable_Fighters.md](work-packages/P4_Two_Fully_Playable_Fighters.md)
- [work-packages/P5_Real_Art_and_Animation_Pipeline.md](work-packages/P5_Real_Art_and_Animation_Pipeline.md)
- [work-packages/P6_Advanced_AI_and_Training.md](work-packages/P6_Advanced_AI_and_Training.md)
- [work-packages/P7_Three_Complete_Arenas.md](work-packages/P7_Three_Complete_Arenas.md)
- [work-packages/P8_Local_Multiplayer.md](work-packages/P8_Local_Multiplayer.md)
- [work-packages/P9_Modes_and_Full_UI.md](work-packages/P9_Modes_and_Full_UI.md)
