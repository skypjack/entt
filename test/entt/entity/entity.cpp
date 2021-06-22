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

    ASSERT_EQ(traits_type::to_value(traits_type::to_entity(entity), traits_type::to_version(entity)), entity);
    ASSERT_EQ(traits_type::to_value(traits_type::to_entity(other), traits_type::to_version(other)), other);
    ASSERT_NE(traits_type::to_value(traits_type::to_integral(entity), {}), entity);

    ASSERT_EQ(traits_type::to_value(entity, entity), entity);
    ASSERT_EQ(traits_type::to_value(other, other), other);
    ASSERT_NE(traits_type::to_value(entity, {}), entity);

    ASSERT_EQ(traits_type::reserved(), entt::tombstone | static_cast<entt::entity>(entt::null));
    ASSERT_EQ(traits_type::reserved(), entt::null | static_cast<entt::entity>(entt::tombstone));
}

TEST(Entity, Null) {
    using traits_type = entt::entt_traits<entt::entity>;
    constexpr entt::entity tombstone = entt::tombstone;
    constexpr entt::entity null = entt::null;

    ASSERT_FALSE(entt::entity{} == entt::null);
    ASSERT_TRUE(entt::entity{traits_type::reserved()} == entt::null);

    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ((entt::null | entity), (traits_type::to_value(entt::null, entity)));
    ASSERT_EQ((entt::null | null), null);
    ASSERT_EQ((entt::null | tombstone), null);

    registry.emplace<int>(entity, 42);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);

    const entt::entity other = entt::null;

    ASSERT_FALSE(registry.valid(other));
    ASSERT_NE(registry.create(other), other);
}

TEST(Entity, Tombstone) {
    using traits_type = entt::entt_traits<entt::entity>;
    constexpr entt::entity tombstone = entt::tombstone;
    constexpr entt::entity null = entt::null;

    ASSERT_FALSE(entt::entity{} == entt::tombstone);
    ASSERT_TRUE(entt::entity{traits_type::reserved()} == entt::tombstone);

    ASSERT_TRUE(entt::tombstone == entt::tombstone);
    ASSERT_FALSE(entt::tombstone != entt::tombstone);

    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ((entt::tombstone | entity), (traits_type::to_value(entity, entt::tombstone)));
    ASSERT_EQ((entt::tombstone | tombstone), tombstone);
    ASSERT_EQ((entt::tombstone | null), tombstone);

    registry.emplace<int>(entity, 42);

    ASSERT_FALSE(entity == entt::tombstone);
    ASSERT_FALSE(entt::tombstone == entity);

    ASSERT_TRUE(entity != entt::tombstone);
    ASSERT_TRUE(entt::tombstone != entity);

    const auto vers = traits_type::to_version(entt::tombstone);
    const auto other = traits_type::to_value(traits_type::to_entity(entity), vers);

    ASSERT_FALSE(registry.valid(entt::tombstone));
    ASSERT_NE(registry.destroy(entity, vers), vers);
    ASSERT_NE(registry.create(other), other);
}
