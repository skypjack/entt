#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include "types.h"

ENTT_API void update_position(int, entt::registry &);
ENTT_API void assign_velocity(int, entt::registry &);

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i+1);
    }

    assign_velocity(2, registry);

    ASSERT_EQ(registry.size<position>(), 3u);
    ASSERT_EQ(registry.size<velocity>(), 3u);

    update_position(1, registry);

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entt::to_integer(entity) + 2);
        ASSERT_EQ(position.y, entt::to_integer(entity) + 3);
    });
}
