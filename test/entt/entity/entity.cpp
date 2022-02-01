#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

TEST(Entity, Traits) {
    using traits_type = entt::entt_traits<entt::entity>;
    constexpr entt::entity tombstone = entt::tombstone;
    constexpr entt::entity null = entt::null;
    entt::registry registry{};

    registry.destroy(registry.create());
    const auto entity = registry.create();
    const auto other = registry.create();

    ASSERT_EQ(entt::to_integral(entity), entt::to_integral(entity));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral<entt::entity>(entt::null));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral(entt::entity{}));

    ASSERT_EQ(entt::to_entity(entity), 0u);
    ASSERT_EQ(entt::to_version(entity), 1u);
    ASSERT_EQ(entt::to_entity(other), 1u);
    ASSERT_EQ(entt::to_version(other), 0u);

    ASSERT_EQ(traits_type::construct(entt::to_entity(entity), entt::to_version(entity)), entity);
    ASSERT_EQ(traits_type::construct(entt::to_entity(other), entt::to_version(other)), other);
    ASSERT_NE(traits_type::construct(entt::to_entity(entity), {}), entity);

    ASSERT_EQ(traits_type::construct(entt::to_entity(other), entt::to_version(entity)), traits_type::combine(entt::to_integral(other), entt::to_integral(entity)));

    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::null), tombstone);
    ASSERT_EQ(traits_type::combine(entt::null, entt::tombstone), null);
}

TEST(Entity, Null) {
    using traits_type = entt::entt_traits<entt::entity>;
    constexpr entt::entity null = entt::null;

    ASSERT_FALSE(entt::entity{} == entt::null);
    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ(traits_type::combine(entt::null, entt::to_integral(entity)), (traits_type::construct(entt::to_entity(null), entt::to_version(entity))));
    ASSERT_EQ(traits_type::combine(entt::null, entt::to_integral(null)), null);
    ASSERT_EQ(traits_type::combine(entt::null, entt::tombstone), null);

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

    ASSERT_FALSE(entt::entity{} == entt::tombstone);
    ASSERT_TRUE(entt::tombstone == entt::tombstone);
    ASSERT_FALSE(entt::tombstone != entt::tombstone);

    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ(traits_type::combine(entt::to_integral(entity), entt::tombstone), (traits_type::construct(entt::to_entity(entity), entt::to_version(tombstone))));
    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::to_integral(tombstone)), tombstone);
    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::null), tombstone);

    registry.emplace<int>(entity, 42);

    ASSERT_FALSE(entity == entt::tombstone);
    ASSERT_FALSE(entt::tombstone == entity);

    ASSERT_TRUE(entity != entt::tombstone);
    ASSERT_TRUE(entt::tombstone != entity);

    constexpr auto vers = entt::to_version(tombstone);
    const auto other = traits_type::construct(entt::to_entity(entity), vers);

    ASSERT_FALSE(registry.valid(entt::tombstone));
    ASSERT_NE(registry.destroy(entity, vers), vers);
    ASSERT_NE(registry.create(other), other);
}
