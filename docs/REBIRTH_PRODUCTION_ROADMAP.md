# EMBER CLASH REBIRTH — Production Roadmap

Complexity: S ≤ 1 wk, M 1–3, L 3–6, XL 6+ weeks for one full-time developer (estimates, ±50%). Every phase ends with a Git checkpoint tag `rebirth-Pn` on a branch, no force-push.

| Phase | Scope | Depends | Size | Acceptance criteria | Automated tests | Manual verification |
|---|---|---|---|---|---|---|
| **P0 Recovery/Audit** | Verify bundle, build, tests, audit | — | S | **Done in this report**: bundle has `2eeceae`; 0 warnings; 14/14 pass | existing 14 | — |
| **P1 Plan** | These 6 docs, owner approval | P0 | S | Owner approves/edits | doc consistency check (shared constants) | read-through |
| **P2 Physics/Movement** | D1 time model, D2 fixed-point, centralized config, jump buffer, coyote, run, drop-through, ledges, DI, deterministic replay | P1 | L | Replay hash equal on Linux, Windows, WASM; input→move ≤ 3 f; all existing tests green | determinism, collision, movement | feel session on keyboard+pad |
| **P3 Combat Engine** | Data-driven moves, behavior hooks, projectiles, grabs, armor, priority, combo windows | P2 | L | Frame-exact tests for every move field; projectile ownership/lifetime | per-move scenario | hitbox overlay review |
| **P4 Two Fighters** | Kaelen + Torga with unique specials, full move sets, balance pass | P3 | L | All moves reachable; distinct playstyles; AI-vs-AI 40–60% | matchup sim | playtest |
| **P5 Art/Animation** | Pipeline spike (A vs B), 2 fighters' art, VFX, atlas loader | P1 (spike), P4 | XL | Real sprites in game; animation timing matches frame data | atlas/frame-count validator | visual review (needs display) |
| **P6 Advanced AI** | Perception, 4 difficulties, personalities, Training mode | P3 | M | AI doc §4 criteria | AI suite | play vs each level |
| **P7 Three Arenas** | Data-driven arenas, camera, blast zones, music hooks | P3, P5 | M | 3 arenas, fair geometry | collision tests | visual |
| **P8 Local Multiplayer** | 4 slots, joins, hot-plug, menu entry | P2, P7 | M | Multiplayer §1 tests | input separation | 4 people with pads |
| **P9 Modes/Full UI** | Char/stage/mode select, FFA, Team, Arcade, Survival, results, pause, settings | P8 | L | No menu entry for unbuilt mode | flow tests | UX pass |
| **P10 Eight Fighters** | Waves of 2 (Vesper, Isolde → Rook, Maren → Zephyr, Bastian) | P4, P5 | XL | Each fighter meets P4 criteria; roster balance window | matchup matrix | playtest |
| **P11 Six Arenas** | Arenas 4–6 + casual hazards | P7 | M | Ranked set hazard-free | collision | visual |
| **P12 Online 1v1** | Rollback, relay, rooms, rematch, disconnect | P2 (determinism), P9 | XL | Multiplayer §4 | netsim/desync tests | 2 real machines |
| **P13 Ranked** | Matchmaking, ratings, history, leaderboards, server validation | P12 | L | Result integrity checks; abuse tests | server tests | closed beta |
| **P14 Optimize/Release** | Perf, packaging (Win/Linux/Web), crash reporting, demo, closed playtests, store pages | all | L | 60 fps on reference HW; no P0 bugs | perf suite | external playtest |

## Critical path
P2 → P3 → P4 → P5 → P10; online (P12) branches after P2 but needs P9 UI. Total: roughly 12–18 months for one developer; faster with an artist (art is the largest uncertainty).

## Top risks
1. Rollback retrofit (P2/P12). 2. Art throughput and tooling access. 3. Feel tuning needs humans. 4. Online ops. 5. Scope creep (mitigation: fighters/arenas in waves, no unbuilt menu entries).

## Owner decisions needed (recommended default in bold)
1. Art method spike: **A (Blender 3D→sprites)**. 2. Fixed-point sim: **yes**. 3. Online backend: **Supabase + small relay**. 4. Include grabs: **yes**. 5. Provide tooling access (image generator / Blender / Windows machine / 2 pads) — or supply art yourself.
