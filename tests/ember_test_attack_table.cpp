#include "Gameplay/Attack.h"

#include <cassert>
#include <cmath>

using namespace ember;

int main() {
    const Attack& none = get_attack(AttackId::None);
    assert(attack_is_well_formed(none));
    assert(attack_total_frames(none) == 0);
    assert(!attack_frame_is_active(none, 0));

    for (int raw = static_cast<int>(AttackId::Jab);
         raw < static_cast<int>(AttackId::Count); ++raw) {
        const auto id = static_cast<AttackId>(raw);
        const Attack& move = get_attack(id);
        assert(move.name != nullptr);
        assert(attack_is_well_formed(move));
        assert(move.active_frames > 0);
        assert(attack_total_frames(move) > 0);
        assert(!attack_frame_is_active(move, -1));
        assert(!attack_frame_is_active(move, move.startup_frames - 1));
        assert(attack_frame_is_active(move, move.startup_frames));
        assert(attack_frame_is_active(
            move, move.startup_frames + move.active_frames - 1));
        assert(!attack_frame_is_active(
            move, move.startup_frames + move.active_frames));
        assert(!attack_frame_is_active(move, attack_total_frames(move)));
    }

    const Attack& invalid_low = get_attack(static_cast<AttackId>(-1));
    const Attack& invalid_high = get_attack(AttackId::Count);
    assert(invalid_low.name != nullptr && invalid_high.name != nullptr);
    assert(attack_total_frames(invalid_low) == 0);
    assert(attack_total_frames(invalid_high) == 0);

    Attack malformed = get_attack(AttackId::Jab);
    malformed.startup_frames = -1;
    assert(!attack_is_well_formed(malformed));
    malformed = get_attack(AttackId::Jab);
    malformed.hitbox.w = 0.0f;
    assert(!attack_is_well_formed(malformed));
    malformed = get_attack(AttackId::Jab);
    malformed.requires_air = true;
    assert(!attack_is_well_formed(malformed));
    return 0;
}
