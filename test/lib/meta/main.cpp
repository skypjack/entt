#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "types.h"

ENTT_API void share(entt::locator<entt::meta_ctx>::node_type);
ENTT_API void set_up();
ENTT_API void tear_down();
ENTT_API entt::meta_any wrap_int(int);

TEST(Lib, Meta) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve("position"_hs));
    ASSERT_FALSE(entt::resolve("velocity"_hs));

    share(entt::locator<entt::meta_ctx>::handle());
    set_up();
    entt::meta<double>().conv<int>();

    ASSERT_TRUE(entt::resolve("position"_hs));
    ASSERT_TRUE(entt::resolve("velocity"_hs));

    ASSERT_EQ(entt::resolve<position>(), entt::resolve("position"_hs));
    ASSERT_EQ(entt::resolve<velocity>(), entt::resolve("velocity"_hs));

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

    pos.reset();
    vel.reset();

    ASSERT_EQ(wrap_int(42).type(), entt::resolve<int>());
    ASSERT_EQ(wrap_int(42).cast<int>(), 42);

    tear_down();

    ASSERT_FALSE(entt::resolve("position"_hs));
    ASSERT_FALSE(entt::resolve("velocity"_hs));
}
