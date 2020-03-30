#include <iterator>
#include <algorithm>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

TEST(RuntimeView, Functionalities) {
    entt::registry registry;

    // forces the creation of the pools
    registry.reserve<int>(0);
    registry.reserve<char>(0);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_FALSE(view.empty());

    registry.assign<char>(e1);

    auto it = view.begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (view.end()));

    ASSERT_NO_THROW((view.begin()++));
    ASSERT_NO_THROW((++view.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), decltype(view.size()){1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(registry.get<int>(entity), 42);
        ASSERT_EQ(registry.get<char>(entity), '2');
    }
}

TEST(RuntimeView, Iterator) {
    entt::registry registry;

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
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
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(RuntimeView, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id(), entt::type_info<float>::id() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), e1)), view.end());
}

TEST(RuntimeView, Each) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
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

    registry.assign<char>(e0, '0');
    registry.assign<char>(e1, '1');

    registry.assign<int>(e0, 0);
    registry.assign<int>(e2, 2);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, MissingPool) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    ENTT_ID_TYPE types[] = { entt::type_info<int>::id(), entt::type_info<char>::id() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});

    registry.assign<char>(e0);

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
}

TEST(RuntimeView, EmptyRange) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const ENTT_ID_TYPE *ptr = nullptr;
    auto view = registry.runtime_view(ptr, ptr);

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    ASSERT_EQ((std::find(view.begin(), view.end(), e0)), view.end());
}
