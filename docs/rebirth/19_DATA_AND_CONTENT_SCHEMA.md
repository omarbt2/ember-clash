# Data and Content Schema

| Field | Value |
|---|---|
| Purpose | JSON content formats, validation and versioning. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 03, 07, 09 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

All files carry `"schema":N`. Loader validates and fails fast with file/line message; game falls back to built-in defaults only for settings, never for fighter/arena data.
## 1. Fighter
```json
{"schema":1,"id":"kaelen","name":"Kaelen Ashforge","weight":100,
 "body":{"w":48,"h":64},"move":{"walk":6.0,"run":8.5,"air":5.0,"fall":12,"jumps":2},
 "moves":["jab","ftilt","fsmash","special_n"],"hooks":["forge_heat"],
 "ai_tags":{"ftilt":["poke"],"fsmash":["kill"]},"palette":"palette.json","anim":"anim.json"}
```
## 2. Move catalog (P3 task 1 — partially verified)
The first C++ loader is `data::load_move_catalog` / `data::parse_move_catalog` in
`src/Data/MoveContent.*`. The version-1 root is `{ "schema": 1, "moves": [...] }`.
Each move requires a lowercase unique `id`, `category` (`ground`, `air`, `special`,
`grab`, or `throw`), `startup` (1..60), `active` (1..60), `recovery` (0..120),
and 1..64 hitbox keyframes. A keyframe requires inclusive `t0`/`t1` within the
move's active window, shape (`aabb` or `circle`), two-number `off` and positive
`size` arrays, damage (0..60), angle (0..361), base knockback (0..300), knockback
scale (0..100), hitstun (0..180), and group (0..255). Optional `fx` is a string
up to 64 bytes. Optional `cancel` requires an ordered in-duration `from` / `to_tick`
window and 1..32 unique target IDs that resolve within the same catalog.

The loader rejects unsupported schema versions, duplicate object keys and move
IDs, malformed JSON, invalid UTF-8, non-finite values, invalid ranges, empty hitbox
lists, and unresolved cancel references. It parses into a candidate catalog and
leaves the caller's existing catalog unchanged on any failure. Input is bounded to
4 MiB and nesting to 64 levels. Unknown fields are tolerated and returned as explicit catalog warnings. The keyframe evaluator in `src/Data/HitboxEngine.*` returns mirrored world-space AABB/circle geometry for a move tick and tests circle-vs-AABB overlap exactly. It is currently standalone: `Attack.cpp` remains the runtime source of truth until Match consumes the loaded catalog.

**Roster reconciliation:** the P3 work-package says 19 data-driven moves, but the
verified `AttackId` enum/table currently contains 16 executable moves (excluding
`None`). `assets/data/moves.json` currently mirrors those 16; the remaining three
moves must be defined from approved fighter/design content rather than invented by
the loader. D-010 (JSON + C++ hooks) remains PROPOSED pending owner review.
## 3. Projectile
`{"schema":1,"id":"ember_bolt","lifetime":120,"speed":9.0,"shape":{"type":"circle","r":10},"damage":6,"kb":{"base":22,"scale":4},"group":0,"max_live":3,"priority":3}`
## 4. Arena
`{"schema":1,"id":"ashen_forge","platforms":[{"x":190,"y":560,"w":900,"h":80,"pass":false},{"x":350,"y":400,"w":200,"h":24,"pass":true}],"blast":{"l":-280,"r":1560,"t":-240,"b":940},"spawns":[[300,500],[520,500],[760,500],[980,500]],"camera":{"x":-100,"y":-80,"w":1480,"h":900},"hazards":[],"music":"arena_forge"}`
## 5. Modes, animation, audio, settings
Mode: `{"id":"ffa","min_players":3,"max_players":4,"teams":false,"stocks":3,"ranked":false}`. Animation: see 09 section 4. Audio metadata: `{"id","file","bus","loop":[start,end],"gain"}`. Settings: key bindings, volumes, accessibility, versioned with migration functions.
## 6. Validation rules
Ranges (e.g. startup 1-60, damage 0-60), unique ids, referenced files exist, move total ticks equals clip duration, arena spawns inside camera, blast outside platforms. Unknown fields: warning; missing optional: default (documented per field); missing required or invalid range: load failure naming the field. Backward compatibility: loaders support schema N and N-1 with migration; replay files pin `content_hash`.
