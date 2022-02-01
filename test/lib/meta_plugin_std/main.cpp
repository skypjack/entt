#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "types.h"

TEST(Lib, Meta) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve("position"_hs));

    userdata ud{};

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &ud;
    cr_plugin_update(ctx);

    entt::meta<double>().conv<int>();

    ASSERT_TRUE(entt::resolve("position"_hs));
    ASSERT_TRUE(entt::resolve("velocity"_hs));

    auto pos = entt::resolve("position"_hs).construct(42., 3.);
    auto vel = entt::resolve("velocity"_hs).construct();

    ASSERT_TRUE(pos && vel);

    ASSERT_EQ(pos.type().data("x"_hs).type(), entt::resolve<int>());
    ASSERT_NE(pos.type().data("y"_hs).get(pos).try_cast<int>(), nullptr);
    ASSERT_EQ(pos.type().data("x"_hs).get(pos).cast<int>(), 42);
    ASSERT_EQ(pos.type().data("y"_hs).get(pos).cast<int>(), 3);

    ASSERT_EQ(vel.type().data("dx"_hs).type(), entt::resolve<double>());
    ASSERT_TRUE(vel.type().data("dy"_hs).get(vel).allow_cast<double>());
    ASSERT_EQ(vel.type().data("dx"_hs).get(vel).cast<double>(), 0.);
    ASSERT_EQ(vel.type().data("dy"_hs).get(vel).cast<double>(), 0.);

    ASSERT_EQ(ud.any.type(), entt::resolve<int>());
    ASSERT_EQ(ud.any.cast<int>(), 42);

    // these objects have been initialized from a different context
    pos.emplace<void>();
    vel.emplace<void>();
    ud.any.emplace<void>();

    cr_plugin_close(ctx);

    ASSERT_FALSE(entt::resolve("position"_hs));
    ASSERT_FALSE(entt::resolve("velocity"_hs));
}
