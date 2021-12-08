#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include "types.h"

ENTT_API void update_position(entt::registry &);
ENTT_API void emplace_velocity(entt::registry &);

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);
    }

    emplace_velocity(registry);
    update_position(registry);

    ASSERT_EQ(registry.storage<position>().size(), registry.storage<velocity>().size());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, static_cast<int>(entt::to_integral(entity) + 16));
        ASSERT_EQ(position.y, static_cast<int>(entt::to_integral(entity) + 16));
    });
}
