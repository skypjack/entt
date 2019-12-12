#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>

TEST(TypeId, Functionalities) {
    ASSERT_NE(entt::type_id_v<int>, entt::type_id_v<const int>);
    ASSERT_NE(entt::type_id_v<int>, entt::type_id_v<char>);
    ASSERT_EQ(entt::type_id_v<int>, entt::type_id_v<int>);
}
