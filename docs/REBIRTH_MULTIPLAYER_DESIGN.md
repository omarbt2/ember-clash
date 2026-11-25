> **SUPERSEDED** by the production bible in `docs/rebirth/` (see `docs/rebirth/README.md`). Kept for history; some values here were corrected in `docs/rebirth/00_PLAN_REVIEW_CORRECTIONS.md`.

# EMBER CLASH REBIRTH — Multiplayer Design (local and online)

## 1. Local (P8)
Up to 4 fighters (`kMaxFighters = 4`, D5). Join screen: each device presses Start to claim a slot; slots map to a keyboard profile (max **1** keyboard player in default config, optional 2nd profile for 1v1) or a gamepad (SDL3 gamepad IDs, hot-plug events). Per-player bindings saved in settings. Character/team/stage select per slot; modes: 1v1, FFA 3–4, 2v2, humans+AI. Today: input path works at Match level (`ember_test_rebirth_input`); menu entry absent; gamepad→fighter unverified.
Tests: input separation (no cross-slot bleed), hot-plug unplug mid-match pauses the match, deterministic 4-slot replay.

## 2. Online decisions (after P2 determinism is proven)
| Topic | Decision | Alternative |
|---|---|---|
| Netcode | **Rollback** (GGPO-style) with 2–3 frame input delay, 8 frame max rollback | Delay-based — only as fallback for bad links |
| Sim | Fixed-point 60 Hz POD snapshot (Tech D1/D2/D4) | — |
| Transport | UDP peer-to-peer with relay fallback (own relay or TURN-compatible); WebRTC data channels for the WASM build | Dedicated authoritative server (costly, not needed for 1v1) |
| NAT | STUN + hole punching; relay when it fails | — |
| Accounts/matchmaking | Supabase (auth, profiles, history, leaderboard tables with RLS) + Edge Functions for matchmaking tickets | Custom backend |
| Anti-cheat | Each peer sends input stream + periodic state hash; desync → both clients flagged; ranked result accepted only if **both** peers' signed reports + final hash agree; server rejects unmatched/duplicate reports; rate limits | Trust clients — **never** |
Supabase does not relay UDP, so a small relay service is required (budget it).

## 3. Features and order
1. Online 1v1 private room (join code) — P12. 2. Rematch, disconnect handling (pause 10 s, then forfeit; reconnect by replaying inputs when feasible). 3. Quick match. 4. Ranked 1v1 (Elo/Glicko, placement games), profiles, history, leaderboards — P13. 5. Spectator and 2v2/4P online — post-launch.

## 4. Acceptance (P12)
Simulated 100/150 ms RTT and 3% packet loss: no desyncs in 1000 recorded matches; rollback visual correction budget; sim re-run ≤ 2 ms per rollback frame ×8; match results equal on both peers; disconnect tests pass. **Cannot be verified without two networked machines**, which the owner must supply for manual tests.

## 5. Not true today
`Match::state_hash()` is a debugging fingerprint, not netcode. No sockets, no accounts, no relay exist.
