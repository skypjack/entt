#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

template<typename Type>
struct entt::type_index<Type> {};

struct listener {
    void on(message msg) { value = msg.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<message>().connect<&listener::on>(listener);

    cr_plugin ctx;
    ctx.userdata = &dispatcher;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 42);

    dispatcher = {};
    cr_plugin_close(ctx);
}
