#pragma once

namespace testbed {

struct input_listener_component {
    enum class type {
        NONE,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        STOP,
        PLUS,
        MINUS
    };

    type command{type::NONE};
};

} // namespace testbed
