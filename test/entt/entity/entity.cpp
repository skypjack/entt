#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

TEST(Entity, Traits) {
    using traits_type = entt::entt_traits<entt::entity>;
    entt::registry registry{};

    registry.destroy(registry.create());
    const auto entity = registry.create();
    const auto other = registry.create();

    ASSERT_EQ(entt::to_integral(entity), traits_type::to_integral(entity));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral<entt::entity>(entt::null));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral(entt::entity{}));

    ASSERT_EQ(traits_type::to_entity(entity), 0u);
    ASSERT_EQ(traits_type::to_version(entity), 1u);
    ASSERT_EQ(traits_type::to_entity(other), 1u);
    ASSERT_EQ(traits_type::to_version(other), 0u);

    ASSERT_EQ(traits_type::to_type(traits_type::to_entity(entity), traits_type::to_version(entity)), entity);
    ASSERT_EQ(traits_type::to_type(traits_type::to_entity(other), traits_type::to_version(other)), other);
}

TEST(Entity, Null) {
    using traits_type = entt::entt_traits<entt::entity>;

    ASSERT_FALSE(entt::entity{} == entt::null);
    ASSERT_TRUE(entt::entity{traits_type::entity_mask} == entt::null);
    ASSERT_TRUE(entt::entity{~typename traits_type::entity_type{}} == entt::null);

    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    entt::registry registry{};
    auto entity = registry.create();

    registry.emplace<int>(entity, 42);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);

    ASSERT_FALSE(registry.valid(entt::null));
    ASSERT_DEATH((entity = registry.create(entt::null)), "");
}
