#include <gtest/gtest.h>
#include <entt/core/family.hpp>

template<typename Type>
using a_family = entt::family<Type, struct a_family_type>;

template<typename Type>
using another_family = entt::family<Type, struct another_family_type>;

TEST(Family, Functionalities) {
    auto t1 = a_family<int>::type;
    auto t2 = a_family<int>::type;
    auto t3 = a_family<char>::type;
    auto t4 = another_family<double>::type;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t1, t3);
    ASSERT_EQ(t1, t4);
}

TEST(Family, Uniqueness) {
    ASSERT_NE(a_family<int>::type, a_family<int &>::type);
    ASSERT_NE(a_family<int>::type, a_family<int &&>::type);
    ASSERT_NE(a_family<int>::type, a_family<const int &>::type);
}
