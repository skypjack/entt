#include <algorithm>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value;
};

TEST(RuntimeView, Functionalities) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_FALSE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    // forces the creation of the pools
    static_cast<void>(registry.storage<int>());
    static_cast<void>(registry.storage<char>());

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    ASSERT_EQ(view.size_hint(), 0u);

    registry.emplace<char>(e0);
    registry.emplace<int>(e1);

    ASSERT_NE(view.size_hint(), 0u);

    registry.emplace<char>(e1);

    ASSERT_EQ(view.size_hint(), 1u);

    auto it = view.begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (view.end()));

    ASSERT_NO_FATAL_FAILURE((view.begin()++));
    ASSERT_NO_FATAL_FAILURE((++view.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size_hint(), 1u);

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(registry.get<int>(entity), 42);
        ASSERT_EQ(registry.get<char>(entity), '2');
    }

    entt::runtime_view empty{};

    ASSERT_EQ(empty.size_hint(), 0u);
    ASSERT_EQ(empty.begin(), empty.end());
}

TEST(RuntimeView, Iterator) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());
    using iterator = typename decltype(view)::iterator;

    iterator end{view.begin()};
    iterator begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, view.begin());
    ASSERT_EQ(begin--, view.end());

    ASSERT_EQ(++begin, view.end());
    ASSERT_EQ(--begin, view.begin());

    ASSERT_EQ(*begin, entity);
    ASSERT_EQ(*begin.operator->(), entity);
}

TEST(RuntimeView, Contains) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(RuntimeView, Empty) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    view.iterate(registry.storage<int>())
        .iterate(registry.storage<char>())
        .iterate(registry.storage<float>());

    ASSERT_FALSE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), e1)), view.end());
}

TEST(RuntimeView, Each) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    view.each([e0](const auto entt) {
        ASSERT_EQ(entt, e0);
    });
}

TEST(RuntimeView, EachWithHoles) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<char>(e0, '0');
    registry.emplace<char>(e1, '1');

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e2, 2);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, ExcludedComponents) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    view.iterate(registry.storage<int>())
        .exclude(registry.storage<char>())
        .exclude(registry.storage<double>());

    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, StableType) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<int>(e0);
    registry.emplace<int>(e1);
    registry.emplace<int>(e2);

    registry.emplace<stable_type>(e0);
    registry.emplace<stable_type>(e1);

    registry.remove<stable_type>(e1);

    view.iterate(registry.storage<int>()).iterate(registry.storage<stable_type>());

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    ASSERT_EQ(*view.begin(), e0);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([e0](const auto entt) {
        ASSERT_EQ(e0, entt);
    });

    for(auto entt: view) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        ASSERT_EQ(e0, entt);
    }

    registry.compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TEST(RuntimeView, StableTypeWithExcludedComponent) {
    entt::registry registry;
    entt::runtime_view view{};

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<stable_type>(entity, 0);
    registry.emplace<stable_type>(other, 42);
    registry.emplace<int>(entity);

    view.iterate(registry.storage<stable_type>()).exclude(registry.storage<int>());

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    registry.destroy(entity);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    for(auto entt: view) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    }

    view.each([other](const auto entt) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    });
}
