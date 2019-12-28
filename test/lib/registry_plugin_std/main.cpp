#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include "proxy.h"
#include "types.h"

proxy::proxy(entt::registry &ref)
    : registry{&ref}
{}

void proxy::for_each(void(*cb)(position &, velocity &)) {
    registry->view<position, velocity>().each(cb);
}

void proxy::assign(velocity vel) {
    for(auto entity: registry->view<position>()) {
        registry->assign<velocity>(entity, vel);
    }
}

TEST(Lib, Registry) {
    entt::registry registry;
    proxy handler{registry};

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);
    }

    cr_plugin ctx;
    ctx.userdata = &handler;
    cr_plugin_load(ctx, PLUGIN);
    cr_plugin_update(ctx);

    ASSERT_EQ(registry.size<position>(), registry.size<velocity>());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entt::to_integral(entity) + 16);
        ASSERT_EQ(position.y, entt::to_integral(entity) + 16);
    });

    cr_plugin_close(ctx);
}
