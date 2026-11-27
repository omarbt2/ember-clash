#include "Data/HitboxEngine.h"
#include "Data/MoveContent.h"

#include <cassert>
#include <string>

using namespace ember;

namespace {
struct MoveScenario {
    const char* id;
    int active_tick;
    float expected_x;
    float expected_y;
    float expected_w;
    float expected_h;
    int expected_damage;
    int expected_angle;
    int expected_base_kb;
    int expected_kb_scale;
    int expected_hitstun;
    Aabb defender_overlap;
};

const data::MoveDefinition* find_move(const data::MoveCatalog& catalog,
                                      const std::string& id) {
    for (const data::MoveDefinition& move : catalog.moves) {
        if (move.id == id) return &move;
    }
    return nullptr;
}
}

int main() {
    data::MoveCatalog catalog;
    data::ContentError error;
    const std::string path = std::string(EMBER_SOURCE_DIR) + "/assets/data/moves.json";
    assert(data::load_move_catalog(path, catalog, error));

    // Hand-calculated world-space fixtures for the starter moves. They catch
    // content-to-world geometry drift, wrong attack-frame boundaries, and
    // combat payload fields being lost between JSON and the evaluator.
    constexpr Aabb body{100.0f, 100.0f, 48.0f, 64.0f};
    const MoveScenario scenarios[] = {
        {"jab",       3, 112, 124, 28, 16,  4,  18,  4,  4, 12, {138, 128, 4, 8}},
        {"ftilt",     8, 124, 108, 40, 24,  9,  14,  7,  7, 18, {160, 110, 4, 8}},
        {"nair",      5,  92,  92, 64, 80,  7,  90,  5,  5, 16, {154, 100, 4, 8}},
        {"utilt",     6, 108,  76, 32, 30,  7,  79,  6,  6, 16, {138,  80, 4, 8}},
        {"dtilt",     5, 116, 144, 40, 20,  6,  53,  5,  5, 14, {154, 148, 4, 8}},
        {"fsmash",   16, 120, 106, 52, 34, 16,  27, 12, 12, 28, {170, 110, 4, 8}},
        {"usmash",   14,  96,  70, 56, 44, 14,  84, 11, 11, 26, {148,  80, 4, 8}},
        {"dsmash",   12,  84, 138, 80, 26, 13,  45, 10, 10, 24, {162, 142, 4, 8}},
        {"fair",      9, 120, 110, 40, 32,  9,  37,  8,  8, 18, {158, 116, 4, 8}},
        {"bair",      7,  66, 112, 40, 30, 10, 204,  8,  8, 18, { 66, 120, 4, 8}},
        {"uair",      6, 102,  74, 44, 32,  6,  90,  6,  6, 14, {142,  80, 4, 8}},
        {"dair",     12, 108, 150, 32, 34,  8, 287,  7,  7, 18, {138, 158, 4, 8}},
        {"special_n",10,  80,  94, 88, 76,  8,  24,  7,  7, 18, {164, 110, 4, 8}},
        {"special_s", 6, 116, 112, 40, 36,  8,  24,  7,  7, 16, {154, 120, 4, 8}},
        {"special_u", 5, 104,  80, 40, 60,  7,  84,  6,  6, 14, {140,  90, 4, 8}},
        {"special_d",14,  76, 136, 96, 30, 11,  53,  9,  9, 22, {168, 140, 4, 8}},
    };

    for (const MoveScenario& scenario : scenarios) {
        const data::MoveDefinition* move = find_move(catalog, scenario.id);
        assert(move != nullptr);
        assert(move->hitboxes.size() == 1U);
        const data::HitboxKeyframe& source = move->hitboxes.front();
        assert(scenario.active_tick == source.t0);

        assert(data::hitboxes_at_tick(*move, source.t0 - 1, body, 1).empty());
        const auto active = data::hitboxes_at_tick(*move, scenario.active_tick, body, 1);
        assert(active.size() == 1U);
        const data::ActiveHitbox& hitbox = active.front();
        assert(hitbox.bounds.x == scenario.expected_x);
        assert(hitbox.bounds.y == scenario.expected_y);
        assert(hitbox.bounds.w == scenario.expected_w);
        assert(hitbox.bounds.h == scenario.expected_h);
        assert(hitbox.damage == scenario.expected_damage);
        assert(hitbox.angle == scenario.expected_angle);
        assert(hitbox.base_knockback == scenario.expected_base_kb);
        assert(hitbox.knockback_scale == scenario.expected_kb_scale);
        assert(hitbox.hitstun == scenario.expected_hitstun);
        assert(data::hitbox_overlaps(hitbox, scenario.defender_overlap));

        // The next pixel beyond the right edge is a true miss; this catches
        // accidentally widened boxes and inclusive AABB edge contact.
        const Aabb just_outside{scenario.expected_x + scenario.expected_w,
                               scenario.expected_y + 2.0f, 1.0f, 2.0f};
        assert(!data::hitbox_overlaps(hitbox, just_outside));
        assert(data::hitboxes_at_tick(*move, source.t1 + 1, body, 1).empty());
    }

    // Left-facing Jab mirrors around the 48px body without changing its payload.
    const data::MoveDefinition* jab = find_move(catalog, "jab");
    assert(jab != nullptr);
    const auto left_jab = data::hitboxes_at_tick(*jab, 3, body, -1);
    assert(left_jab.size() == 1U);
    assert(left_jab[0].bounds.x == 108.0f);
    assert(left_jab[0].bounds.y == 124.0f);
    assert(left_jab[0].damage == 4);
    return 0;
}
