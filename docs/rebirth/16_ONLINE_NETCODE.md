# Online Netcode and Threat Model

| Field | Value |
|---|---|
| Purpose | Evidence-based online architecture including security model. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 17, 18, 25 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline: no networking exists (MISSING). `Match::state_hash()` is a debug fingerprint only.
## 1. Architecture
Rollback with input delay [SC-15]: each peer simulates locally, sends inputs, rolls back on late/mispredicted remote input. Requirements: tick-exact deterministic sim (04, 18), snapshot save/restore per tick (<= 8 frames window), input history ring buffer, desync detection via per-N-tick hash of the snapshot bytes.
**Rollback state:** everything in `MatchState` (18 section 6). **Excluded:** rendering state, particles, camera, audio, UI, interpolation state, debug.
## 2. Parameters (PROPOSED; gate: measured)
Input delay default 2 ticks (adaptive 0-4), rollback max 8; resim cost <= 8 ticks per frame within 6 ms on reference HW [SC-13]; if exceeded fall back to delay-based for that match. Jitter buffer 2 ticks. Packet loss tolerance: 5% with input redundancy (last 6 inputs per packet).
## 3. Transport
| Platform | Option | Notes |
|---|---|---|
| Native Win/Linux | UDP via SDL_net or own sockets; STUN + hole punch; relay fallback | UDP plus own reliability layer for control messages |
| WebAssembly | WebRTC data channels (unreliable/unordered) via signalling server; or WebSocket relay (TCP head-of-line blocking hurts rollback) | browsers cannot open raw UDP; WebRTC recommended, WebSocket fallback with larger delay |
Native and web peers do not interoperate in the first online release (separate pools) unless WebRTC is also used natively - OPEN (D-013).
## 4. Room lifecycle
Create (code) -> join -> ready -> sync (RNG seed, version, content hash) -> countdown -> play -> result -> rematch or leave. Version/content-hash mismatch blocks the match.
## 5. Disconnects
Timeout 5 s without packets -> pause 10 s -> forfeit. Reconnect: peer replays the input log from last snapshot checkpoint (feasible because inputs are small) - post-first-online-release feature (DEFERRED).
## 6. Threat model
| Actor | Can do | Cannot / mitigation |
|---|---|---|
| Malicious client | send impossible inputs (frame-perfect scripts/macros), modify local sim to fake own state, send desync-causing inputs, lag-switch, DoS peer, send false result | A deterministic lockstep/rollback peer cannot hide state, but an edited client can simulate a different game and claim a result; macros are undetectable by hashes |
| Two colluding clients | agree on any result and hash (win trading) | Mutual signed reports and equal hashes DO NOT prove the match happened. Rejected as sufficient |
| Relay | forward/drop/delay packets, see inputs, not verify gameplay | can rate-limit, cap sizes, authenticate sessions, log |
| Supabase | authenticate, store, enforce RLS, run edge functions | cannot validate 60 Hz gameplay; edge functions are short-lived; no UDP |
## 7. Result validation models
| Model | Cost | Security |
|---|---|---|
| A. Trust (casual) | none | none - acceptable only for unranked, unlisted |
| B. Dual-report agreement | low | stops accidental mismatch only; no cheat/collusion protection |
| C. Input-log submission + server re-simulation (headless C++ sim worker; SDL-free) | medium (CPU: a 3-min match = 10,800 ticks, well under 1 s) | validates that the result follows from the inputs and sim rules; does not stop macros/aimbots or collusion |
| D. C + anomaly detection (input timing statistics, win-trade graph analysis, rate limits, reports) | medium-high | best practical; no model is cheat-proof |
| E. Authoritative server sim | high | strongest, input latency |
Cheapest viable architecture for first online 1v1: A+B for unranked rooms with explicit "unverified" label; ranked requires C+D and is excluded from the first release (SC-16, D-014). This design claims no stronger security than listed.
## 8. Deferred
Spectators, online 2v2/FFA, reconnect, cross-platform native/web play, ranked.
## 9. Tests
Netsim harness: configurable latency/jitter/loss; 1000 recorded matches with no desync; snapshot-restore equivalence; fuzz inputs; relay auth tests; manual: two real machines (owner supplies).
