#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

TEST(Traits, Null) {
    entt::registry<> registry{};

    const auto entity = registry.create();
    registry.assign<int>(entity, 42);

    ASSERT_TRUE(~typename entt::registry<>::entity_type{} == entt::null);

    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);

    ASSERT_FALSE(registry.valid(entt::null));
}
