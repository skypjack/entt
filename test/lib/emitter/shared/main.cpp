#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include "types.h"

ENTT_API void emit(test_emitter &);

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.on<message>([&](message msg, test_emitter &owner) {
        value = msg.payload;
        owner.erase<message>();
    });

    emit(emitter);

    ASSERT_EQ(value, 42);
}
