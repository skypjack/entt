#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_1_t {};
struct base_2_t {};
struct derived_t: base_1_t, base_2_t {};

struct MetaProp: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<base_1_t>().prop("int"_hs, 42);
        entt::meta<base_2_t>().prop("bool"_hs, false);
        entt::meta<derived_t>().base<base_1_t>().base<base_2_t>();
    }
};

TEST_F(MetaProp, Functionalities) {
    auto prop = entt::resolve<base_1_t>().prop("int"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), "int"_hs);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaProp, FromBase) {
    auto type = entt::resolve<derived_t>();
    auto prop_bool = type.prop("bool"_hs);
    auto prop_int = type.prop("int"_hs);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
}
