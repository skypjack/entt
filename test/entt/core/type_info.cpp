#include <string_view>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>

TEST(TypeInfo, Id) {
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<const int>::id());
    ASSERT_NE(entt::type_info<int>::id(), entt::type_info<char>::id());
    ASSERT_EQ(entt::type_info<int>::id(), entt::type_info<int>::id());
}

TEST(TypeInfo, Name) {
    ASSERT_EQ(entt::type_info<int>::name(), std::string_view{"int"});

    ASSERT_TRUE((entt::type_info<entt::integral_constant<3>>::name() == std::string_view{"std::integral_constant<int, 3>"})
                || (entt::type_info<entt::integral_constant<3>>::name() == std::string_view{"std::__1::integral_constant<int, 3>"})
                || (entt::type_info<entt::integral_constant<3>>::name() == std::string_view{"struct std::integral_constant<int,3>"}));

    ASSERT_TRUE(((entt::type_info<entt::type_list<entt::type_list<int, char>, double>>::name()) == std::string_view{"entt::type_list<entt::type_list<int, char>, double>"})
                || ((entt::type_info<entt::type_list<entt::type_list<int, char>, double>>::name()) == std::string_view{"struct entt::type_list<struct entt::type_list<int,char>,double>"}));
}

TEST(TypeIndex, Functionalities) {
    ASSERT_EQ(entt::type_index<int>::value(), entt::type_index<int>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<char>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<int &&>::value());
    ASSERT_NE(entt::type_index<int &>::value(), entt::type_index<const int &>::value());
}
