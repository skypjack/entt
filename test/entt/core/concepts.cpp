#include <memory>
#include <gtest/gtest.h>
#include <entt/core/concepts.hpp>

TEST(Concepts, CVRefUnqualified) {
    ASSERT_TRUE(entt::cvref_unqualified<void>);
    ASSERT_TRUE(entt::cvref_unqualified<int>);
    ASSERT_FALSE(entt::cvref_unqualified<int &>);
    ASSERT_FALSE(entt::cvref_unqualified<const int>);
    ASSERT_FALSE(entt::cvref_unqualified<const int &>);
    ASSERT_TRUE(entt::cvref_unqualified<std::shared_ptr<int>>);
    ASSERT_FALSE(entt::cvref_unqualified<const std::shared_ptr<int>>);
    ASSERT_FALSE(entt::cvref_unqualified<std::shared_ptr<int> &>);
}

TEST(Concepts, AllocatorLike) {
    ASSERT_FALSE(entt::allocator_like<int>);
    ASSERT_TRUE(entt::allocator_like<std::allocator<int>>);
    ASSERT_TRUE(entt::allocator_like<std::allocator<void>>);
    ASSERT_FALSE(entt::allocator_like<std::shared_ptr<int>>);
}
