#include <algorithm>
#include <iterator>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

struct stable_type {
    int value;
};

template<>
struct entt::component_traits<stable_type>: basic_component_traits {
    static constexpr auto in_place_delete = true;
};

TEST(RuntimeView, Functionalities) {
    entt::registry registry;

    // forces the creation of the pools
    static_cast<void>(registry.storage<int>());
    static_cast<void>(registry.storage<char>());

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_EQ(view.size_hint(), 0u);

    const auto e0 = registry.create();
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    ASSERT_NE(view.size_hint(), 0u);

    registry.emplace<char>(e1);

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

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));
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

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(RuntimeView, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value(), entt::type_hash<float>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), e1)), view.end());
}

TEST(RuntimeView, Each) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));
    std::size_t cnt = 0;

    view.each([&cnt](auto) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});
}

TEST(RuntimeView, EachWithHoles) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<char>(e0, '0');
    registry.emplace<char>(e1, '1');

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e2, 2);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, MissingPool) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    entt::id_type types[] = {entt::type_hash<int>::value(), entt::type_hash<char>::value()};
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_EQ(view.size_hint(), 0u);

    registry.emplace<char>(e0);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
}

TEST(RuntimeView, EmptyRange) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const entt::id_type *ptr = nullptr;
    auto view = registry.runtime_view(ptr, ptr);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
}

TEST(RuntimeView, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    entt::id_type components[] = {entt::type_hash<int>::value()};
    entt::id_type filter[] = {entt::type_hash<char>::value(), entt::type_hash<double>::value()};
    auto view = registry.runtime_view(std::begin(components), std::end(components), std::begin(filter), std::end(filter));

    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, StableType) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<int>(e0);
    registry.emplace<int>(e1);
    registry.emplace<int>(e2);

    registry.emplace<stable_type>(e0);
    registry.emplace<stable_type>(e1);

    registry.remove<stable_type>(e1);

    entt::id_type components[] = {entt::type_hash<int>::value(), entt::type_hash<stable_type>::value()};
    auto view = registry.runtime_view(std::begin(components), std::end(components));

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

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<stable_type>(entity, 0);
    registry.emplace<stable_type>(other, 42);
    registry.emplace<int>(entity);

    entt::id_type components[] = {entt::type_hash<stable_type>::value()};
    entt::id_type filter[] = {entt::type_hash<int>::value()};
    auto view = registry.runtime_view(std::begin(components), std::end(components), std::begin(filter), std::end(filter));

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
