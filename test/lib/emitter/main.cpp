#include <gtest/gtest.h>
#include <entt/lib/attribute.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

ENTT_API void emit(int, test_emitter &);

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    emitter.once<event>([&](event ev, test_emitter &) { value = ev.payload; });
    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });
    emitter.publish<event>(3);

    ASSERT_EQ(value, 3);

    emit(42, emitter);
    emit(3, emitter);

    ASSERT_EQ(value, 42);
}
