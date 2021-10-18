#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &emitter;
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 42);

    emitter = {};
    cr_plugin_close(ctx);
}
