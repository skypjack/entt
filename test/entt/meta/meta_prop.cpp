#include <cstring>
#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_1_t {};
struct base_2_t {};
struct base_3_t {};
struct derived_t: base_1_t, base_2_t, base_3_t {};

struct MetaProp: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<base_1_t>()
            .type("base_1"_hs)
            .prop("int"_hs, 42);

        entt::meta<base_2_t>()
            .type("base_2"_hs)
            .prop("bool"_hs, false)
            .prop("char[]"_hs, "char[]");

        entt::meta<base_3_t>()
            .type("base_3"_hs)
            .prop("key_only"_hs)
            .prop("key"_hs, 42);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_1_t>()
            .base<base_2_t>()
            .base<base_3_t>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaProp, Functionalities) {
    using namespace entt::literals;

    auto prop = entt::resolve<base_1_t>().prop("int"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaProp, FromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived_t>();
    auto prop_bool = type.prop("bool"_hs);
    auto prop_int = type.prop("int"_hs);
    auto key_only = type.prop("key_only"_hs);
    auto key_value = type.prop("key"_hs);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);
    ASSERT_TRUE(key_only);
    ASSERT_TRUE(key_value);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
    ASSERT_FALSE(key_only.value());
    ASSERT_EQ(key_value.value().cast<int>(), 42);
}

TEST_F(MetaProp, DeducedArrayType) {
    using namespace entt::literals;

    auto prop = entt::resolve<base_2_t>().prop("char[]"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.value().type(), entt::resolve<const char *>());
    ASSERT_EQ(strcmp(prop.value().cast<const char *>(), "char[]"), 0);
}

TEST_F(MetaProp, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto &&node = entt::internal::resolve<base_1_t>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));
    auto type = entt::resolve<base_1_t>();

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->prop.empty());
    ASSERT_EQ(node.details->prop.size(), 1u);

    ASSERT_TRUE(type.prop("int"_hs));
    ASSERT_EQ(type.prop("int"_hs).value().cast<int>(), 42);

    entt::meta<base_1_t>().prop("int"_hs, 0);
    entt::meta<base_1_t>().prop("double"_hs, 3.);

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->prop.empty());
    ASSERT_EQ(node.details->prop.size(), 2u);

    ASSERT_TRUE(type.prop("int"_hs));
    ASSERT_TRUE(type.prop("double"_hs));
    ASSERT_EQ(type.prop("int"_hs).value().cast<int>(), 0);
    ASSERT_EQ(type.prop("double"_hs).value().cast<double>(), 3.);
}
