#include <entt/entity/registry.hpp>
#include <gtest/gtest.h>
#include "component.h"

extern typename entt::registry<>::component_type a_module_int_type();
extern typename entt::registry<>::component_type a_module_char_type();
extern typename entt::registry<>::component_type another_module_int_type();
extern typename entt::registry<>::component_type another_module_char_type();

extern void update_position(int delta, entt::registry<> &);
extern void assign_velocity(int, entt::registry<> &);

ENTT_SHARED_TYPE(int)
ENTT_SHARED_TYPE(char)

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

TEST(Lib, PositionVelocity) {
    entt::registry<> registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i+1);
    }

    assign_velocity(2, registry);

    ASSERT_EQ(registry.size<position>(), entt::registry<>::size_type{3});
    ASSERT_EQ(registry.size<velocity>(), entt::registry<>::size_type{3});

    update_position(1, registry);

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entity + 2);
        ASSERT_EQ(position.y, entity + 3);
    });
}
