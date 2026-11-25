# Animation and VFX

| Field | Value |
|---|---|
| Purpose | Animation production system and VFX design. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 09, 20 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Clip list per fighter [SC-7] (47)
Locomotion (11): Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup. Ground (4): Jab, FTilt, UTilt, DTilt. Smash (6): FSmashCharge, FSmash, USmashCharge, USmash, DSmashCharge, DSmash. Aerial (5): NAir, FAir, BAir, UAir, DAir. Specials (4): SpecialN/S/U/D. Defense (3): Shield, Parry, Dodge. Grab (6): Grab, Pummel, ThrowF/B/U/D. Reactions (7): HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat. Surge (1): SurgeActivate.
Workload: 8 x 47 = 376 clips. With a shared humanoid rig, locomotion/defense/reaction clips (21) are retargeted with ~35% unique work; attack/special/grab/surge clips (26) are ~100% unique. Approximate unique clip-equivalents: 8 x (21 x 0.35 + 26) = 8 x 33.35 = 266.8. At 3 h average (spike target), that is about 800 hours (about 20 weeks full-time) of animation labor excluding concept/modeling/cleanup - this drives the schedule in 24.

## 2. Timing sync
Attack clip length (ticks) = startup + active + recovery from move data; the "active" frame range in the sprite sheet marks the swing peak. Event markers in `anim.json`: `hit_active_start`, `sfx`, `vfx`. Rule: if art and data conflict, data wins and art is revised.

## 3. VFX list and budgets
Light hit spark (<= 6 ticks, VERIFIED hit spark 6 f), heavy hit (10 ticks, screen flash <= 2 ticks), specials (per move sheet), projectile trails, shield impact, parry flash (white ring 6 ticks), dash trail, Surge aura (loop, <= 40 particles), KO burst (shake 14 ticks VERIFIED), stage atmosphere. Budgets: <= 200 live particles total, <= 8 full-screen alpha layers per frame, additive blend only for effects, fighter face/torso not covered > 6 ticks.
Validation: debug overlay count of particles; capture of worst-case 4P match for perf (20).
