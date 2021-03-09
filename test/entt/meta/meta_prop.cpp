#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_1_t {};
struct base_2_t {};
struct derived_t: base_1_t, base_2_t {};

struct MetaProp: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<base_1_t>()
            .type("base_1"_hs)
            .prop("int"_hs, 42);

        entt::meta<base_2_t>()
            .type("base_2"_hs)
            .prop("bool"_hs, false);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_1_t>()
            .base<base_2_t>();
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaProp, Functionalities) {
    using namespace entt::literals;

    auto prop = entt::resolve<base_1_t>().prop("int"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), "int"_hs);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaProp, FromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived_t>();
    auto prop_bool = type.prop("bool"_hs);
    auto prop_int = type.prop("int"_hs);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
}

TEST_F(MetaProp, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto *node = entt::internal::meta_info<base_1_t>::resolve();
    auto type = entt::resolve<base_1_t>();

    ASSERT_NE(node->prop, nullptr);
    ASSERT_EQ(node->prop->next, nullptr);

    ASSERT_TRUE(type.prop("int"_hs));
    ASSERT_EQ(type.prop("int"_hs).value().cast<int>(), 42);

    entt::meta<base_1_t>().prop("double"_hs, 3.);

    ASSERT_NE(node->prop, nullptr);
    ASSERT_EQ(node->prop->next, nullptr);

    ASSERT_FALSE(type.prop("int"_hs));
    ASSERT_TRUE(type.prop("double"_hs));
    ASSERT_EQ(type.prop("double"_hs).value().cast<double>(), 3.);
}
