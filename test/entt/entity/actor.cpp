#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

TEST(Actor, Component) {
    entt::registry<> registry;
    entt::actor actor{registry};
    const auto &cactor = actor;

    ASSERT_EQ(&registry, &actor.backend());
    ASSERT_EQ(&registry, &cactor.backend());
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<int>());

    const auto &cint = actor.assign<int>();
    const auto &cchar = actor.assign<char>();

    ASSERT_EQ(&cint, &actor.get<int>());
    ASSERT_EQ(&cchar, &cactor.get<char>());
    ASSERT_EQ(&cint, &std::get<0>(actor.get<int, char>()));
    ASSERT_EQ(&cchar, &std::get<1>(actor.get<int, char>()));
    ASSERT_EQ(&cint, std::get<0>(actor.get_if<int, char, double>()));
    ASSERT_EQ(&cchar, std::get<1>(actor.get_if<int, char, double>()));
    ASSERT_EQ(nullptr, std::get<2>(actor.get_if<int, char, double>()));
    ASSERT_EQ(nullptr, actor.get_if<double>());
    ASSERT_EQ(&cchar, actor.get_if<char>());
    ASSERT_EQ(&cint, actor.get_if<int>());

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor.has<int>());
    ASSERT_TRUE(actor.has<char>());
    ASSERT_FALSE(actor.has<double>());

    actor.remove<int>();

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor.has<int>());
}

TEST(Actor, EntityLifetime) {
    entt::registry<> registry;
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
