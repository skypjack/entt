#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>
#include "type_context.h"
#include "types.h"

template<typename Type>
struct entt::type_seq<Type> {
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const entt::id_type value = type_context::instance()->value(entt::type_hash<Type>::value());
        return value;
    }
};

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        registry.emplace<position>(registry.create(), i, i);
    }

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = type_context::instance();
    cr_plugin_update(ctx);

    ctx.userdata = &registry;
    cr_plugin_update(ctx);

    ASSERT_EQ(registry.size<position>(), registry.size<velocity>());
    ASSERT_EQ(registry.size<position>(), registry.size());

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, static_cast<int>(entt::to_integral(entity) + 16u));
        ASSERT_EQ(position.y, static_cast<int>(entt::to_integral(entity) + 16u));
    });

    registry = {};
    cr_plugin_close(ctx);
}
