#include <functional>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include "../../../common/boxed_type.h"
#include "../../../common/emitter.h"
#include "lib.h"

TEST(Lib, Emitter) {
    test::emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.on<test::boxed_int>([&](test::boxed_int msg, test::emitter &owner) {
        value = msg.value;
        owner.erase<test::boxed_int>();
    });

    emit(emitter);

    ASSERT_EQ(value, 2);
}
