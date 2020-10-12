#include <array>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>

TEST(TypeTraits, SizeOf) {
    static_assert(entt::size_of_v<void> == 0u);
    static_assert(entt::size_of_v<char> == sizeof(char));
    static_assert(entt::size_of_v<int[]> == 0u);
    static_assert(entt::size_of_v<int[3]> == sizeof(int[3]));
}

TEST(TypeTraits, UnpackAsType) {
    ASSERT_EQ([](auto &&... args) {
        return [](entt::unpack_as_t<int, decltype(args)>... value) {
            return (value + ... + 0);
        };
    }('c', 42., true)(1, 2, 3), 6);
}

TEST(TypeTraits, UnpackAsValue) {
    ASSERT_EQ([](auto &&... args) {
        return (entt::unpack_as_v<2, decltype(args)> + ... + 0);
    }('c', 42., true), 6);
}

TEST(TypeTraits, IntegralConstant) {
    entt::integral_constant<3> constant;

    static_assert(std::is_same_v<typename entt::integral_constant<3>::value_type, int>);
    static_assert(constant.value == 3);
}

TEST(TypeTraits, Choice) {
    static_assert(std::is_base_of_v<entt::choice_t<0>, entt::choice_t<1>>);
    static_assert(!std::is_base_of_v<entt::choice_t<1>, entt::choice_t<0>>);
}

TEST(TypeTraits, TypeList) {
    using type = entt::type_list<int, char>;
    using other = entt::type_list<double>;

    static_assert(type::size == 2u);
    static_assert(other::size == 1u);

    static_assert(std::is_same_v<decltype(type{} + other{}), entt::type_list<int, char, double>>);
    static_assert(std::is_same_v<entt::type_list_cat_t<type, other, type, other>, entt::type_list<int, char, double, int, char, double>>);
    static_assert(std::is_same_v<entt::type_list_cat_t<type, other>, entt::type_list<int, char, double>>);
    static_assert(std::is_same_v<entt::type_list_cat_t<type, type>, entt::type_list<int, char, int, char>>);
    static_assert(std::is_same_v<entt::type_list_unique_t<entt::type_list_cat_t<type, type>>, entt::type_list<int, char>>);

    static_assert(entt::type_list_contains_v<type, int>);
    static_assert(entt::type_list_contains_v<type, char>);
    static_assert(!entt::type_list_contains_v<type, double>);
}

TEST(TypeTraits, IsEqualityComparable) {
    static_assert(entt::is_equality_comparable_v<int>);
    static_assert(!entt::is_equality_comparable_v<void>);
}

TEST(TypeTraits, MemberClass) {
    struct clazz {
        char foo(int) { return {}; }
        int bar(double, float) const { return {}; }
        bool quux;
    };

    static_assert(std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::foo)>>);
    static_assert(std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::bar)>>);
    static_assert(std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::quux)>>);
}

TEST(TypeTraits, Tag) {
    static_assert(entt::tag<"foobar"_hs>::value == entt::hashed_string::value("foobar"));
    static_assert(std::is_same_v<typename entt::tag<"foobar"_hs>::value_type, entt::id_type>);
}
