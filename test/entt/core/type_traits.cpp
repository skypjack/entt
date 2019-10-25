#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>

TEST(Choice, Functionalities) {
    ASSERT_TRUE((std::is_base_of_v<entt::choice_t<0>, entt::choice_t<1>>));
    ASSERT_FALSE((std::is_base_of_v<entt::choice_t<1>, entt::choice_t<0>>));
}

TEST(TypeList, Functionalities) {
    using type = entt::type_list<int, char>;
    using other = entt::type_list<double>;

    ASSERT_EQ(entt::type_list_size_v<type>, 2u);
    ASSERT_EQ(entt::type_list_size_v<other>, 1u);
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other, type, other>, entt::type_list<int, char, double, int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other>, entt::type_list<int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, type>, entt::type_list<int, char, int, char>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_unique_t<entt::type_list_cat_t<type, type>>, entt::type_list<int, char>>));
}

TEST(IsEqualityComparable, Functionalities) {
    ASSERT_TRUE(entt::is_equality_comparable_v<int>);
    ASSERT_FALSE(entt::is_equality_comparable_v<void>);
}
