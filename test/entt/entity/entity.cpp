#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

TEST(Traits, Null) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry registry{};
    const auto entity = registry.create();

    registry.assign<int>(entity, 42);

    ASSERT_TRUE(entt::entity{~traits_type::entity_type{}} == entt::null);

    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);

    ASSERT_FALSE(registry.valid(entt::null));
}
