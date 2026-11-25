#include "Gameplay/AiConfig.h"

#include <cassert>

int main() {
    using namespace ember::ai;
    assert(kEasy.reaction_delay_ticks >= 45 && kEasy.reaction_delay_ticks <= 70);
    assert(kStandard.reaction_delay_ticks == 40);
    assert(kHard.reaction_delay_ticks >= 18 && kHard.reaction_delay_ticks <= 24);
    assert(kExpert.reaction_delay_ticks >= 12 && kExpert.reaction_delay_ticks <= 16);
    assert(kExpert.reaction_delay_ticks >= 12); // never a frame-zero/superhuman read
    assert(kEasy.reaction_delay_ticks > kStandard.reaction_delay_ticks);
    assert(kStandard.reaction_delay_ticks > kHard.reaction_delay_ticks);
    assert(kHard.reaction_delay_ticks > kExpert.reaction_delay_ticks);
    assert(kExpert.recovery_priority == 100);
    assert(&from_name("easy") == &kEasy);
    assert(&from_name("standard") == &kStandard);
    assert(&from_name("hard") == &kHard);
    assert(&from_name("expert") == &kExpert);
    assert(&from_name("unknown") == &kStandard);
    assert(kEasy.seed != kStandard.seed && kStandard.seed != kHard.seed
           && kHard.seed != kExpert.seed);
    return 0;
}
