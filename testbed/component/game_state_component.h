#pragma once

namespace testbed {

struct game_state_component {
    int score{};
    int best_score{};
    float remaining_time{30.f};
    bool running{true};
};

} // namespace testbed
