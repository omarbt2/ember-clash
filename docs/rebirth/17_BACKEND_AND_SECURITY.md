# Backend and Security

| Field | Value |
|---|---|
| Purpose | Services, schema, RLS, operations. Supabase for persistent app data; separate relay. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 16 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Responsibilities
| Service | Role | Not responsible for |
|---|---|---|
| Supabase Auth | accounts (email/OAuth), JWT | gameplay validation |
| Supabase Postgres + RLS | profiles, matches, ratings, leaderboards, rooms metadata | real-time packet relay |
| Edge Functions | create room, matchmaking tickets, signed relay tokens, result intake | simulating matches (too long-running) |
| Relay service (own, small VM/container) | UDP/WebRTC relay, token check, rate limits | accounts |
| Resim worker (post-first-release) | re-simulates submitted input logs with the headless sim | - |
## 2. Schema (sketch)
`profiles(id uuid pk -> auth.users, display_name, created_at)`, `rooms(id, code, host, status, version, created_at)`, `matches(id, room, p1, p2, stage, result jsonb, input_log_ref, validated bool, created_at)`, `ratings(profile, mode, rating, rd, updated_at)`, `reports(id, reporter, match, reason)`. RLS: users read own rows and public profile fields; insert only via edge functions (service role); no client write to `ratings`/`matches.validated`.
## 3. Matchmaking
Ticket table with rating band widening every 10 s; edge function pairs and issues relay tokens (short-lived, signed).
## 4. Ranking
Glicko-2; placement 10 games; leaderboards read from a materialized view refreshed every minute; only `validated` ranked matches count.
## 5. Abuse and privacy
Rate limits per IP/account at edge and relay; report + moderation queue; display-name filter; delete account endpoint; data retention: input logs 30 days, match results 12 months, relay logs 7 days; GDPR-style export/delete; no passwords handled by the game (Supabase Auth).
## 6. Recovery/ops
Email recovery via Auth; monitoring: relay health, error rate, matchmaking wait time, DB size; alerts to owner.
## 7. Stages and cost assumptions (to verify)
Prototype: Supabase free tier + one small VM (about 5-10 USD/month). Closed beta: Supabase Pro (about 25 USD/month) + relay VM (about 20-40 USD/month). Production: scales with concurrent matches; relay bandwidth per match about 20 kbps/peer.
## 8. Security checklist
Secrets only in server env; RLS on every table; relay tokens expire in 60 s; no trust in client-reported ratings (C3, 16).
