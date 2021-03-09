#include <gtest/gtest.h>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct base_t {
    base_t() = default;
    int value;
};

struct derived_t: base_t {
    derived_t() = default;
};

struct MetaBase: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_t>();
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaBase, Functionalities) {
    auto any = entt::resolve<derived_t>().construct();
    any.cast<derived_t &>().value = 42;
    auto as_derived = any.as_ref();

    ASSERT_TRUE(any.allow_cast<base_t &>());

    ASSERT_FALSE(any.allow_cast<char>());
    ASSERT_FALSE(as_derived.allow_cast<char>());

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<base_t &>().value, as_derived.cast<derived_t &>().value);

    any.cast<base_t &>().value = 3;

    ASSERT_EQ(any.cast<const base_t &>().value, as_derived.cast<const derived_t &>().value);
}

TEST_F(MetaBase, ReRegistration) {
    SetUp();

    auto *node = entt::internal::meta_info<derived_t>::resolve();

    ASSERT_NE(node->base, nullptr);
    ASSERT_EQ(node->base->next, nullptr);
}
