#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

ENTT_API void update(entt::registry &, int);
ENTT_API void insert(entt::registry &);

TEST(Lib, Registry) {
    constexpr auto count = 3;
    entt::registry registry;

    for(auto i = 0; i < count; ++i) {
        const auto entity = registry.create();
        registry.emplace<test::boxed_int>(entity, i);
    }

    insert(registry);
    update(registry, count);

    ASSERT_EQ(registry.storage<test::boxed_int>().size(), registry.storage<test::empty>().size());

    registry.view<test::boxed_int>().each([count](auto entity, auto &elem) {
        ASSERT_EQ(elem.value, static_cast<int>(entt::to_integral(entity) + count));
    });
}
