#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>
#include "proxy.h"
#include "types.h"

proxy::proxy(entt::dispatcher &ref)
    : dispatcher{&ref}
{}

void proxy::trigger(message msg) {
    dispatcher->trigger(msg);
}

void proxy::trigger(event ev) {
    dispatcher->trigger(ev);
}

struct listener {
    void on(message msg) { value = msg.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    proxy handler{dispatcher};
    listener listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<message>().connect<&listener::on>(listener);

    cr_plugin ctx;
    ctx.userdata = &handler;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 42);

    cr_plugin_close(ctx);
}
