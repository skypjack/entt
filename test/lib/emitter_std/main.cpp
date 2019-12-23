#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

ENTT_API void emit(test_emitter &);

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });
    emit(emitter);

    ASSERT_EQ(value, 42);
}
