#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/signal/dispatcher.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/listener.h"

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    test::listener<test::boxed_int> listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<test::boxed_int>().connect<&test::listener<test::boxed_int>::on>(listener);

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &dispatcher;
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 4);

    dispatcher = {};
    cr_plugin_close(ctx);
}
