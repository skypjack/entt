#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

TEST(DefaultRegistry, Functionalities) {
    entt::DefaultRegistry registry;

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.capacity<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.capacity<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    auto e1 = registry.create();
    auto e2 = registry.create<int, char>();

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_GE(registry.capacity<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_GE(registry.capacity<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e1, e2);

    ASSERT_FALSE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_FALSE(registry.has<char>(e1));
    ASSERT_TRUE(registry.has<char>(e2));
    ASSERT_FALSE((registry.has<int, char>(e1)));
    ASSERT_TRUE((registry.has<int, char>(e2)));

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

    auto e3 = registry.create();

    registry.accomodate<int>(e3, registry.get<int>(e1));
    registry.accomodate<char>(e3, registry.get<char>(e1));

    ASSERT_TRUE(registry.has<int>(e3));
    ASSERT_TRUE(registry.has<char>(e3));
    ASSERT_EQ(registry.get<int>(e1), 42);
    ASSERT_EQ(registry.get<char>(e1), 'c');
    ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e3));
    ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e3));
    ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e3));
    ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e3));

    ASSERT_NO_THROW(registry.replace<int>(e1, 0));
    ASSERT_EQ(registry.get<int>(e1), 0);

    ASSERT_NO_THROW(registry.accomodate<int>(e1, 1));
    ASSERT_NO_THROW(registry.accomodate<int>(e2, 1));
    ASSERT_EQ(static_cast<const entt::DefaultRegistry &>(registry).get<int>(e1), 1);
    ASSERT_EQ(static_cast<const entt::DefaultRegistry &>(registry).get<int>(e2), 1);

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{3});
    ASSERT_GE(registry.capacity(), entt::DefaultRegistry::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e3), entt::DefaultRegistry::version_type{0});
    ASSERT_EQ(registry.current(e3), entt::DefaultRegistry::version_type{0});
    ASSERT_NO_THROW(registry.destroy(e3));
    ASSERT_EQ(registry.version(e3), entt::DefaultRegistry::version_type{0});
    ASSERT_EQ(registry.current(e3), entt::DefaultRegistry::version_type{1});

    ASSERT_TRUE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{2});
    ASSERT_GE(registry.capacity(), entt::DefaultRegistry::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity(), entt::DefaultRegistry::size_type{3});
    ASSERT_TRUE(registry.empty());

    registry.create<int, char>();

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_GE(registry.capacity<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_GE(registry.capacity<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset<int>());

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_GE(registry.capacity<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    e1 = registry.create<int>();
    e2 = registry.create();

    ASSERT_NO_THROW(registry.reset<int>(e1));
    ASSERT_NO_THROW(registry.reset<int>(e2));

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_GE(registry.capacity<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
}

TEST(DefaultRegistry, SortSingle) {
    entt::DefaultRegistry registry;

    auto e1 = registry.create();
    auto e2 = registry.create();
    auto e3 = registry.create();

    auto val = 0;

    registry.assign<int>(e1, val++);
    registry.assign<int>(e2, val++);
    registry.assign<int>(e3, val++);

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --val);
    }

    registry.sort<int>(std::less<int>{});

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), val++);
    }
}

TEST(DefaultRegistry, SortMulti) {
    entt::DefaultRegistry registry;

    auto e1 = registry.create();
    auto e2 = registry.create();
    auto e3 = registry.create();

    auto uval = 0u;
    auto ival = 0;

    registry.assign<unsigned int>(e1, uval++);
    registry.assign<unsigned int>(e2, uval++);
    registry.assign<unsigned int>(e3, uval++);

    registry.assign<int>(e1, ival++);
    registry.assign<int>(e2, ival++);
    registry.assign<int>(e3, ival++);

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), --uval);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less<unsigned int>{});
    registry.sort<int, unsigned int>();

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), uval++);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), ival++);
    }
}
