// =============================================================================
// M6-B: animation system (headless harness).
//
// The full M6-B behaviour is "the fighter sprite is no longer a single
// frame". This harness verifies the new pieces - the multi-frame Sprite
// and the anim::Driver - without bringing up SDL or a Renderer. Any visual
// confirmation belongs to a future M6-C / M6-E step.
//
// Coverage:
//   [1] Sprite::build_animation_strip produces exactly 6 frames
//       in the order documented in Sprite.h.
//   [2] P1 and P2 strips are byte-distinct (still palette-deterministic).
//   [3] Strip total opaque pixel count sits in the expected band
//       (sum across 6 frames; each frame is a 48x64 body in a 64x64 tex).
//   [4] The walk cycle has two distinct frames (legs swing).
//   [5] Every AnimationKind has a non-zero opaque silhouette.
//   [6] anim::Driver defaults to Idle / frame 0 / full timer.
//   [7] tick() with grounded + horizontal_speed=0 stays in Idle.
//   [8] tick() with grounded + horizontal_speed>0.05 switches to Walk.
//   [9] tick() with !grounded switches to Jump (even if moving).
//  [10] tick() with attacking=true switches to Attack.
//  [11] tick() with in_hitstun=true switches to Hitstun (highest priority
//       after dead).
//  [12] Walk cycles through 2 frames at the configured walk_ticks_per_frame
//       cadence.
//  [13] A fresh driver and an already-ticked driver agree on every
//       (pose, frame, timer) tuple after the same N ticks with the same
//       Context sequence - i.e. the driver is deterministic.
//  [14] reset_to_idle() always returns to Idle/0/IdleTimer regardless
//       of the current pose.
//
// Total: 5 scenarios, 14 asserts.
// =============================================================================

#include "Gameplay/Animation.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/Sprite.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using ember::Sprite;
using ember::art::kPaletteP1;
using ember::art::kPaletteP2;

namespace {

int g_pass = 0;
int g_fail = 0;
int g_scenario = 0;

void check(bool cond, const char* what) {
    if (cond) {
        ++g_pass;
        std::printf("  %-58s PASS\n", what);
    } else {
        ++g_fail;
        std::printf("  %-58s FAIL\n", what);
    }
}

void begin(const char* name) {
    ++g_scenario;
    std::printf("[%d] %s\n", g_scenario, name);
}

} // namespace

