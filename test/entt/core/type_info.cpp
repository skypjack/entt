#include <string_view>
#include <type_traits>
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

TEST(TypeInfo, Functionalities) {
    static_assert(std::is_default_constructible_v<entt::type_info>);
    static_assert(std::is_copy_constructible_v<entt::type_info>);
    static_assert(std::is_move_constructible_v<entt::type_info>);
    static_assert(std::is_copy_assignable_v<entt::type_info>);
    static_assert(std::is_move_assignable_v<entt::type_info>);

    ASSERT_EQ(entt::type_info{}, entt::type_info{});
    ASSERT_NE(entt::type_id<int>(), entt::type_info{});
    ASSERT_NE(entt::type_id<int>(), entt::type_id<char>());

    auto info = entt::type_id<int>();
    auto other = entt::type_id(42);
    entt::type_info empty{};

    ASSERT_EQ(info, other);
    ASSERT_NE(info, empty);

    ASSERT_TRUE(info == info);
    ASSERT_FALSE(info != other);

    ASSERT_EQ(info.seq(), other.seq());
    ASSERT_EQ(info.hash(), other.hash());
    ASSERT_EQ(info.name(), other.name());

    ASSERT_EQ(info.seq(), entt::type_seq<int>::value());
    ASSERT_EQ(info.hash(), entt::type_hash<int>::value());
    ASSERT_EQ(info.name(), entt::type_name<int>::value());

    ASSERT_FALSE(empty);
    ASSERT_TRUE(info);
    ASSERT_TRUE(other);

    empty = info;

    ASSERT_TRUE(empty);
    ASSERT_EQ(empty.hash(), info.hash());

    empty = {};

    ASSERT_FALSE(empty);

    empty = std::move(other);

    ASSERT_TRUE(empty);
    ASSERT_EQ(empty.hash(), other.hash());
}