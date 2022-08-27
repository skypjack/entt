#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>

struct MetaRange: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<int>().type("int"_hs);
        entt::meta<double>().type("double"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaRange, Range) {
    using namespace entt::literals;

    auto range = entt::resolve();
    auto it = range.begin();

    ASSERT_NE(it, range.end());
    ASSERT_TRUE(it != range.end());
    ASSERT_FALSE(it == range.end());

    ASSERT_EQ(it->second.info(), entt::resolve<int>().info());
    ASSERT_EQ((++it)->second.info(), entt::resolve("double"_hs).info());
    ASSERT_EQ((it++)->second.info(), entt::resolve<double>().info());

    ASSERT_EQ(it, range.end());
}

TEST_F(MetaRange, EmptyRange) {
    entt::meta_reset();
    auto range = entt::resolve();
    ASSERT_EQ(range.begin(), range.end());
}
