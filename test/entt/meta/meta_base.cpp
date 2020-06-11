#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_t {};
struct derived_t: base_t {};

struct MetaBase: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<base_t>().type("base"_hs);
        entt::meta<derived_t>().type("derived"_hs).base<base_t>();
    }
};

TEST_F(MetaBase, Functionalities) {
    auto base = entt::resolve<derived_t>().base("base"_hs);
    derived_t derived{};

    ASSERT_TRUE(base);
    ASSERT_EQ(base.parent(), entt::resolve_id("derived"_hs));
    ASSERT_EQ(base.type(), entt::resolve<base_t>());
    ASSERT_EQ(base.cast(&derived), static_cast<base_t *>(&derived));
}
