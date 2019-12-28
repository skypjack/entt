#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/signal/emitter.hpp>
#include "proxy.h"
#include "types.h"

proxy::proxy(test_emitter &ref)
    : emitter{&ref}
{}

void proxy::publish(message msg) {
    emitter->publish<message>(msg);
}

void proxy::publish(event ev) {
    emitter->publish<event>(ev);
}

TEST(Lib, Emitter) {
    test_emitter emitter;
    proxy handler{emitter};
    int value{};

    ASSERT_EQ(value, 0);

    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });

    cr_plugin ctx;
    ctx.userdata = &handler;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 42);

    cr_plugin_close(ctx);
}
