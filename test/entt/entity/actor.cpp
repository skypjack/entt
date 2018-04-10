#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/actor.hpp>
#include <entt/entity/registry.hpp>

struct TestActor: entt::DefaultActor<unsigned int> {
    using entt::DefaultActor<unsigned int>::DefaultActor;
    void update(unsigned int) {}
};

struct ActorPosition final {};
struct ActorVelocity final {};

TEST(Actor, Functionalities) {
    entt::DefaultRegistry registry;
    TestActor *actor = new TestActor{registry};
    const auto &cactor = *actor;

    ASSERT_EQ(&registry, &actor->registry());
    ASSERT_EQ(&registry, &cactor.registry());
    ASSERT_TRUE(registry.empty<ActorPosition>());
    ASSERT_TRUE(registry.empty<ActorVelocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor->has<ActorPosition>());
    ASSERT_FALSE(actor->has<ActorVelocity>());

    const auto &position = actor->set<ActorPosition>();

    ASSERT_EQ(&position, &actor->get<ActorPosition>());
    ASSERT_EQ(&position, &cactor.get<ActorPosition>());
    ASSERT_FALSE(registry.empty<ActorPosition>());
    ASSERT_TRUE(registry.empty<ActorVelocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE(actor->has<ActorPosition>());
    ASSERT_FALSE(actor->has<ActorVelocity>());

    actor->unset<ActorPosition>();

    ASSERT_TRUE(registry.empty<ActorPosition>());
    ASSERT_TRUE(registry.empty<ActorVelocity>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(actor->has<ActorPosition>());
    ASSERT_FALSE(actor->has<ActorVelocity>());

    actor->set<ActorPosition>();
    actor->set<ActorVelocity>();

    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(registry.empty<ActorPosition>());
    ASSERT_FALSE(registry.empty<ActorVelocity>());

    delete actor;

    ASSERT_TRUE(registry.empty());
    ASSERT_TRUE(registry.empty<ActorPosition>());
    ASSERT_TRUE(registry.empty<ActorVelocity>());
}
