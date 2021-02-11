#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>

struct not_comparable {
    bool operator==(const not_comparable &) const = delete;
};

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
    entt::integral_constant<3> constant{};

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

    static_assert(std::is_same_v<entt::type_list_element_t<0u, type>, int>);
    static_assert(std::is_same_v<entt::type_list_element_t<1u, type>, char>);
    static_assert(std::is_same_v<entt::type_list_element_t<0u, other>, double>);

    static_assert(std::is_same_v<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<float, bool>>, entt::type_list<int, char, double>>);
    static_assert(std::is_same_v<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<int, char, double>>, entt::type_list<>>);
    static_assert(std::is_same_v<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<int, char>>, entt::type_list<double>>);
    static_assert(std::is_same_v<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<char, double>>, entt::type_list<int>>);
    static_assert(std::is_same_v<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<char>>, entt::type_list<int, double>>);
}

TEST(TypeTraits, ValueList) {
    using value = entt::value_list<0, 2>;
    using other = entt::value_list<1>;

    static_assert(value::size == 2u);
    static_assert(other::size == 1u);

    static_assert(std::is_same_v<decltype(value{} + other{}), entt::value_list<0, 2, 1>>);
    static_assert(std::is_same_v<entt::value_list_cat_t<value, other, value, other>, entt::value_list<0, 2, 1, 0, 2, 1>>);
    static_assert(std::is_same_v<entt::value_list_cat_t<value, other>, entt::value_list<0, 2, 1>>);
    static_assert(std::is_same_v<entt::value_list_cat_t<value, value>, entt::value_list<0, 2, 0, 2>>);

    static_assert(entt::value_list_element_v<0u, value> == 0);
    static_assert(entt::value_list_element_v<1u, value> == 2);
    static_assert(entt::value_list_element_v<0u, other> == 1);
}

TEST(TypeTraits, IsEqualityComparable) {
    static_assert(entt::is_equality_comparable_v<int>);
    static_assert(entt::is_equality_comparable_v<std::vector<int>>);
    static_assert(entt::is_equality_comparable_v<std::vector<std::vector<int>>>);
    static_assert(entt::is_equality_comparable_v<std::unordered_map<int, int>>);
    static_assert(entt::is_equality_comparable_v<std::unordered_map<int, std::unordered_map<int, char>>>);

    static_assert(!entt::is_equality_comparable_v<not_comparable>);
    static_assert(!entt::is_equality_comparable_v<std::vector<not_comparable>>);
    static_assert(!entt::is_equality_comparable_v<std::vector<std::vector<not_comparable>>>);
    static_assert(!entt::is_equality_comparable_v<std::unordered_map<int, not_comparable>>);
    static_assert(!entt::is_equality_comparable_v<std::unordered_map<int, std::unordered_map<int, not_comparable>>>);

    static_assert(!entt::is_equality_comparable_v<void>);
}

TEST(TypeTraits, IsApplicable) {
    static_assert(entt::is_applicable_v<void(int, char), std::tuple<double, char>>);
    static_assert(!entt::is_applicable_v<void(int, char), std::tuple<int>>);

    static_assert(entt::is_applicable_r_v<float, int(int, char), std::tuple<double, char>>);
    static_assert(!entt::is_applicable_r_v<float, void(int, char), std::tuple<double, char>>);
    static_assert(!entt::is_applicable_r_v<int, int(int, char), std::tuple<void>>);
}

TEST(TypeTraits, IsComplete) {
    static_assert(entt::is_complete_v<int>);
    static_assert(!entt::is_complete_v<void>);
}

TEST(TypeTraits, IsStdHashable) {
    static_assert(entt::is_std_hashable_v<int>);
    static_assert(!entt::is_std_hashable_v<not_comparable>);
}

TEST(TypeTraits, ConstnessAs) {
    static_assert(std::is_same_v<entt::constness_as_t<int, char>, int>);
    static_assert(std::is_same_v<entt::constness_as_t<const int, char>, int>);
    static_assert(std::is_same_v<entt::constness_as_t<int, const char>, const int>);
    static_assert(std::is_same_v<entt::constness_as_t<const int, const char>, const int>);
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
    using namespace entt::literals;
    static_assert(entt::tag<"foobar"_hs>::value == entt::hashed_string::value("foobar"));
    static_assert(std::is_same_v<typename entt::tag<"foobar"_hs>::value_type, entt::id_type>);
}
