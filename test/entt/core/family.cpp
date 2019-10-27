#include <gtest/gtest.h>
#include <entt/core/family.hpp>

using a_family = entt::family<struct a_family_type>;
using another_family = entt::family<struct another_family_type>;

TEST(Family, Functionalities) {
    auto t1 = a_family::type<int>;
    auto t2 = a_family::type<int>;
    auto t3 = a_family::type<char>;
    auto t4 = another_family::type<double>;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t1, t3);
    ASSERT_EQ(t1, t4);
}

TEST(Family, Uniqueness) {
    ASSERT_NE(a_family::type<int>, a_family::type<int &>);
    ASSERT_NE(a_family::type<int>, a_family::type<int &&>);
    ASSERT_NE(a_family::type<int>, a_family::type<const int &>);
}
