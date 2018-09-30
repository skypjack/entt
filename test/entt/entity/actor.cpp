#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

struct actor_component final {};

TEST(Actor, Component) {
    entt::registry<> registry;
    entt::actor actor{registry};
    const auto &cactor = actor;

    ASSERT_EQ(&registry, &actor.backend());
    ASSERT_EQ(&registry, &cactor.backend());
    ASSERT_TRUE(registry.empty<actor_component>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<actor_component>());

    const auto &component = actor.assign<actor_component>();

    ASSERT_EQ(&component, &actor.get<actor_component>());
    ASSERT_EQ(&component, &cactor.get<actor_component>());
    ASSERT_FALSE(registry.empty<actor_component>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor.has<actor_component>());

    actor.remove<actor_component>();

    ASSERT_TRUE(registry.empty<actor_component>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<actor_component>());
}

TEST(Actor, EntityLifetime) {
    entt::registry<> registry;
    auto *actor = new entt::actor{registry};
    actor->assign<actor_component>();

    ASSERT_FALSE(registry.empty<actor_component>());
    ASSERT_FALSE(registry.empty());

    registry.each([actor](const auto entity) {
        ASSERT_EQ(actor->entity(), entity);
    });

    delete actor;

    ASSERT_TRUE(registry.empty<actor_component>());
    ASSERT_TRUE(registry.empty());
}
