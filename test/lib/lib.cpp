#include <entt/entity/registry.hpp>
#include <gtest/gtest.h>

extern typename entt::registry<>::component_type a_module_int_type();
extern typename entt::registry<>::component_type a_module_char_type();
extern typename entt::registry<>::component_type another_module_int_type();
extern typename entt::registry<>::component_type another_module_char_type();

TEST(Lib, Shared) {
    entt::registry<> registry;

    ASSERT_EQ(registry.type<int>(), registry.type<const int>());
    ASSERT_EQ(registry.type<char>(), registry.type<const char>());

    ASSERT_EQ(registry.type<int>(), a_module_int_type());
    ASSERT_EQ(registry.type<char>(), a_module_char_type());
    ASSERT_EQ(registry.type<const int>(), a_module_int_type());
    ASSERT_EQ(registry.type<const char>(), a_module_char_type());

    ASSERT_EQ(registry.type<const char>(), another_module_char_type());
    ASSERT_EQ(registry.type<const int>(), another_module_int_type());
    ASSERT_EQ(registry.type<char>(), another_module_char_type());
    ASSERT_EQ(registry.type<int>(), another_module_int_type());
}
