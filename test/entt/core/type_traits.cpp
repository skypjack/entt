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

    ASSERT_TRUE((std::is_same_v<typename entt::integral_constant<3>::value_type, int>));
    ASSERT_EQ(constant.value, 3);
}

TEST(TypeTraits, Choice) {
    ASSERT_TRUE((std::is_base_of_v<entt::choice_t<0>, entt::choice_t<1>>));
    ASSERT_FALSE((std::is_base_of_v<entt::choice_t<1>, entt::choice_t<0>>));
}

TEST(TypeTraits, TypeList) {
    using type = entt::type_list<int, char>;
    using other = entt::type_list<double>;

    ASSERT_EQ(entt::type_list_size_v<type>, 2u);
    ASSERT_EQ(entt::type_list_size_v<other>, 1u);
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other, type, other>, entt::type_list<int, char, double, int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other>, entt::type_list<int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, type>, entt::type_list<int, char, int, char>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_unique_t<entt::type_list_cat_t<type, type>>, entt::type_list<int, char>>));
}

TEST(TypeTraits, IsEqualityComparable) {
    ASSERT_TRUE(entt::is_equality_comparable_v<int>);
    ASSERT_FALSE(entt::is_equality_comparable_v<void>);
}

TEST(TypeTraits, MemberClass) {
    struct clazz {
        char foo(int) { return {}; }
        int bar(double, float) const { return {}; }
        bool quux;
    };

    ASSERT_TRUE((std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::foo)>>));
    ASSERT_TRUE((std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::bar)>>));
    ASSERT_TRUE((std::is_same_v<clazz, entt::member_class_t<decltype(&clazz::quux)>>));
}

TEST(TypeTraits, Tag) {
    ASSERT_EQ(entt::tag<"foobar"_hs>::value, entt::hashed_string::value("foobar"));
    ASSERT_TRUE((std::is_same_v<typename entt::tag<"foobar"_hs>::value_type, entt::id_type>));
}
