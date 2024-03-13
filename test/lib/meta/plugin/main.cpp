#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "userdata.h"

TEST(Lib, Meta) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve("boxed_int"_hs));

    userdata ud{entt::locator<entt::meta_ctx>::handle(), entt::meta_any{}};

    cr_plugin ctx;
    ctx.userdata = &ud;

    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_TRUE(entt::resolve("boxed_int"_hs));
    ASSERT_TRUE(entt::resolve("empty"_hs));

    auto boxed_int = entt::resolve("boxed_int"_hs).construct(4.);
    auto empty = entt::resolve("empty"_hs).construct();

    ASSERT_TRUE(boxed_int);
    ASSERT_TRUE(empty);

    ASSERT_EQ(boxed_int.type().data("value"_hs).type(), entt::resolve<int>());
    ASSERT_NE(boxed_int.get("value"_hs).try_cast<int>(), nullptr);
    ASSERT_EQ(boxed_int.get("value"_hs).cast<int>(), 4);

    ASSERT_EQ(ud.any.type(), entt::resolve<int>());
    ASSERT_EQ(ud.any.cast<int>(), 4);

    // these objects have been initialized from a different context
    boxed_int.emplace<void>();
    empty.emplace<void>();
    ud.any.emplace<void>();

    cr_plugin_close(ctx);

    ASSERT_FALSE(entt::resolve("boxed_int"_hs));
    ASSERT_FALSE(entt::resolve("empty"_hs));
}
