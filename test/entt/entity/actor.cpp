#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

struct TestActor: entt::DefaultActor<unsigned int> {
    using entt::DefaultActor<unsigned int>::DefaultActor;
    void update(unsigned int) {}
};

struct Position final {};
struct Velocity final {};

TEST(Actor, Functionalities) {
    entt::DefaultRegistry registry;
    TestActor *actor = new TestActor{registry};
    const auto &cactor = *actor;

    ASSERT_EQ(&registry, &actor->registry());
    ASSERT_EQ(&registry, &cactor.registry());
    ASSERT_TRUE(registry.empty<Position>());
    ASSERT_TRUE(registry.empty<Velocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor->has<Position>());
    ASSERT_FALSE(actor->has<Velocity>());

    const auto &position = actor->set<Position>();

    ASSERT_EQ(&position, &actor->get<Position>());
    ASSERT_EQ(&position, &cactor.get<Position>());
    ASSERT_FALSE(registry.empty<Position>());
    ASSERT_TRUE(registry.empty<Velocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor->has<Position>());
    ASSERT_FALSE(actor->has<Velocity>());

    actor->unset<Position>();

    ASSERT_TRUE(registry.empty<Position>());
    ASSERT_TRUE(registry.empty<Velocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor->has<Position>());
    ASSERT_FALSE(actor->has<Velocity>());

    actor->set<Position>();
    actor->set<Velocity>();

    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(registry.empty<Position>());
    ASSERT_FALSE(registry.empty<Velocity>());

    delete actor;

    ASSERT_TRUE(registry.empty());
    ASSERT_TRUE(registry.empty<Position>());
    ASSERT_TRUE(registry.empty<Velocity>());
}
