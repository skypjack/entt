#include <string_view>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>

template<>
struct entt::type_name<float> final {
    [[nodiscard]] static constexpr std::string_view value() noexcept {
        return std::string_view{""};
    }
};

TEST(TypeIndex, Functionalities) {
    ASSERT_EQ(entt::type_index<int>::value(), entt::type_index<int>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<char>::value());
    ASSERT_NE(entt::type_index<int>::value(), entt::type_index<int &&>::value());
    ASSERT_NE(entt::type_index<int &>::value(), entt::type_index<const int &>::value());
    ASSERT_EQ(static_cast<entt::id_type>(entt::type_index<int>{}), entt::type_index<int>::value());
}

TEST(TypeHash, Functionalities) {
    ASSERT_NE(entt::type_hash<int>::value(), entt::type_hash<const int>::value());
    ASSERT_NE(entt::type_hash<int>::value(), entt::type_hash<char>::value());
    ASSERT_EQ(entt::type_hash<int>::value(), entt::type_hash<int>::value());
    ASSERT_EQ(static_cast<entt::id_type>(entt::type_hash<int>{}), entt::type_hash<int>::value());
}

TEST(TypeName, Functionalities) {
    ASSERT_EQ(entt::type_name<int>::value(), std::string_view{"int"});
    ASSERT_EQ(entt::type_name<float>{}.value(), std::string_view{""});

    ASSERT_TRUE((entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"std::integral_constant<int, 3>"})
                || (entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"std::__1::integral_constant<int, 3>"})
                || (entt::type_name<entt::integral_constant<3>>::value() == std::string_view{"struct std::integral_constant<int,3>"}));

    ASSERT_TRUE(((entt::type_name<entt::type_list<entt::type_list<int, char>, double>>::value()) == std::string_view{"entt::type_list<entt::type_list<int, char>, double>"})
                || ((entt::type_name<entt::type_list<entt::type_list<int, char>, double>>::value()) == std::string_view{"struct entt::type_list<struct entt::type_list<int,char>,double>"}));

    ASSERT_EQ(static_cast<std::string_view>(entt::type_name<int>{}), entt::type_name<int>::value());
}

TEST(TypeInfo, Functionalities) {
    static_assert(std::is_copy_constructible_v<entt::type_info>);
    static_assert(std::is_move_constructible_v<entt::type_info>);
    static_assert(std::is_copy_assignable_v<entt::type_info>);
    static_assert(std::is_move_assignable_v<entt::type_info>);

    entt::type_info info{std::in_place_type<int>};
    entt::type_info other{std::in_place_type<void>};

    ASSERT_EQ(info, entt::type_info{std::in_place_type<int &>});
    ASSERT_EQ(info, entt::type_info{std::in_place_type<int &&>});
    ASSERT_EQ(info, entt::type_info{std::in_place_type<const int &>});

    ASSERT_NE(info, other);
    ASSERT_TRUE(info == info);
    ASSERT_FALSE(info != info);

    ASSERT_EQ(info.index(), entt::type_index<int>::value());
    ASSERT_EQ(info.hash(), entt::type_hash<int>::value());
    ASSERT_EQ(info.name(), entt::type_name<int>::value());

    other = info;

    ASSERT_EQ(other.index(), entt::type_index<int>::value());
    ASSERT_EQ(other.hash(), entt::type_hash<int>::value());
    ASSERT_EQ(other.name(), entt::type_name<int>::value());

    ASSERT_EQ(other.index(), info.index());
    ASSERT_EQ(other.hash(), info.hash());
    ASSERT_EQ(other.name(), info.name());

    other = std::move(info);

    ASSERT_EQ(other.index(), entt::type_index<int>::value());
    ASSERT_EQ(other.hash(), entt::type_hash<int>::value());
    ASSERT_EQ(other.name(), entt::type_name<int>::value());

    ASSERT_EQ(other.index(), info.index());
    ASSERT_EQ(other.hash(), info.hash());
    ASSERT_EQ(other.name(), info.name());
}

TEST(TypeInfo, Order) {
    entt::type_info rhs = entt::type_id<int>();
    entt::type_info lhs = entt::type_id<char>();

    // let's adjust the two objects since values are generated at runtime
    rhs < lhs ? void() : std::swap(lhs, rhs);

    ASSERT_FALSE(lhs < lhs);
    ASSERT_FALSE(rhs < rhs);

    ASSERT_LT(rhs, lhs);
    ASSERT_LE(rhs, lhs);

    ASSERT_GT(lhs, rhs);
    ASSERT_GE(lhs, rhs);
}

TEST(TypeId, Functionalities) {
    const int value = 42;

    ASSERT_EQ(entt::type_id(value), entt::type_id<int>());
    ASSERT_EQ(entt::type_id(42), entt::type_id<int>());

    ASSERT_EQ(entt::type_id<int>(), entt::type_id<int>());
    ASSERT_EQ(entt::type_id<int &>(), entt::type_id<int &&>());
    ASSERT_EQ(entt::type_id<int &>(), entt::type_id<int>());
    ASSERT_NE(entt::type_id<int>(), entt::type_id<char>());

    ASSERT_EQ(&entt::type_id<int>(), &entt::type_id<int>());
    ASSERT_NE(&entt::type_id<int>(), &entt::type_id<void>());
}
