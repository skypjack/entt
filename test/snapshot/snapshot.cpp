#include <sstream>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <cereal/archives/json.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>

struct position {
    float x;
    float y;
};

struct timer {
    int duration;
    int elapsed{0};
};

struct relationship {
    entt::entity parent;
};

template<typename Archive>
void serialize(Archive &archive, position &position) {
    archive(position.x, position.y);
}

template<typename Archive>
void serialize(Archive &archive, timer &timer) {
    archive(timer.duration);
}

template<typename Archive>
void serialize(Archive &archive, relationship &relationship) {
    archive(relationship.parent);
}

TEST(Snapshot, Full) {
    using namespace entt::literals;

    std::stringstream storage;

    entt::registry source;
    entt::registry destination;

    auto e0 = source.create();
    source.emplace<position>(e0, 16.f, 16.f);

    source.destroy(source.create());

    auto e1 = source.create();
    source.emplace<position>(e1, .8f, .0f);
    source.emplace<relationship>(e1, e0);

    auto e2 = source.create();

    auto e3 = source.create();
    source.emplace<timer>(e3, 1000, 100);
    source.emplace<entt::tag<"empty"_hs>>(e3);

    source.destroy(e2);
    auto v2 = source.current(e2);

    {
        // output finishes flushing its contents when it goes out of scope
        cereal::JSONOutputArchive output{storage};

        entt::snapshot{source}
            .get<entt::entity>(output)
            .get<position>(output)
            .get<timer>(output)
            .get<relationship>(output)
            .get<entt::tag<"empty"_hs>>(output);
    }

    cereal::JSONInputArchive input{storage};

    entt::snapshot_loader{destination}
        .get<entt::entity>(input)
        .get<position>(input)
        .get<timer>(input)
        .get<relationship>(input)
        .get<entt::tag<"empty"_hs>>(input);

    ASSERT_TRUE(destination.valid(e0));
    ASSERT_TRUE(destination.all_of<position>(e0));
    ASSERT_EQ(destination.get<position>(e0).x, 16.f);
    ASSERT_EQ(destination.get<position>(e0).y, 16.f);

    ASSERT_TRUE(destination.valid(e1));
    ASSERT_TRUE(destination.all_of<position>(e1));
    ASSERT_EQ(destination.get<position>(e1).x, .8f);
    ASSERT_EQ(destination.get<position>(e1).y, .0f);
    ASSERT_TRUE(destination.all_of<relationship>(e1));
    ASSERT_EQ(destination.get<relationship>(e1).parent, e0);

    ASSERT_FALSE(destination.valid(e2));
    ASSERT_EQ(destination.current(e2), v2);

    ASSERT_TRUE(destination.valid(e3));
    ASSERT_TRUE(destination.all_of<timer>(e3));
    ASSERT_TRUE(destination.all_of<entt::tag<"empty"_hs>>(e3));
    ASSERT_EQ(destination.get<timer>(e3).duration, 1000);
    ASSERT_EQ(destination.get<timer>(e3).elapsed, 0);
}

TEST(Snapshot, Continuous) {
    using namespace entt::literals;

    std::stringstream storage;

    entt::registry source;
    entt::registry destination;

    std::vector<entt::entity> entity;
    for(auto i = 0; i < 10; ++i) {
        entity.push_back(source.create());
    }

    for(auto entt: entity) {
        source.destroy(entt);
    }

    auto e0 = source.create();
    source.emplace<position>(e0, 0.f, 0.f);
    source.emplace<relationship>(e0, e0);

    auto e1 = source.create();
    source.emplace<position>(e1, 1.f, 1.f);
    source.emplace<relationship>(e1, e0);

    auto e2 = source.create();
    source.emplace<position>(e2, .2f, .2f);
    source.emplace<relationship>(e2, e0);

    auto e3 = source.create();
    source.emplace<timer>(e3, 1000, 1000);
    source.emplace<relationship>(e3, e2);
    source.emplace<entt::tag<"empty"_hs>>(e3);

    {
        // output finishes flushing its contents when it goes out of scope
        cereal::JSONOutputArchive output{storage};

        entt::snapshot{source}
            .get<entt::entity>(output)
            .get<position>(output)
            .get<relationship>(output)
            .get<timer>(output)
            .get<entt::tag<"empty"_hs>>(output);
    }

    cereal::JSONInputArchive input{storage};
    entt::continuous_loader loader{destination};

    auto archive = [&input, &loader](auto &value) {
        input(value);

        if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>, relationship>) {
            value.parent = loader.map(value.parent);
        }
    };

    loader
        .get<entt::entity>(input)
        .get<position>(input)
        .get<relationship>(archive)
        .get<timer>(input)
        .get<entt::tag<"empty"_hs>>(input);

    ASSERT_FALSE(destination.valid(e0));
    ASSERT_TRUE(loader.contains(e0));

    auto l0 = loader.map(e0);

    ASSERT_TRUE(destination.valid(l0));
    ASSERT_TRUE(destination.all_of<position>(l0));
    ASSERT_EQ(destination.get<position>(l0).x, 0.f);
    ASSERT_EQ(destination.get<position>(l0).y, 0.f);
    ASSERT_TRUE(destination.all_of<relationship>(l0));
    ASSERT_EQ(destination.get<relationship>(l0).parent, l0);

    ASSERT_FALSE(destination.valid(e1));
    ASSERT_TRUE(loader.contains(e1));

    auto l1 = loader.map(e1);

    ASSERT_TRUE(destination.valid(l1));
    ASSERT_TRUE(destination.all_of<position>(l1));
    ASSERT_EQ(destination.get<position>(l1).x, 1.f);
    ASSERT_EQ(destination.get<position>(l1).y, 1.f);
    ASSERT_TRUE(destination.all_of<relationship>(l1));
    ASSERT_EQ(destination.get<relationship>(l1).parent, l0);

    ASSERT_FALSE(destination.valid(e2));
    ASSERT_TRUE(loader.contains(e2));

    auto l2 = loader.map(e2);

    ASSERT_TRUE(destination.valid(l2));
    ASSERT_TRUE(destination.all_of<position>(l2));
    ASSERT_EQ(destination.get<position>(l2).x, .2f);
    ASSERT_EQ(destination.get<position>(l2).y, .2f);
    ASSERT_TRUE(destination.all_of<relationship>(l2));
    ASSERT_EQ(destination.get<relationship>(l2).parent, l0);

    ASSERT_FALSE(destination.valid(e3));
    ASSERT_TRUE(loader.contains(e3));

    auto l3 = loader.map(e3);

    ASSERT_TRUE(destination.valid(l3));
    ASSERT_TRUE(destination.all_of<timer>(l3));
    ASSERT_EQ(destination.get<timer>(l3).duration, 1000);
    ASSERT_EQ(destination.get<timer>(l3).elapsed, 0);
    ASSERT_TRUE(destination.all_of<relationship>(l3));
    ASSERT_EQ(destination.get<relationship>(l3).parent, l2);
    ASSERT_TRUE(destination.all_of<entt::tag<"empty"_hs>>(l3));
}
