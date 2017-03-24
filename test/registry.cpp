#include <gtest/gtest.h>
#include <registry.hpp>

TEST(DefaultRegistry, Functionalities) {
    using registry_type = entt::DefaultRegistry<int, char>;

    registry_type registry;

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
    ASSERT_TRUE(registry.empty());

    registry_type::entity_type e1 = registry.create();
    registry_type::entity_type e2 = registry.create<int, char>();

    ASSERT_NE(e1, e2);

    ASSERT_FALSE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_FALSE(registry.has<char>(e1));
    ASSERT_TRUE(registry.has<char>(e2));

    ASSERT_EQ(registry.assign<int>(e1, 42), 42);
    ASSERT_EQ(registry.assign<char>(e1, 'c'), 'c');
    ASSERT_NO_THROW(registry.remove<int>(e2));
    ASSERT_NO_THROW(registry.remove<char>(e2));

    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_FALSE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE(registry.has<char>(e2));

    registry_type::entity_type e3 = registry.clone(e1);

    ASSERT_TRUE(registry.has<int>(e3));
    ASSERT_TRUE(registry.has<char>(e3));
    ASSERT_EQ(registry.get<int>(e1), 42);
    ASSERT_EQ(registry.get<char>(e1), 'c');
    ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e3));
    ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e3));
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e3));
    ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e3));

    ASSERT_NO_THROW(registry.copy(e1, e2));
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
    ASSERT_NO_THROW(registry.copy<int>(e1, e2));
    ASSERT_EQ(registry.get<int>(e2), 0);
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e2));

    ASSERT_EQ(registry.size(), registry_type::size_type{3});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.destroy(e3));

    ASSERT_EQ(registry.size(), registry_type::size_type{2});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
    ASSERT_TRUE(registry.empty());
}

TEST(DefaultRegistry, ViewSingleComponent) {
    using registry_type = entt::DefaultRegistry<int, char>;

    registry_type registry;

    registry_type::entity_type e1 = registry.create();
    registry_type::entity_type e2 = registry.create<int, char>();

    auto view = registry.view<char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{1});

    registry.assign<char>(e1);

    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{2});

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NO_THROW(registry.reset());

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());
}

TEST(DefaultRegistry, ViewMultipleComponent) {
    using registry_type = entt::DefaultRegistry<int, char>;

    registry_type registry;

    registry_type::entity_type e1 = registry.create<char>();
    registry_type::entity_type e2 = registry.create<int, char>();

    auto view = registry.view<int, char>();

    ASSERT_NE(view.begin(), view.end());

    registry.remove<char>(e1);
    registry.remove<char>(e2);
    view.reset();

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NO_THROW(registry.reset());

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));
}
