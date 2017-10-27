#include <gtest/gtest.h>
#include <entt/core/family.hpp>

using my_family = entt::Family<struct MyFamily>;
using your_family = entt::Family<struct YourFamily>;

TEST(Family, Functionalities) {
    auto myFamilyType = my_family::type<struct MyFamilyType>();
    auto mySameFamilyType = my_family::type<struct MyFamilyType>();
    auto myOtherFamilyType = my_family::type<struct MyOtherFamilyType>();
    auto yourFamilyType = your_family::type<struct YourFamilyType>();

    ASSERT_EQ(myFamilyType, mySameFamilyType);
    ASSERT_NE(myFamilyType, myOtherFamilyType);
    ASSERT_EQ(myFamilyType, yourFamilyType);
}

TEST(Family, Uniqueness) {
    ASSERT_EQ(my_family::type<int>(), my_family::type<int &>());
    ASSERT_EQ(my_family::type<int>(), my_family::type<int &&>());
    ASSERT_EQ(my_family::type<int>(), my_family::type<const int &>());
}
