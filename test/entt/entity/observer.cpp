#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/observer.hpp>
#include <entt/entity/registry.hpp>

TEST(Observer, Functionalities) {
    entt::registry registry;
    entt::observer observer{registry, entt::collector.group<int>()};

    ASSERT_EQ(observer.size(), 0u);
    ASSERT_TRUE(observer.empty());
    ASSERT_EQ(observer.begin(), observer.end());

    const auto entity = registry.create();
    registry.emplace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);
    ASSERT_NE(observer.begin(), observer.end());
    ASSERT_EQ(++observer.begin(), observer.end());
    ASSERT_EQ(*observer.begin(), entity);

    observer.clear();

    ASSERT_EQ(observer.size(), 0u);
    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.erase<int>(entity);
    registry.emplace<int>(entity);

    ASSERT_EQ(observer.size(), 0u);
    ASSERT_TRUE(observer.empty());
}

TEST(Observer, AllOf) {
    constexpr auto collector =
        entt::collector
            .group<int, char>(entt::exclude<float>)
            .group<int, double>();

    entt::registry registry;
    entt::observer observer{registry, collector};
    const auto entity = registry.create();

    ASSERT_TRUE(observer.empty());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    registry.emplace<double>(entity);

    ASSERT_FALSE(observer.empty());

    registry.erase<int>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace<float>(entity);
    registry.emplace<int>(entity);

    ASSERT_FALSE(observer.empty());

    registry.erase<double>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace<double>(entity);
    observer.clear();

    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.emplace_or_replace<int>(entity);
    registry.emplace_or_replace<char>(entity);
    registry.erase<float>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, AllOfFiltered) {
    constexpr auto collector =
        entt::collector
            .group<int>()
            .where<char>(entt::exclude<double>);

    entt::registry registry;
    entt::observer observer{registry, collector};
    const auto entity = registry.create();

    ASSERT_TRUE(observer.empty());

    registry.emplace<int>(entity);

    ASSERT_EQ(observer.size(), 0u);
    ASSERT_TRUE(observer.empty());

    registry.erase<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<double>(entity);
    registry.emplace<int>(entity);

    ASSERT_TRUE(observer.empty());

    registry.erase<int>(entity);
    registry.erase<double>(entity);
    registry.emplace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    registry.emplace<double>(entity);

    ASSERT_TRUE(observer.empty());

    registry.erase<double>(entity);

    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.erase<int>(entity);
    registry.emplace<int>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, Observe) {
    entt::registry registry;
    entt::observer observer{registry, entt::collector.update<int>().update<char>()};
    const auto entity = registry.create();

    ASSERT_TRUE(observer.empty());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    observer.clear();
    registry.replace<char>(entity);

    ASSERT_FALSE(observer.empty());

    observer.clear();

    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.emplace_or_replace<int>(entity);
    registry.emplace_or_replace<char>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, ObserveFiltered) {
    constexpr auto collector =
        entt::collector
            .update<int>()
            .where<char>(entt::exclude<double>);

    entt::registry registry;
    entt::observer observer{registry, collector};
    const auto entity = registry.create();

    ASSERT_TRUE(observer.empty());

    registry.emplace<int>(entity);
    registry.replace<int>(entity);

    ASSERT_EQ(observer.size(), 0u);
    ASSERT_TRUE(observer.empty());

    registry.emplace<char>(entity);
    registry.emplace<double>(entity);
    registry.replace<int>(entity);

    ASSERT_TRUE(observer.empty());

    registry.erase<double>(entity);
    registry.replace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    registry.emplace<double>(entity);

    ASSERT_TRUE(observer.empty());

    registry.erase<double>(entity);

    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.replace<int>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, AllOfObserve) {
    entt::registry registry;
    entt::observer observer{};
    const auto entity = registry.create();

    observer.connect(registry, entt::collector.group<int>().update<char>());

    ASSERT_TRUE(observer.empty());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.replace<char>(entity);
    registry.erase<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    registry.erase<char>(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(observer.empty());

    registry.replace<char>(entity);
    observer.clear();

    ASSERT_TRUE(observer.empty());

    observer.disconnect();
    registry.emplace_or_replace<int>(entity);
    registry.emplace_or_replace<char>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, CrossRulesCornerCase) {
    entt::registry registry;
    entt::observer observer{registry, entt::collector.group<int>().group<char>()};
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    observer.clear();

    ASSERT_TRUE(observer.empty());

    registry.emplace<char>(entity);
    registry.erase<int>(entity);

    ASSERT_FALSE(observer.empty());
}

TEST(Observer, Each) {
    entt::registry registry;
    entt::observer observer{registry, entt::collector.group<int>()};
    const auto entity = registry.create();
    registry.emplace<int>(entity);

    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(observer.size(), 1u);

    std::as_const(observer).each([entity](const auto entt) {
        ASSERT_EQ(entity, entt);
    });

    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(observer.size(), 1u);

    observer.each([entity](const auto entt) {
        ASSERT_EQ(entity, entt);
    });

    ASSERT_TRUE(observer.empty());
    ASSERT_EQ(observer.size(), 0u);
}

TEST(Observer, MultipleFilters) {
    constexpr auto collector =
        entt::collector
            .update<int>()
            .where<char>()
            .update<double>()
            .where<float>();

    entt::registry registry;
    entt::observer observer{registry, collector};
    const auto entity = registry.create();

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    observer.clear();
    registry.emplace<double>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<double>(entity);
    registry.emplace<float>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<double>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    registry.erase<float>(entity);

    ASSERT_TRUE(observer.empty());

    registry.emplace_or_replace<int>(entity);

    ASSERT_EQ(observer.size(), 1u);
    ASSERT_FALSE(observer.empty());
    ASSERT_EQ(*observer.data(), entity);

    observer.clear();
    observer.disconnect();

    registry.emplace_or_replace<int>(entity);

    ASSERT_TRUE(observer.empty());
}

TEST(Observer, GroupCornerCase) {
    constexpr auto add_collector = entt::collector.group<int>(entt::exclude<char>);
    constexpr auto remove_collector = entt::collector.group<int, char>();

    entt::registry registry;
    entt::observer add_observer{registry, add_collector};
    entt::observer remove_observer{registry, remove_collector};

    const auto entity = registry.create();
    registry.emplace<int>(entity);

    ASSERT_FALSE(add_observer.empty());
    ASSERT_TRUE(remove_observer.empty());

    add_observer.clear();
    registry.emplace<char>(entity);

    ASSERT_TRUE(add_observer.empty());
    ASSERT_FALSE(remove_observer.empty());

    remove_observer.clear();
    registry.erase<char>(entity);

    ASSERT_FALSE(add_observer.empty());
    ASSERT_TRUE(remove_observer.empty());
}
