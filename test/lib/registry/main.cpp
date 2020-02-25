#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include "types.h"

ENTT_API void update_position(entt::registry &);
ENTT_API void assign_velocity(entt::registry &);

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);
    }

    assign_velocity(registry);
    update_position(registry);

    ASSERT_EQ(registry.size<position>(), registry.size<velocity>());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entt::to_integer(entity) + 16);
        ASSERT_EQ(position.y, entt::to_integer(entity) + 16);
    });
}
