#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/core/utility.hpp>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

struct listener {
    void on(event ev) { value = ev.payload; }
    void on(message msg) { value = msg.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher.sink<event>().connect<entt::overload<void(event)>(&listener::on)>(listener);
    dispatcher.sink<message>().connect<entt::overload<void(message)>(&listener::on)>(listener);
    dispatcher.trigger<event>(3);

    ASSERT_EQ(listener.value, 3);

    cr_plugin ctx;
    ctx.userdata = &dispatcher;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 42);

    cr_plugin_close(ctx);
}
