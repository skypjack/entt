#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

TEST(Lib, Registry) {
    constexpr auto count = 3;
    entt::registry registry;

    for(auto i = 0; i < count; ++i) {
        const auto entity = registry.create();
        registry.emplace<test::boxed_int>(entity, i);
    }

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &registry;
    cr_plugin_update(ctx);

    ASSERT_EQ(registry.storage<test::boxed_int>().size(), registry.storage<test::empty>().size());
    ASSERT_EQ(registry.storage<test::boxed_int>().size(), registry.storage<entt::entity>().size());

    registry.view<test::boxed_int>().each([count](auto entity, auto &elem) {
        ASSERT_EQ(elem.value, entt::to_integral(entity) + count);
    });

    registry = {};
    cr_plugin_close(ctx);
}
