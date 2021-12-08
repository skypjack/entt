#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/entity/registry.hpp>
#include "types.h"

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        registry.emplace<position>(registry.create(), i, i);
    }

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &registry;
    cr_plugin_update(ctx);

    ASSERT_EQ(registry.storage<position>().size(), registry.storage<velocity>().size());
    ASSERT_EQ(registry.storage<position>().size(), registry.size());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, static_cast<int>(entt::to_integral(entity) + 16u));
        ASSERT_EQ(position.y, static_cast<int>(entt::to_integral(entity) + 16u));
    });

    registry = {};
    cr_plugin_close(ctx);
}
