#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>

TEST(TypeList, Functionalities) {
    using type = entt::type_list<int, char>;
    using other = entt::type_list<double>;

    ASSERT_EQ((type::size), (decltype(type::size){2}));
    ASSERT_EQ((other::size), (decltype(other::size){1}));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other, type, other>, entt::type_list<int, char, double, int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, other>, entt::type_list<int, char, double>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_cat_t<type, type>, entt::type_list<int, char, int, char>>));
    ASSERT_TRUE((std::is_same_v<entt::type_list_unique_t<entt::type_list_cat_t<type, type>>, entt::type_list<int, char>>));
}
