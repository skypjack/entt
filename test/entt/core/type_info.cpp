#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>

TEST(TypeInfo, Id) {
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<const int>::id());
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<char>::id());
    ASSERT_EQ(entt::type_info<int>::id(), entt::type_info<int>::id());
}

TEST(TypeInfo, Index) {
    ASSERT_EQ(entt::type_info<int>::index(), entt::type_info<int>::index());
    ASSERT_NE(entt::type_info<int>::index(), entt::type_info<char>::index());
    ASSERT_NE(entt::type_info<int>::index(), entt::type_info<int &&>::index());
    ASSERT_NE(entt::type_info<int &>::index(), entt::type_info<const int &>::index());
}
