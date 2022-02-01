#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/sigh.hpp>
#include "types.h"

struct listener {
    void on(message msg) {
        value = msg.payload;
    }

    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<message>().connect<&listener::on>(listener);

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &dispatcher;
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 42);

    dispatcher = {};
    cr_plugin_close(ctx);
}
