#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>

TEST(TypeInfo, Functionalities) {
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<const int>::id());
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<char>::id());
    ASSERT_EQ(entt::type_info<int>::id(), entt::type_info<int>::id());
}

TEST(TypeIndex, Functionalities) {
    ASSERT_EQ(entt::type_index<int>::value(), entt::type_index<int>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<char>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<int &&>::value());
    ASSERT_NE(entt::type_index<int &>::value(), entt::type_index<const int &>::value());
}
