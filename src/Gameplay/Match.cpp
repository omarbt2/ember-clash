#include "Gameplay/Match.h"

#include "Core/Logger.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/InputMap.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"

namespace ember {

Match::Match() {
    reset();
}

Match::~Match() = default;

void Match::reset() {
    arena_.reset_default();
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i] = std::make_unique<Fighter>(i, arena_);
    }
}

void Match::update(const InputManager& input, double dt) {
    for (int i = 0; i < kFighterCount; ++i) {
        const PlayerIntent intent = InputMap::sample(input, i);
        fighters_[i]->update(intent, arena_, dt);
    }

    // Auto-respawn: M2 has no KO counter yet, so dead fighters simply
    // teleport back to their starting position after a brief moment off
    // screen. M3/M5 will replace this with the real KO + respawn flow.
    for (int i = 0; i < kFighterCount; ++i) {
        if (fighters_[i]->is_dead()) {
            fighters_[i]->respawn(arena_);
        }
    }
}

void Match::render(Renderer& renderer) const {
    arena_.render(renderer);
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i]->render(renderer);
    }
}

const Fighter& Match::fighter(int index) const {
    return *fighters_[index];
}

} // namespace ember
