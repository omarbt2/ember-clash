#include "Input/InputManager.h"

#include <cassert>

using namespace ember;

int main() {
    InputManager input;
    input.add_gamepad_added_for_test(17);
    input.add_gamepad_added_for_test(28);
    input.add_gamepad_removed_for_test(17);
    assert(input.gamepad_added_count() == 2);
    assert(input.gamepad_added_id(0) == 17);
    assert(input.gamepad_added_id(1) == 28);
    assert(input.gamepad_removed_count() == 1);
    assert(input.gamepad_removed_id(0) == 17);
    assert(input.gamepad_added_id(2) == 0);

    for (int i = 0; i < InputManager::kMaxGamepadEventsPerFrame + 4; ++i)
        input.add_gamepad_added_for_test(100 + i);
    assert(input.gamepad_added_count() == InputManager::kMaxGamepadEventsPerFrame);

    input.clear_for_test();
    assert(input.gamepad_added_count() == 0);
    assert(input.gamepad_removed_count() == 0);
    return 0;
}
