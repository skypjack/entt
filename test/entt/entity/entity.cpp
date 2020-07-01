#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

TEST(Entity, Null) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);

    ASSERT_FALSE(entt::entity{} == entt::null);
    ASSERT_TRUE(entt::entity{traits_type::entity_mask} == entt::null);
    ASSERT_TRUE(entt::entity{~typename traits_type::entity_type{}} == entt::null);

    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);

    ASSERT_FALSE(registry.valid(entt::null));
}
