// Ember Clash M6 headless test harness (M6-A slice: art identity + sprite).
//
// Scenarios:
//   [1] Sprite dimensions are 64x64 and the buffer is non-zero sized.
//   [2] P1 sprite is a warm palette; P2 sprite is a cool palette. Neither
//       is empty, neither is fully opaque (the silhouette is meant to
//       have transparent edges), and they are NOT byte-identical.
//   [3] The opaque pixel count falls in a sane band (>= 600 source
//       pixels = roughly 15% of the 64x64 grid). The upper bound is
//       deliberately generous (50% of the grid) so a future richer
//       silhouette still passes; the lower bound catches "the generator
//       drew nothing".
//   [4] The index map actually uses each non-transparent slot at least
//       once. This catches a "we drew outline but forgot the body" or
//       "we drew body but no highlight" regression.
//   [5] A second Sprite instance with the same palette produces the same
//       pixel buffer (procedural determinism - critical for replay /
//       online sync; M11 will build on this guarantee).
//
// Builds against the same sources as the main game via run_m6_test.bat.
// No SDL init, no display, no filesystem. Pure C++.

#include "Gameplay/ArtConfig.h"
#include "Gameplay/Sprite.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

int g_total = 0;
int g_failed = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_total;                                                           \
        if (!(cond)) {                                                       \
            ++g_failed;                                                      \
            std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                    \
    } while (0)

#define PASS(name)  std::printf("  %-60s PASS\n", name)
#define SCENARIO(name) std::printf("[%s] %s\n", id, name)

} // namespace

int main() {
    using namespace ember;
    using Index = Sprite::Index;

    std::printf("Ember Clash M6-A test harness\n");
    std::printf("==============================\n");

    // -------------------------------------------------------------------
    // [1] sprite dimensions + buffer
    // -------------------------------------------------------------------
    {
        const char* id = "1";
        SCENARIO("Sprite dimensions are 64x64");
        Sprite p1(art::kPaletteP1);
        CHECK(p1.width()  == 64);
        CHECK(p1.height() == 64);
        CHECK(p1.pixels_rgba() != nullptr);
        // 64 * 64 * 4 = 16384 bytes. We don't assert on opaque_count
        // here (that's scenario 3); we just check the buffer is
        // accessible.
        PASS("Sprite dimensions are 64x64 and the pixel buffer is non-null");
    }

    // -------------------------------------------------------------------
    // [2] P1 vs P2 palettes are distinct + not empty + not full
    // -------------------------------------------------------------------
    {
        const char* id = "2";
        SCENARIO("P1 (warm) and P2 (cool) sprites are distinct and not degenerate");
        Sprite p1(art::kPaletteP1);
        Sprite p2(art::kPaletteP2);

        CHECK(p1.opaque_pixel_count() > 0);
        CHECK(p2.opaque_pixel_count() > 0);
        // A full-opaque sprite would mean the silhouette became a
        // filled rectangle, defeating the purpose of the art pass.
        CHECK(p1.opaque_pixel_count() < Sprite::kSize * Sprite::kSize);
        CHECK(p2.opaque_pixel_count() < Sprite::kSize * Sprite::kSize);
        // P1 and P2 must differ in raw pixel data (they are different
        // palettes). If they're identical, the palette is being ignored.
        const int cmp = std::memcmp(p1.pixels_rgba(),
                                    p2.pixels_rgba(),
                                    static_cast<std::size_t>(Sprite::kSize * Sprite::kSize * 4));
        CHECK(cmp != 0);
        PASS("P1 and P2 sprites are non-empty, non-full, and byte-distinct");
    }

    // -------------------------------------------------------------------
    // [3] opaque pixel count is in a sensible band
    // -------------------------------------------------------------------
    {
        const char* id = "3";
        SCENARIO("Silhouette covers a sane fraction of the grid");
        Sprite p1(art::kPaletteP1);
        // 64*64 = 4096. A fighter silhouette + head + arms + legs
        // should cover ~700-2000 source pixels. We leave generous
        // headroom (>= 600, <= 2048) so a richer silhouette in M6-B
        // still passes without touching this test.
        constexpr int kMinOpaque = 600;
        constexpr int kMaxOpaque = 2048;
        const int n = p1.opaque_pixel_count();
        CHECK(n >= kMinOpaque);
        CHECK(n <= kMaxOpaque);
        std::printf("    info: P1 opaque_count = %d (band %d..%d)\n",
                    n, kMinOpaque, kMaxOpaque);
        PASS("P1 opaque pixel count is in the expected band");
    }

    // -------------------------------------------------------------------
    // [4] every non-transparent palette slot is actually used
    // -------------------------------------------------------------------
    {
        const char* id = "4";
        SCENARIO("All four colour stops are present in the sprite");
        Sprite p1(art::kPaletteP1);
        bool seen[5] = {false, false, false, false, false};
        for (int y = 0; y < Sprite::kSize; ++y) {
            for (int x = 0; x < Sprite::kSize; ++x) {
                const Index i = p1.index_at(x, y);
                const int k = static_cast<int>(i);
                if (k >= 0 && k < 5) seen[k] = true;
            }
        }
        CHECK(seen[static_cast<int>(Index::Outline)]);
        CHECK(seen[static_cast<int>(Index::Body)]);
        CHECK(seen[static_cast<int>(Index::Accent)]);
        CHECK(seen[static_cast<int>(Index::Highlight)]);
        PASS("Outline + body + accent + highlight all appear in the sprite");
    }

    // -------------------------------------------------------------------
    // [5] procedural determinism
    // -------------------------------------------------------------------
    {
        const char* id = "5";
        SCENARIO("Two Sprite instances with the same palette are byte-identical");
        Sprite a(art::kPaletteP1);
        Sprite b(art::kPaletteP1);
        const int cmp = std::memcmp(a.pixels_rgba(),
                                    b.pixels_rgba(),
                                    static_cast<std::size_t>(Sprite::kSize * Sprite::kSize * 4));
        CHECK(cmp == 0);
        CHECK(a.opaque_pixel_count() == b.opaque_pixel_count());
        PASS("Sprite generation is deterministic for a given palette");
    }

    std::printf("== %s (%d failure(s)) ==\n",
                g_failed == 0 ? "ALL PASS" : "FAILURES",
                g_failed);
    return g_failed == 0 ? 0 : 1;
}