int main() {
    std::printf("=== M6-B: animation system (headless) ===\n");

    // -------------------------------------------------------------------
    // [1] build_animation_strip returns 6 frames in the expected order.
    // -------------------------------------------------------------------
    begin("build_animation_strip returns 6 frames in the expected order");
    Sprite strip_p1 = Sprite::build_animation_strip(kPaletteP1);
    check(strip_p1.frame_count() == 6, "frame_count() == 6");
    check(Sprite::first_frame_of(Sprite::AnimationKind::Idle)    == 0, "Idle offset == 0");
    check(Sprite::first_frame_of(Sprite::AnimationKind::Walk)    == 1, "Walk offset == 1");
    check(Sprite::first_frame_of(Sprite::AnimationKind::Jump)    == 3, "Jump offset == 3");
    check(Sprite::first_frame_of(Sprite::AnimationKind::Attack)  == 4, "Attack offset == 4");
    check(Sprite::first_frame_of(Sprite::AnimationKind::Hitstun) == 5, "Hitstun offset == 5");

    // -------------------------------------------------------------------
    // [2] P1 and P2 strips are byte-distinct (palette is the only
    //     difference, so the silhouettes are identical but the pixels
    //     differ).
    // -------------------------------------------------------------------
    begin("P1 and P2 strips are byte-distinct");
    Sprite strip_p2 = Sprite::build_animation_strip(kPaletteP2);
    bool p1_eq_p2 = true;
    for (int i = 0; i < 6 && p1_eq_p2; ++i) {
        // Compare index maps frame by frame - if a single index differs
        // for the same (frame, x, y), the strips are distinct.
        for (int y = 0; y < Sprite::kSize; ++y) {
            for (int x = 0; x < Sprite::kSize; ++x) {
                if (strip_p1.index_at(i, x, y) != strip_p2.index_at(i, x, y)) {
                    p1_eq_p2 = false;
                    break;
                }
            }
            if (!p1_eq_p2) break;
        }
    }
    // Index maps are identical (silhouette is the same), but pixels
    // (RGBA) must differ because the palette is different.
    bool pixels_distinct = false;
    strip_p1.set_current_frame(0);
    strip_p2.set_current_frame(0);
    const std::uint8_t* p1px = strip_p1.pixels_rgba();
    const std::uint8_t* p2px = strip_p2.pixels_rgba();
    const std::size_t pixel_count = static_cast<std::size_t>(Sprite::kSize) * Sprite::kSize;
    for (std::size_t i = 0; i + 3 < pixel_count * 4; i += 4) {
        if (p1px[i] != p2px[i] ||
            p1px[i + 1] != p2px[i + 1] ||
            p1px[i + 2] != p2px[i + 2]) {
            pixels_distinct = true;
            break;
        }
    }
    check(!p1_eq_p2 || pixels_distinct,
          "P1 and P2 pixels differ (palette is the discriminator)");

    // -------------------------------------------------------------------
    // [3] total_opaque_pixel_count sits in the expected band.
    //     Each frame's silhouette is roughly 1300-1500 opaque pixels
    //     (a 48x64 body region with 8px margin on each side, with the
    //     head/torso/arms/legs cutouts). Times 6 frames = roughly
    //     7800-9000 total. Be conservative: 6000..12000.
    // -------------------------------------------------------------------
    begin("Strip total opaque pixel count is in the expected band");
    const int total_opaque_p1 = strip_p1.total_opaque_pixel_count();
    check(total_opaque_p1 >= 6000 && total_opaque_p1 <= 12000,
          "P1 strip total opaque count in [6000, 12000]");

    // -------------------------------------------------------------------
    // [4] Walk cycle has two distinct frames (legs swing).
    // -------------------------------------------------------------------
    begin("Walk cycle has two distinct frames");
    strip_p1.set_current_frame(Sprite::first_frame_of(Sprite::AnimationKind::Walk));
    const int walk0 = strip_p1.opaque_pixel_count();
    strip_p1.set_current_frame(Sprite::first_frame_of(Sprite::AnimationKind::Walk) + 1);
    const int walk1 = strip_p1.opaque_pixel_count();
    check(walk0 != walk1, "walk frame 0 and walk frame 1 opaque counts differ");

    // -------------------------------------------------------------------
    // [5] Every AnimationKind has a non-zero opaque silhouette.
    // -------------------------------------------------------------------
    begin("Every AnimationKind has a non-zero opaque silhouette");
    bool all_non_zero = true;
    for (int k = 0; k < static_cast<int>(Sprite::AnimationKind::_Count); ++k) {
        strip_p1.set_current_frame(Sprite::first_frame_of(static_cast<Sprite::AnimationKind>(k)));
        if (strip_p1.opaque_pixel_count() <= 0) {
            all_non_zero = false;
            break;
        }
    }
    check(all_non_zero, "all 5 poses have a non-zero opaque silhouette");

    // -------------------------------------------------------------------
    // [6] anim::Driver defaults to Idle / frame 0 / full timer.
    // -------------------------------------------------------------------
    begin("anim::Driver defaults to Idle / frame 0 / full timer");
    ember::anim::Driver drv;
    check(drv.pose() == Sprite::AnimationKind::Idle, "default pose == Idle");
    check(drv.frame_in_pose() == 0,                  "default frame_in_pose == 0");
    check(drv.global_frame() == 0,                   "default global_frame == 0");
    check(drv.ticks_until_next_frame() == 30,        "default timer == 30 (idle)");

    // -------------------------------------------------------------------
    // [7] tick() with grounded + no horizontal speed stays in Idle.
    // -------------------------------------------------------------------
    begin("grounded + no horizontal speed -> Idle");
    ember::anim::Driver d7;
    d7.tick({/*grounded=*/true, /*dead=*/false, /*attacking=*/false,
             /*in_hitstun=*/false, /*horizontal_speed=*/0.0f});
    check(d7.pose() == Sprite::AnimationKind::Idle, "pose == Idle");

    // -------------------------------------------------------------------
    // [8] tick() with grounded + horizontal speed switches to Walk.
    // -------------------------------------------------------------------
    begin("grounded + horizontal speed > 0.05 -> Walk");
    ember::anim::Driver d8;
    d8.tick({true, false, false, false, 1.0f});
    check(d8.pose() == Sprite::AnimationKind::Walk,    "pose == Walk");
    check(d8.frame_in_pose() == 0,                    "first walk frame is 0");
    check(d8.global_frame() == 1,                     "global_frame == 1 (Walk start)");

    // -------------------------------------------------------------------
    // [9] tick() with !grounded switches to Jump.
    // -------------------------------------------------------------------
    begin("not grounded -> Jump (even with horizontal speed)");
    ember::anim::Driver d9;
    d9.tick({false, false, false, false, 5.0f});
    check(d9.pose() == Sprite::AnimationKind::Jump, "pose == Jump");

    // -------------------------------------------------------------------
    // [10] tick() with attacking=true switches to Attack.
    // -------------------------------------------------------------------
    begin("attacking -> Attack");
    ember::anim::Driver d10;
    d10.tick({true, false, true, false, 0.0f});
    check(d10.pose() == Sprite::AnimationKind::Attack, "pose == Attack");

    // -------------------------------------------------------------------
    // [11] tick() with in_hitstun=true switches to Hitstun.
    // -------------------------------------------------------------------
    begin("in_hitstun -> Hitstun (priority over attacking)");
    ember::anim::Driver d11;
    d11.tick({false, false, true, true, 0.0f});
    check(d11.pose() == Sprite::AnimationKind::Hitstun, "pose == Hitstun");

    // -------------------------------------------------------------------
    // [12] Walk cycles through 2 frames at walk_ticks_per_frame cadence.
    //     Default walk_ticks_per_frame = 6. So advancing the driver 6
    //     times with a walking Context should advance the frame.
    // -------------------------------------------------------------------
    begin("Walk cycles at the configured walk_ticks_per_frame cadence");
    ember::anim::Driver d12;
    const ember::anim::Context walk_ctx{true, false, false, false, 1.0f};
    // First tick: pose changes Idle -> Walk, frame 0, timer = walk_ticks.
    d12.tick(walk_ctx);
    check(d12.pose() == Sprite::AnimationKind::Walk, "first tick: pose == Walk");
    check(d12.frame_in_pose() == 0,                  "first tick: frame_in_pose == 0");
    check(d12.ticks_until_next_frame() == 6,         "first tick: timer == walk_ticks_per_frame");
    // Advance walk_ticks-1 more ticks - timer should hit 0 but not yet
    // advance the frame (we decrement AFTER the check).
    for (int i = 0; i < 5; ++i) {
        d12.tick(walk_ctx);
    }
    check(d12.frame_in_pose() == 0, "after 5 more ticks, frame_in_pose == 0");
    // One more tick: timer hits 0, frame advances to 1.
    d12.tick(walk_ctx);
    check(d12.frame_in_pose() == 1, "after 6th tick, frame_in_pose == 1");
    // Another 6 ticks -> back to 0.
    for (int i = 0; i < 6; ++i) {
        d12.tick(walk_ctx);
    }
    check(d12.frame_in_pose() == 0, "after 12 ticks, frame_in_pose == 0 (cycle)");

    // -------------------------------------------------------------------
    // [13] Determinism: same Context sequence, two fresh drivers ->
    //     same final (pose, frame, timer).
    // -------------------------------------------------------------------
    begin("Driver is deterministic for the same Context sequence");
    const std::vector<ember::anim::Context> seq = {
        {true,  false, false, false, 0.0f},  // Idle
        {true,  false, false, false, 1.5f},  // Walk
        {false, false, false, false, 1.5f},  // Jump
        {false, false, true,  false, 0.0f},  // Attack
        {false, false, false, true,  0.0f},  // Hitstun
        {true,  false, false, false, 0.0f},  // back to Idle
    };
    ember::anim::Driver a, b;
    for (const auto& c : seq) a.tick(c);
    for (const auto& c : seq) b.tick(c);
    check(a.pose() == b.pose(),                    "same pose after same sequence");
    check(a.frame_in_pose() == b.frame_in_pose(),  "same frame_in_pose after same sequence");
    check(a.ticks_until_next_frame() == b.ticks_until_next_frame(),
          "same timer after same sequence");

    // -------------------------------------------------------------------
    // [14] reset_to_idle() always returns to Idle/0/IdleTimer.
    // -------------------------------------------------------------------
    begin("reset_to_idle() always returns to Idle/0/IdleTimer");
    ember::anim::Driver d14;
    d14.tick({true, false, false, false, 1.5f}); // walk
    d14.tick({true, false, false, false, 1.5f}); // walk
    d14.reset_to_idle();
    check(d14.pose() == Sprite::AnimationKind::Idle,   "after reset: pose == Idle");
    check(d14.frame_in_pose() == 0,                    "after reset: frame_in_pose == 0");
    check(d14.ticks_until_next_frame() == 30,          "after reset: timer == 30 (idle)");

    std::printf("\nM6-B: %d pass, %d fail\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
