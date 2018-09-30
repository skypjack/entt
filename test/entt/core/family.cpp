#include <gtest/gtest.h>
#include <entt/core/family.hpp>

using my_family = entt::family<struct my_family_type>;
using your_family = entt::family<struct your_family_type>;

TEST(Family, Functionalities) {
    auto a_family_type = my_family::type<struct family_type_a>;
    auto same_family_type = my_family::type<struct family_type_a>;
    auto another_family_type = my_family::type<struct family_type_b>;
    auto your_family_type = your_family::type<struct family_type_c>;

    ASSERT_EQ(a_family_type, same_family_type);
    ASSERT_NE(a_family_type, another_family_type);
    ASSERT_EQ(a_family_type, your_family_type);
}

TEST(Family, Uniqueness) {
    ASSERT_EQ(my_family::type<int>, my_family::type<int &>);
    ASSERT_EQ(my_family::type<int>, my_family::type<int &&>);
    ASSERT_EQ(my_family::type<int>, my_family::type<const int &>);
}
