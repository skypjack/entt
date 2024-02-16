#include <cstring>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_1 {};
struct base_2 {};
struct base_3 {};
struct derived: base_1, base_2, base_3 {};

struct MetaProp: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<base_1>()
            .type("base_1"_hs)
            .prop("int"_hs, 2);

        entt::meta<base_2>()
            .type("base_2"_hs)
            .prop("bool"_hs, false)
            .prop("char[]"_hs, "char[]");

        entt::meta<base_3>()
            .type("base_3"_hs)
            .prop("key_only"_hs)
            .prop("key"_hs, 2);

        entt::meta<derived>()
            .type("derived"_hs)
            .base<base_1>()
            .base<base_2>()
            .base<base_3>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaProp, Functionalities) {
    using namespace entt::literals;

    auto prop = entt::resolve<base_1>().prop("int"_hs);

    ASSERT_TRUE(prop);

    ASSERT_EQ(prop, prop);
    ASSERT_NE(prop, entt::meta_prop{});
    ASSERT_FALSE(prop != prop);
    ASSERT_TRUE(prop == prop);

    auto value = prop.value();
    auto cvalue = std::as_const(prop).value();

    ASSERT_NE(value.try_cast<int>(), nullptr);
    ASSERT_EQ(cvalue.try_cast<int>(), nullptr);

    ASSERT_NE(value.try_cast<const int>(), nullptr);
    ASSERT_NE(cvalue.try_cast<const int>(), nullptr);

    ASSERT_EQ(value, 2);
    ASSERT_EQ(cvalue, 2);
}

TEST_F(MetaProp, FromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived>();
    auto prop_bool = type.prop("bool"_hs);
    auto prop_int = type.prop("int"_hs);
    auto key_only = type.prop("key_only"_hs);
    auto key_value = type.prop("key"_hs);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);
    ASSERT_TRUE(key_only);
    ASSERT_TRUE(key_value);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 2);
    ASSERT_FALSE(key_only.value());
    ASSERT_EQ(key_value.value().cast<int>(), 2);
}

TEST_F(MetaProp, DeducedArrayType) {
    using namespace entt::literals;

    auto prop = entt::resolve<base_2>().prop("char[]"_hs);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.value().type(), entt::resolve<const char *>());
    ASSERT_EQ(strcmp(prop.value().cast<const char *>(), "char[]"), 0);
}

TEST_F(MetaProp, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto &&node = entt::internal::resolve<base_1>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));
    auto type = entt::resolve<base_1>();

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->prop.empty());
    ASSERT_EQ(node.details->prop.size(), 1u);

    ASSERT_TRUE(type.prop("int"_hs));
    ASSERT_EQ(type.prop("int"_hs).value().cast<int>(), 2);

    entt::meta<base_1>().prop("int"_hs, 0);
    entt::meta<base_1>().prop("double"_hs, 3.);

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->prop.empty());
    ASSERT_EQ(node.details->prop.size(), 2u);

    ASSERT_TRUE(type.prop("int"_hs));
    ASSERT_TRUE(type.prop("double"_hs));
    ASSERT_EQ(type.prop("int"_hs).value().cast<int>(), 0);
    ASSERT_EQ(type.prop("double"_hs).value().cast<double>(), 3.);
}
