#pragma once

namespace testbed {

struct input_listener_component {
    enum class type {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    type command;
};

} // namespace testbed
