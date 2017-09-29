#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

TEST(DefaultRegistry, DynamicViewSingleComponent) {
    using registry_type = entt::DefaultRegistry;

    registry_type registry;

    registry_type::entity_type e1 = registry.create();
    registry_type::entity_type e2 = registry.create<int, char>();

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());

    auto view = registry.view<char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<char>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NO_THROW(registry.reset());
}

TEST(DefaultRegistry, DynamicViewMultipleComponent) {
    using registry_type = entt::DefaultRegistry;

    registry_type registry;

    registry_type::entity_type e1 = registry.create<char>();
    registry_type::entity_type e2 = registry.create<int, char>();

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));

    auto view = registry.view<int, char>();

    ASSERT_NE(view.begin(), view.end());

    registry.remove<char>(e1);
    registry.remove<char>(e2);
    view.reset();

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NO_THROW(registry.reset());
}

TEST(DefaultRegistry, DynamicViewSingleComponentEmpty) {
    using registry_type = entt::DefaultRegistry;

    registry_type registry;

    registry.create<char, double>();
    registry.create<char>();

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), registry_type::size_type{0});

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }

    registry.reset();
}

TEST(DefaultRegistry, DynamicViewMultipleComponentEmpty) {
    using registry_type = entt::DefaultRegistry;

    registry_type registry;

    registry.create<double, int, float>();
    registry.create<char, float>();

    auto view = registry.view<char, int, float>();

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }

    registry.reset();
}
