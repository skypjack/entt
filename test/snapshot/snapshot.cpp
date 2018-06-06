#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <cereal/archives/json.hpp>
#include <entt/entity/registry.hpp>

struct Position {
    float x;
    float y;
};

struct Timer {
    int duration;
    int elapsed{0};
};

struct Relationship {
    entt::DefaultRegistry::entity_type parent;
};

template<typename Archive>
void serialize(Archive &archive, Position &position) {
  archive(position.x, position.y);
}

template<typename Archive>
void serialize(Archive &archive, Timer &timer) {
  archive(timer.duration);
}

template<typename Archive>
void serialize(Archive &archive, Relationship &relationship) {
  archive(relationship.parent);
}

TEST(Snapshot, Full) {
    std::stringstream storage;

    entt::DefaultRegistry source;
    entt::DefaultRegistry destination;

    auto e0 = source.create();
    source.assign<Position>(e0, 16.f, 16.f);

    source.destroy(source.create());

    auto e1 = source.create();
    source.assign<Position>(e1, .8f, .0f);
    source.assign<Relationship>(e1, e0);

    auto e2 = source.create();

    auto e3 = source.create();
    source.assign<Timer>(e3, 1000, 100);

    source.destroy(e2);
    auto v2 = source.current(e2);

    {
        // output finishes flushing its contents when it goes out of scope
        cereal::JSONOutputArchive output{storage};
        source.snapshot().entities(output).destroyed(output)
                .component<Position, Timer, Relationship>(output);
    }

    cereal::JSONInputArchive input{storage};
    destination.restore().entities(input).destroyed(input)
            .component<Position, Timer, Relationship>(input);

    ASSERT_TRUE(destination.valid(e0));
    ASSERT_TRUE(destination.has<Position>(e0));
    ASSERT_EQ(destination.get<Position>(e0).x, 16.f);
    ASSERT_EQ(destination.get<Position>(e0).y, 16.f);

    ASSERT_TRUE(destination.valid(e1));
    ASSERT_TRUE(destination.has<Position>(e1));
    ASSERT_EQ(destination.get<Position>(e1).x, .8f);
    ASSERT_EQ(destination.get<Position>(e1).y, .0f);
    ASSERT_TRUE(destination.has<Relationship>(e1));
    ASSERT_EQ(destination.get<Relationship>(e1).parent, e0);

    ASSERT_FALSE(destination.valid(e2));
    ASSERT_EQ(destination.current(e2), v2);

    ASSERT_TRUE(destination.valid(e3));
    ASSERT_TRUE(destination.has<Timer>(e3));
    ASSERT_EQ(destination.get<Timer>(e3).duration, 1000);
    ASSERT_EQ(destination.get<Timer>(e3).elapsed, 0);
}

TEST(Snapshot, Continuous) {
    std::stringstream storage;

    entt::DefaultRegistry source;
    entt::DefaultRegistry destination;

    std::vector<entt::DefaultRegistry::entity_type> entities;
    for(auto i = 0; i < 10; ++i) {
        entities.push_back(source.create());
    }

    for(auto entity: entities) {
        source.destroy(entity);
    }

    auto e0 = source.create();
    source.assign<Position>(e0, 0.f, 0.f);
    source.assign<Relationship>(e0, e0);

    auto e1 = source.create();
    source.assign<Position>(e1, 1.f, 1.f);
    source.assign<Relationship>(e1, e0);

    auto e2 = source.create();
    source.assign<Position>(e2, .2f, .2f);
    source.assign<Relationship>(e2, e0);

    auto e3 = source.create();
    source.assign<Timer>(e3, 1000, 1000);
    source.assign<Relationship>(e3, e2);

    {
        // output finishes flushing its contents when it goes out of scope
        cereal::JSONOutputArchive output{storage};
        source.snapshot().entities(output).component<Position, Relationship, Timer>(output);
    }

    cereal::JSONInputArchive input{storage};
    entt::ContinuousLoader<entt::DefaultRegistry::entity_type> loader{destination};
    loader.entities(input)
            .component<Position, Relationship>(input, &Relationship::parent)
            .component<Timer>(input);

    ASSERT_FALSE(destination.valid(e0));
    ASSERT_TRUE(loader.has(e0));

    auto l0 = loader.map(e0);

    ASSERT_TRUE(destination.valid(l0));
    ASSERT_TRUE(destination.has<Position>(l0));
    ASSERT_EQ(destination.get<Position>(l0).x, 0.f);
    ASSERT_EQ(destination.get<Position>(l0).y, 0.f);
    ASSERT_TRUE(destination.has<Relationship>(l0));
    ASSERT_EQ(destination.get<Relationship>(l0).parent, l0);

    ASSERT_FALSE(destination.valid(e1));
    ASSERT_TRUE(loader.has(e1));

    auto l1 = loader.map(e1);

    ASSERT_TRUE(destination.valid(l1));
    ASSERT_TRUE(destination.has<Position>(l1));
    ASSERT_EQ(destination.get<Position>(l1).x, 1.f);
    ASSERT_EQ(destination.get<Position>(l1).y, 1.f);
    ASSERT_TRUE(destination.has<Relationship>(l1));
    ASSERT_EQ(destination.get<Relationship>(l1).parent, l0);

    ASSERT_FALSE(destination.valid(e2));
    ASSERT_TRUE(loader.has(e2));

    auto l2 = loader.map(e2);

    ASSERT_TRUE(destination.valid(l2));
    ASSERT_TRUE(destination.has<Position>(l2));
    ASSERT_EQ(destination.get<Position>(l2).x, .2f);
    ASSERT_EQ(destination.get<Position>(l2).y, .2f);
    ASSERT_TRUE(destination.has<Relationship>(l2));
    ASSERT_EQ(destination.get<Relationship>(l2).parent, l0);

    ASSERT_FALSE(destination.valid(e3));
    ASSERT_TRUE(loader.has(e3));

    auto l3 = loader.map(e3);

    ASSERT_TRUE(destination.valid(l3));
    ASSERT_TRUE(destination.has<Timer>(l3));
    ASSERT_EQ(destination.get<Timer>(l3).duration, 1000);
    ASSERT_EQ(destination.get<Timer>(l3).elapsed, 0);
    ASSERT_TRUE(destination.has<Relationship>(l3));
    ASSERT_EQ(destination.get<Relationship>(l3).parent, l2);
}
