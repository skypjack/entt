#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

struct ActorComponent final {};
struct ActorTag final {};

TEST(Actor, Component) {
    entt::DefaultRegistry registry;
    entt::DefaultActor actor{registry};
    const auto &cactor = actor;

    ASSERT_EQ(&registry, &actor.registry());
    ASSERT_EQ(&registry, &cactor.registry());
    ASSERT_TRUE(registry.empty<ActorComponent>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<ActorComponent>());

    const auto &component = actor.assign<ActorComponent>();

    ASSERT_EQ(&component, &actor.get<ActorComponent>());
    ASSERT_EQ(&component, &cactor.get<ActorComponent>());
    ASSERT_FALSE(registry.empty<ActorComponent>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor.has<ActorComponent>());

    actor.remove<ActorComponent>();

    ASSERT_TRUE(registry.empty<ActorComponent>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<ActorComponent>());
}

TEST(Actor, Tag) {
    entt::DefaultRegistry registry;
    entt::DefaultActor actor{registry};
    const auto &cactor = actor;

    ASSERT_EQ(&registry, &actor.registry());
    ASSERT_EQ(&registry, &cactor.registry());
    ASSERT_FALSE(registry.has<ActorTag>());
    ASSERT_FALSE(actor.has<ActorTag>(entt::tag_t{}));

    const auto &tag = actor.assign<ActorTag>(entt::tag_t{});

    ASSERT_EQ(&tag, &actor.get<ActorTag>(entt::tag_t{}));
    ASSERT_EQ(&tag, &cactor.get<ActorTag>(entt::tag_t{}));
    ASSERT_TRUE(registry.has<ActorTag>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor.has<ActorTag>(entt::tag_t{}));

    actor.remove<ActorTag>(entt::tag_t{});

    ASSERT_FALSE(registry.has<ActorTag>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<ActorTag>(entt::tag_t{}));
}

TEST(Actor, EntityLifetime) {
    entt::DefaultRegistry registry;
    auto *actor = new entt::DefaultActor{registry};
    actor->assign<ActorComponent>();

    ASSERT_FALSE(registry.empty<ActorComponent>());
    ASSERT_FALSE(registry.empty());

    registry.each([actor](const auto entity) {
        ASSERT_EQ(actor->entity(), entity);
    });

    delete actor;

    ASSERT_TRUE(registry.empty<ActorComponent>());
    ASSERT_TRUE(registry.empty());
}
