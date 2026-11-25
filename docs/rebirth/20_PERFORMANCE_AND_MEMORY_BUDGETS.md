# Performance and Memory Budgets

| Field | Value |
|---|---|
| Purpose | Measurable targets; nothing here is measured yet. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 18, 21 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Reference hardware [SC-13]. Measured status: UNMEASURED - no benchmark has been run in this task; first measurement is a P2 deliverable.
| Budget | Target |
|---|---|
| Frame | 16.67 ms at 60 fps [SC-14]; 1% low >= 55 fps |
| Sim tick (4 fighters, 16 projectiles) | <= 1.5 ms; resim x8 <= 6 ms |
| Render | <= 8 ms at 1080p; draw calls <= 400 |
| Active hitboxes | <= 32 per tick |
| Projectiles / traps | <= 64 / 16 per match; <= 16 projectiles per fighter |
| VFX particles | <= 200 live |
| Texture memory | <= 160 MB (4 fighters x 16 MB + arena 24 MB + UI/VFX 40 MB) at launch roster; 8 fighters <= 256 MB |
| Audio memory | <= 64 MB decoded SFX; music streamed |
| Atlas | <= 4096x4096 each |
| Load | match start <= 3 s; cold startup <= 5 s |
| WASM | heap <= 512 MB; download <= 60 MB compressed |
| Startup/frame pacing | no hitches > 20 ms in a 3-min match |
Tools: built-in frame timer HUD (F3), perf test suite with fixed replays, CI perf smoke (warn on +20% regressions).
