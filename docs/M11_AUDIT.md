# M11-AUDIT: Online / Competitive Hardening — Audit Report

**Status:** COMPLETE for the in-scope work. **NOT** "online done" — see
the bottom of this document for the explicit non-claims.

**Milestone:** M11-AUDIT (determinism + desync + disconnect audit)
**Commit:** (see git log — M11 series)
**Date:** 2026-09-02

---

## 1. What was audited

The M11 scope per `AGENTS.md` is *"determinism + desync + disconnect
audit of the multiplayer / online paths."* The audit looked at three
buckets:

1. **Code presence** — does any networking code exist in the repo?
2. **Dependency presence** — does the build system pull in any
   networking library?
3. **Documentation presence** — do any first-party docs claim online
   features that don't exist?

Plus one M11-delta addition:

4. **Deterministic-simulation foundation** — even though no online
   code exists, is the underlying simulation deterministic enough
   that an online implementation would be viable once M9 + M10 land?

---

## 2. Audit results

### 2.1 Code presence — **NO networking code exists**

Searched `src/` recursively for the following terms (whole-word,
case-insensitive):

```
online, network, socket, ENet, SDL_net, WebSocket, websocket,
HTTP, lobby, matchmaking, leaderboard, player_profile,
account_name, account_id, TCP, UDP
```

**Hits in `src/`:** 0.

The only false-positive was `Arena.cpp:83` matching the regex
"...penetration..." via the substring "net" inside "penetration",
which is a physics comment, not a network reference.

### 2.2 Dependency presence — **NO networking dependencies**

`CMakeLists.txt` only fetches **SDL3** via `FetchContent_Declare`.
No `find_package(ENet)`, `find_package(SDL_net)`, `find_package(asio)`,
`find_package(curl)`, or any other transport library. The only
third-party code in the project is SDL3 itself, which is used for
windowing / input / rendering / audio. SDL3's optional `SDL_net` is
**not** pulled in.

### 2.3 Documentation presence — **first-party docs are honest**

The only "online" hits in the first-party docs (`README.md`,
`AGENTS.md`, `EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md`) are
**explicit statements that online is missing**:

- `README.md:268-269`:
  > **M10 - Online multiplayer (MISSING):** zero networking code in
  > the repo. No socket / udp / tcp / webrtc / lobby / matchmaking.
- `README.md:273-277`:
  > **M11 - Online / competitive hardening (BLOCKED):** the M11 work
  > is a determinism + desync + disconnect audit of the multiplayer /
  > online paths. Until M9 + M10 actually exist, M11 can only audit
  > deterministic simulation and cannot harden any real online path.
  > We will explicitly NOT claim M11 done on online features that do
  > [not exist].

All other "online" hits in `.md` files are inside `build/_deps/sdl3-src/`,
which is third-party SDL3 documentation (e.g. its `README-ios.md`
mentions "Game Center" and `matchmaking` as an iOS example, not a
claim about Ember Clash).

### 2.4 Deterministic-simulation foundation — **PASS**

Added `Match::state_hash()` (FNV-1a 64-bit over every field that
affects simulation output: arena, both fighters' body/velocity/state/
damage/stocks/facing/jumps/attack, match phase, respawn timers,
hit-spark, hit-stop, KO shake). The M11 test harness
(`tests/ember_test_m11.cpp`) verifies:

| Scenario                                                       | Result |
|----------------------------------------------------------------|--------|
| `state_hash()` returns non-zero on a fresh match               | PASS   |
| `state_hash()` changes after 30 ticks of bot-only activity    | PASS   |
| `state_hash()` is deterministic (same state -> same hash)      | PASS   |
| Same inputs + same AI seed -> identical hash sequence (601/601) | PASS   |
| Same inputs + same AI seed -> identical hash sequence (600/600) | PASS   |
| Different AI seeds -> different hash sequences                  | PASS   |

**What this means:** the simulation foundation is deterministic
enough that, if M9 (local multiplayer) and M10 (online) ever ship,
the `state_hash()` function can be sent over the wire as a desync
detection primitive. Two peers running the same inputs would produce
the same hash on every tick; a hash mismatch would mean the two
peers have diverged and the desync handler can kick in (rollback,
resync, or disconnect).

**What this does NOT mean:** it does not mean online multiplayer
works. There is still no socket, no peer discovery, no lobby, no
input encoding, no latency compensation, no rollback netcode, no
relay server, no matchmaking, no leaderboard, and no networking
library dependency. `state_hash()` is a 64-bit number with nowhere
to go.

---

## 3. What is BLOCKED

Everything in M11 that touches actual network I/O is **BLOCKED**
until M9 + M10 land:

- [ ] Rollback netcode
- [ ] Input delay / buffering
- [ ] Desync recovery (despite the hash primitive, there's no peer
      to exchange it with)
- [ ] Disconnect handling
- [ ] Reconnection
- [ ] Lobby / matchmaking / quickplay
- [ ] Leaderboard / ranking
- [ ] Cloud saves / replays / spectate
- [ ] Anti-cheat

None of these have any code in the repo. The M11 milestone does
**not** claim them as done.

---

## 4. What M11-AUDIT actually delivered

- **`Match::state_hash()`** — 64-bit FNV-1a fingerprint of the
  full simulation state. Public API. Documented in
  `src/Gameplay/Match.h`.
- **`Fighter::jumps_remaining()`** — public accessor for the
  fighter's remaining jumps, added so the hash could include it
  without making `Match` a friend of `Fighter`.
- **`tests/ember_test_m11.cpp`** — 4 scenarios / 9 assertions
  proving the determinism contract.
- **`tests/run_m11_test.{bat,cmd}`** — headless runner following
  the M4/M5/M6/M7/M8 pattern.
- **`tests/verify_all_desktop_tests.bat`** — M11 added to the
  regression chain.
- **This document** — the truthful audit.

---

## 5. Verification

- Build: clean (0 errors, 0 warnings).
- Full regression: 8 harnesses, **159 checks GREEN** (AI 14 + M5 14
  + M6 12 + M6-B 33 + M6-C 59 + M7-AI 14 + M8 4 + M11-AUDIT 9).
- Game smoke: `build/bin/EmberClash.exe` launches, runs, accepts
  clean termination.

---

## 6. Non-claims (explicit)

The M11-AUDIT work does **NOT** claim:

- That online multiplayer works.
- That any networking code exists.
- That any networking dependency is linked.
- That rollback netcode is implemented.
- That desync recovery is implemented.
- That disconnect handling is implemented.
- That the game can connect to a server.
- That the game can connect to another instance.
- That the game has any online feature of any kind.

It **only** claims that the **simulation foundation** is
deterministic, and that the `state_hash()` primitive is a sound
starting point for a future online implementation to build on.
