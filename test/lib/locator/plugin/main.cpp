#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/locator/locator.hpp>
#include "types.h"

TEST(Lib, Locator) {
    entt::locator<service>::emplace().value = 42;

    ASSERT_EQ(entt::locator<service>::value().value, 42);

    userdata ud{};
    ud.handle = entt::locator<service>::handle();
    ud.value = 3;

    cr_plugin ctx;
    ctx.userdata = &ud;

    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(entt::locator<service>::value().value, ud.value);

    // service updates do not propagate across boundaries
    entt::locator<service>::emplace().value = 42;
    cr_plugin_update(ctx);

    ASSERT_NE(entt::locator<service>::value().value, ud.value);

    cr_plugin_close(ctx);
}
