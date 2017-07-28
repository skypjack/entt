#include <gtest/gtest.h>
#include <registry.hpp>

TEST(DefaultRegistry, Functionalities) {
    using registry_type = entt::DefaultRegistry<int, char>;

    registry_type registry;

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
    ASSERT_TRUE(registry.empty());

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    registry_type::entity_type e1 = registry.create();
    registry_type::entity_type e2 = registry.create<int, char>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e1, e2);

    ASSERT_FALSE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_FALSE(registry.has<char>(e1));
    ASSERT_TRUE(registry.has<char>(e2));
    ASSERT_TRUE((registry.has<int, char>(e2)));
    ASSERT_FALSE((registry.has<int, char>(e1)));

    ASSERT_EQ(registry.assign<int>(e1, 42), 42);
    ASSERT_EQ(registry.assign<char>(e1, 'c'), 'c');
    ASSERT_NO_THROW(registry.remove<int>(e2));
    ASSERT_NO_THROW(registry.remove<char>(e2));

    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_FALSE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE(registry.has<char>(e2));
    ASSERT_TRUE((registry.has<int, char>(e1)));
    ASSERT_FALSE((registry.has<int, char>(e2)));

    registry_type::entity_type e3 = registry.clone(e1);

    ASSERT_TRUE(registry.has<int>(e3));
    ASSERT_TRUE(registry.has<char>(e3));
    ASSERT_EQ(registry.get<int>(e1), 42);
    ASSERT_EQ(registry.get<char>(e1), 'c');
    ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e3));
    ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e3));
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e3));
    ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e3));

    ASSERT_NO_THROW(registry.copy(e2, e1));
    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e2));
    ASSERT_EQ(registry.get<int>(e1), 42);
    ASSERT_EQ(registry.get<char>(e1), 'c');
    ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e2));

    ASSERT_NO_THROW(registry.replace<int>(e1, 0));
    ASSERT_EQ(registry.get<int>(e1), 0);
    ASSERT_NO_THROW(registry.copy<int>(e2, e1));
    ASSERT_EQ(registry.get<int>(e2), 0);
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e2));

    ASSERT_NO_THROW(registry.remove<int>(e2));
    ASSERT_NO_THROW(registry.accomodate<int>(e1, 1));
    ASSERT_NO_THROW(registry.accomodate<int>(e2, 1));
    ASSERT_EQ(static_cast<const registry_type &>(registry).get<int>(e1), 1);
    ASSERT_EQ(static_cast<const registry_type &>(registry).get<int>(e2), 1);

    ASSERT_EQ(registry.size(), registry_type::size_type{3});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.destroy(e3));

    ASSERT_TRUE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    ASSERT_EQ(registry.size(), registry_type::size_type{2});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
    ASSERT_TRUE(registry.empty());

    registry.create<int, char>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset<int>());

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    e1 = registry.create<int>();
    e2 = registry.create();

    ASSERT_NO_THROW(registry.reset<int>(e1));
    ASSERT_NO_THROW(registry.reset<int>(e2));
    ASSERT_TRUE(registry.empty<int>());
}

TEST(DefaultRegistry, Copy) {
    using registry_type = entt::DefaultRegistry<int, char, double>;

    registry_type registry;

    registry_type::entity_type e1 = registry.create<int, char>();
    registry_type::entity_type e2 = registry.create<int, double>();

    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE(registry.has<double>(e1));

    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_FALSE(registry.has<char>(e2));
    ASSERT_TRUE(registry.has<double>(e2));

    ASSERT_NO_THROW(registry.copy(e2, e1));

    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE(registry.has<double>(e1));

    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e2));
    ASSERT_FALSE(registry.has<double>(e2));

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_TRUE(registry.empty<double>());

    registry.reset();
}

TEST(DefaultRegistry, ViewSingleComponent) {
    using registry_type = entt::DefaultRegistry<int, char>;

    registry_type registry;

    registry_type::entity_type e1 = registry.create();
    registry_type::entity_type e2 = registry.create<int, char>();

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());

    auto view = registry.view<char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{1});

    registry.assign<char>(e1);

    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{2});

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NO_THROW(registry.reset());
}

TEST(DefaultRegistry, ViewMultipleComponent) {
    using registry_type = entt::DefaultRegistry<int, char>;

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

TEST(DefaultRegistry, EmptyViewSingleComponent) {
    using registry_type = entt::DefaultRegistry<char, int, double>;

    registry_type registry;

    registry.create<char, double>();
    registry.create<char>();

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), registry_type::size_type{0});

    registry.reset();
}

TEST(DefaultRegistry, EmptyViewMultipleComponent) {
    using registry_type = entt::DefaultRegistry<char, int, float, double>;

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
