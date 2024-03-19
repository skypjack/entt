#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/locator/locator.hpp>
#include "../../../common/boxed_type.h"
#include "userdata.h"

TEST(Lib, Locator) {
    entt::locator<test::boxed_int>::emplace().value = 4;

    ASSERT_EQ(entt::locator<test::boxed_int>::value().value, 4);

    userdata ud{entt::locator<test::boxed_int>::handle(), 3};

    cr_plugin ctx;
    ctx.userdata = &ud;

    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(entt::locator<test::boxed_int>::value().value, ud.value);

    // service updates do not propagate across boundaries
    entt::locator<test::boxed_int>::emplace().value = 4;
    cr_plugin_update(ctx);

    ASSERT_NE(entt::locator<test::boxed_int>::value().value, ud.value);

    cr_plugin_close(ctx);
}
