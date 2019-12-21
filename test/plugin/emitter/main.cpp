#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });

    cr_plugin ctx;
    ctx.userdata = &emitter;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 42);

    cr_plugin_close(ctx);
}
