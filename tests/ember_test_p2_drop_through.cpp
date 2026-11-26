#include "Gameplay/Fighter.h"
#include "World/Arena.h"
#include <cassert>
#include <cmath>
int main() {
    using namespace ember;
    Arena arena;
    Fighter fighter(0, arena);
    const auto& ledge = arena.platforms()[1].aabb;
    fighter.body() = Aabb{350.0f, ledge.y - Fighter::kHeight,
                          Fighter::kWidth, Fighter::kHeight};
    PlayerIntent intent{};
    intent.fast_fall_held = true;
    intent.jump_pressed = true;
    fighter.update(intent, CombatIntent{}, arena);
    assert(fighter.body().bottom() > ledge.y);
    assert(!fighter.is_grounded());
    return 0;
}
