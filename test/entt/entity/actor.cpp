#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

TEST(Actor, Component) {
    entt::registry registry;
    entt::actor actor{registry};

    ASSERT_EQ(&registry, &actor.backend());
    ASSERT_EQ(&registry, &std::as_const(actor).backend());
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<int>());

    const auto &cint = actor.assign<int>();
    const auto &cchar = actor.assign<char>();

    ASSERT_EQ(&cint, &actor.get<int>());
    ASSERT_EQ(&cchar, &std::as_const(actor).get<char>());
    ASSERT_EQ(&cint, &std::get<0>(actor.get<int, char>()));
    ASSERT_EQ(&cchar, &std::get<1>(actor.get<int, char>()));
    ASSERT_EQ(&cint, std::get<0>(actor.try_get<int, char, double>()));
    ASSERT_EQ(&cchar, std::get<1>(actor.try_get<int, char, double>()));
    ASSERT_EQ(nullptr, std::get<2>(actor.try_get<int, char, double>()));
    ASSERT_EQ(nullptr, actor.try_get<double>());
    ASSERT_EQ(&cchar, actor.try_get<char>());
    ASSERT_EQ(&cint, actor.try_get<int>());

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE((actor.has<int, char>()));
    ASSERT_FALSE(actor.has<double>());

    actor.remove<int>();

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<int>());
}

TEST(Actor, FromEntity) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.assign<int>(entity, 42);
    registry.assign<char>(entity, 'c');

    entt::actor actor{entity, registry};

    ASSERT_TRUE(actor);
    ASSERT_EQ(entity, actor.entity());
    ASSERT_TRUE((actor.has<int, char>()));
    ASSERT_EQ(actor.get<int>(), 42);
    ASSERT_EQ(actor.get<char>(), 'c');
}

TEST(Actor, EntityLifetime) {
    entt::registry registry;
    entt::actor actor{};

    ASSERT_FALSE(actor);

    actor = entt::actor{registry};
    actor.assign<int>();

    ASSERT_TRUE(actor);
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());

    registry.destroy(actor.entity());

    ASSERT_FALSE(actor);
}

TEST(Actor, ActorLifetime) {
    entt::registry registry;
    auto *actor = new entt::actor{registry};
    actor->assign<int>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());

    registry.each([actor](const auto entity) {
        ASSERT_EQ(actor->entity(), entity);
    });

    delete actor;

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty());
}
