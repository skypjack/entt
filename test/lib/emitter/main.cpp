#include <gtest/gtest.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

extern void emit_event(int, test_emitter &);

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    emitter.once<int>([](int, test_emitter &) { FAIL(); });
    emitter.once<event>([&](event event, test_emitter &) {
        ASSERT_EQ(event.payload, 42);
        value = event.payload;
    });

    emit_event(42, emitter);
    emit_event(3, emitter);

    ASSERT_EQ(value, 42);
}
