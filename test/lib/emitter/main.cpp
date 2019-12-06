#include <gtest/gtest.h>
#include <entt/lib/attribute.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

ENTT_API void emit(int, test_emitter &);

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    emitter.once<event>([](event, test_emitter &) {});
    emitter.once<message>([&](message msg, test_emitter &) {
        ASSERT_EQ(msg.payload, 42);
        value = msg.payload;
    });

    emit(42, emitter);
    emit(3, emitter);

    ASSERT_EQ(value, 42);
}
