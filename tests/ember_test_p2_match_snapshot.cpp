#include "Gameplay/Match.h"
#include "Input/InputManager.h"

#include <cassert>
#include <type_traits>

using namespace ember;

static_assert(std::is_trivially_copyable_v<Match::Snapshot>);

int main() {
    Match match;
    InputManager input;
    match.force_live_state_for_test();
    match.start_training_recording();
    match.update(input);
    match.stop_training_recording();

    // Capture a live ledge-hang state as well as the attached AI and training
    // recorder state, so the new P2 recovery fields are part of the snapshot.
    Fighter& p1 = match.fighter(0);
    const Aabb& main_stage = match.arena().platforms()[0].aabb;
    p1.body() = Aabb{main_stage.x - Fighter::kWidth - 10.0f,
                     main_stage.y + 20.0f,
                     Fighter::kWidth, Fighter::kHeight};
    p1.velocity() = Vec2{0.0, 4.0};
    p1.set_grounded_for_test(false);
    p1.update(PlayerIntent{}, CombatIntent{}, match.arena());
    assert(p1.is_ledge_hanging());

    const Match::Snapshot snapshot = match.capture_state();
    const std::uint64_t checkpoint_hash = match.state_hash();
    assert(snapshot.version == 1);
    assert(snapshot.has_ai[1]);
    assert(snapshot.training.count == 1);
    assert(snapshot.fighters[0].state == Fighter::State::LedgeHang);
    assert(snapshot.fighters[0].ledge_invulnerability_used);

    match.fighter(0).body().x += 47.0f;
    match.fighter(0).apply_hit(12, Vec2{3.0, -5.0}, 24);
    assert(match.state_hash() != checkpoint_hash);
    assert(match.restore_state(snapshot));
    assert(match.state_hash() == checkpoint_hash);
    assert(match.is_ai(1));
    assert(match.training_recorded_ticks() == 1);
    assert(match.fighter(0).ledge_invulnerability_used());

    // An AI controller snapshot preserves even the xorshift absorbing-zero
    // state exactly; restore must not silently reseed a captured checkpoint.
    AiController zero_rng;
    zero_rng.set_runtime_state_for_test(AiController::State::Idle, 7, -1, 0);
    const AiController::Snapshot zero_rng_snapshot = zero_rng.capture();
    AiController restored_zero_rng;
    assert(restored_zero_rng.restore(zero_rng_snapshot));
    assert(restored_zero_rng.rng_state() == 0);

    // Replaying from an identical snapshot must produce the same next-tick
    // hash, including restored AI RNG and training recorder state.
    match.update(input);
    const std::uint64_t first_branch_hash = match.state_hash();
    assert(match.restore_state(snapshot));
    match.update(input);
    assert(match.state_hash() == first_branch_hash);

    // Reject unsupported versions atomically.
    Match::Snapshot invalid = snapshot;
    invalid.version = 99;
    const std::uint64_t before_rejection = match.state_hash();
    assert(!match.restore_state(invalid));
    assert(match.state_hash() == before_rejection);

    // Reject invalid animation cursors atomically as well.
    invalid = snapshot;
    invalid.fighters[0].animation.ticks_until_next = 10000;
    assert(!match.restore_state(invalid));
    assert(match.state_hash() == before_rejection);
    return 0;
}
