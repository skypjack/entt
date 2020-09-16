#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>

struct MetaRange: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<int>().type("int"_hs);
        entt::meta<double>().type("double"_hs);
    }
};

TEST_F(MetaRange, Range) {
    entt::meta_range<entt::meta_type> range{entt::internal::meta_context::local()};
    auto it = range.begin();

    ASSERT_NE(it, range.end());
    ASSERT_TRUE(it != range.end());
    ASSERT_FALSE(it == range.end());

    ASSERT_EQ((*it).hash(), entt::resolve<double>().hash());
    ASSERT_EQ((*(++it)).hash(), entt::resolve_id("int"_hs).hash());
    ASSERT_EQ((*it++).hash(), entt::resolve<int>().hash());

    ASSERT_EQ(it, range.end());
}

TEST_F(MetaRange, EmptyRange) {
    entt::meta_range<entt::meta_data> range{};
    ASSERT_EQ(range.begin(), range.end());
}
