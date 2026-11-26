#include "Gameplay/Match.h"
#include "Gameplay/CombatConfig.h"
#include "Input/InputManager.h"

#include <SDL3/SDL.h>

#include <cassert>
#include <memory>

using namespace ember;

namespace {
void prepare_two_humans(Match& match, InputManager& input) {
    match.set_ai(1, nullptr);
    match.force_live_state_for_test();
    input.clear_for_test();
    // Keep both fighters grounded and put P2 inside P1's Jab reach.
    match.fighter(0).body().x = 220.0f;
    match.fighter(1).body().x = 250.0f;
    match.fighter(1).body().y = match.fighter(0).body().y;
}

void tick(Match& match, InputManager& input) {
    match.update(input);
    input.begin_frame_for_test();
}
}

int main() {
    {
        Match match;
        InputManager input;
        prepare_two_humans(match, input);

        input.set_held_for_test(SDLK_J, true);
        tick(match, input);
        for (int i = 0; i < 18; ++i) tick(match, input);

        // Jab has a two-frame active window. A single swing must not deal
        // damage once per active frame (or again during hit-stop recovery).
        assert(match.fighter(1).damage_percent() == 4);
    }

    {
        Match match;
        InputManager input;
        prepare_two_humans(match, input);

        input.set_held_for_test(SDLK_J, true);
        input.set_held_for_test(SDLK_KP_5, true); // P2 shield/parry
        tick(match, input);
        for (int i = 0; i < 10; ++i) tick(match, input);

        // P2's early guard parries the Jab: no damage, and the attacker is
        // punished with the configured parry stun rather than bypassing guard.
        assert(match.fighter(1).damage_percent() == 0);
        assert(match.fighter(0).state() == Fighter::State::Hitstun);
        assert(match.fighter(0).hitstun_frames() > 0);
        assert(match.last_hit_position_for_test().valid);
    }

    {
        Match match;
        InputManager input;
        prepare_two_humans(match, input);
        match.fighter(0).set_surge_meter_for_test(combat::kSurgeMeterMax);
        input.set_held_for_test(SDLK_Y, true); // activate P1 Surge
        input.set_held_for_test(SDLK_J, true); // and start Jab in the same tick
        tick(match, input);
        assert(match.fighter(0).surge_active());
        for (int i = 0; i < 18; ++i) tick(match, input);
        assert(match.fighter(1).damage_percent() == 5); // round(4 * 1.25)
    }

    {
        Arena arena;
        Fighter fighter(0, arena);
        fighter.set_invuln_frames_for_test(2);
        const auto ignored = fighter.receive_hit(10, Vec2{1.0, -1.0}, 12, 1);
        assert(ignored == Fighter::HitOutcome::Ignored);
        assert(fighter.damage_percent() == 0);
    }
    return 0;
}
