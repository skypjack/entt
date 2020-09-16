#include <string_view>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>

TEST(TypeSeq, Functionalities) {
    ASSERT_EQ(entt::type_seq<int>::value(), entt::type_seq<int>::value());
    ASSERT_NE(entt::type_seq<int>::value(), entt::type_seq<char>::value());
    ASSERT_NE(entt::type_seq<int>::value(), entt::type_seq<int &&>::value());
    ASSERT_NE(entt::type_seq<int &>::value(), entt::type_seq<const int &>::value());
}

TEST(TypeHash, Functionalities) {
    ASSERT_NE(entt::type_hash<int>::value(), entt::type_hash<const int>::value());
    ASSERT_NE(entt::type_hash<int>::value(), entt::type_hash<char>::value());
    ASSERT_EQ(entt::type_hash<int>::value(), entt::type_hash<int>::value());
}

TEST(TypeName, Functionalities) {
    ASSERT_EQ(entt::type_name<int>::value(), std::string_view{"int"});

    ASSERT_TRUE((entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"std::integral_constant<int, 3>"})
        || (entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"std::__1::integral_constant<int, 3>"})
        || (entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"struct std::integral_constant<int,3>"}));

    ASSERT_TRUE(((entt::type_name<entt::type_list<entt::type_list<int, char>, double>>::value()) == std::string_view{"entt::type_list<entt::type_list<int, char>, double>"})
        || ((entt::type_name<entt::type_list<entt::type_list<int, char>, double>>::value()) == std::string_view{"struct entt::type_list<struct entt::type_list<int,char>,double>"}));
}
