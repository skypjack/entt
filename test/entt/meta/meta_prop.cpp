#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_1 {};
struct base_2 {};
struct derived: base_1, base_2 {};

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<base_1>().prop("int"_hs, 42);
        entt::meta<base_2>().prop("bool"_hs, false);
        entt::meta<derived>().base<base_1>().base<base_2>();
    }
};

TEST_F(Meta, MetaProp) {
    auto prop = entt::resolve<base_1>().prop("int"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), "int"_hs);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaPropFromBase) {
    auto type = entt::resolve<derived>();
    auto prop_bool = type.prop("bool"_hs);
    auto prop_int = type.prop("int"_hs);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
}
