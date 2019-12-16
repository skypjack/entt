#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include "types.h"

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i+1);
    }

    ASSERT_FALSE(registry.empty<position>());
    ASSERT_TRUE(registry.empty<velocity>());

    cr_plugin ctx;
    ctx.userdata = &registry;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_FALSE(registry.empty<position>());
    ASSERT_FALSE(registry.empty<velocity>());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entt::to_integer(entity) + 2);
        ASSERT_EQ(position.y, entt::to_integer(entity) + 3);
    });

    cr_plugin_close(ctx);
}
