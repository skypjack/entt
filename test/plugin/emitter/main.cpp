#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    emitter.once<event>([&](event ev, test_emitter &) { value = ev.payload; });
    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });
    emitter.publish<event>(3);

    ASSERT_EQ(value, 3);

    cr_plugin ctx;
    ctx.userdata = &emitter;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 42);

    cr_plugin_close(ctx);
}
