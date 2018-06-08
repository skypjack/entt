#include <gtest/gtest.h>
#include <entt/entity/prototype.hpp>
#include <entt/entity/registry.hpp>

TEST(Prototype, SameRegistry) {
    entt::DefaultRegistry registry;
    entt::DefaultPrototype prototype{registry};
    const auto &cprototype = prototype;

    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE((prototype.has<int, char>()));

    ASSERT_EQ(prototype.set<int>(2), 2);
    ASSERT_EQ(prototype.set<int>(3), 3);
    ASSERT_EQ(prototype.set<char>('c'), 'c');

    ASSERT_EQ(prototype.get<int>(), 3);
    ASSERT_EQ(cprototype.get<char>(), 'c');
    ASSERT_EQ(std::get<0>(prototype.get<int, char>()), 3);
    ASSERT_EQ(std::get<1>(cprototype.get<int, char>()), 'c');

    const auto e0 = prototype.create();

    ASSERT_TRUE((prototype.has<int, char>()));
    ASSERT_FALSE(registry.orphan(e0));

    const auto e1 = prototype();
    prototype(e0);

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e1));

    ASSERT_TRUE((registry.has<int, char>(e0)));
    ASSERT_TRUE((registry.has<int, char>(e1)));

    registry.remove<int>(e0);
    registry.remove<int>(e1);
    prototype.unset<int>();

    ASSERT_FALSE((prototype.has<int, char>()));
    ASSERT_FALSE((prototype.has<int>()));
    ASSERT_TRUE((prototype.has<char>()));

    prototype(e0);
    prototype(e1);

    ASSERT_FALSE(registry.has<int>(e0));
    ASSERT_FALSE(registry.has<int>(e1));

    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<char>(e1), 'c');

    registry.get<char>(e0) = '*';
    prototype.assign(e0);

    ASSERT_EQ(registry.get<char>(e0), '*');

    registry.get<char>(e1) = '*';
    prototype.accommodate(e1);

    ASSERT_EQ(registry.get<char>(e1), 'c');
}

TEST(Prototype, OtherRegistry) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry repository;
    entt::DefaultPrototype prototype{repository};
    const auto &cprototype = prototype;

    ASSERT_TRUE(registry.empty());
    ASSERT_FALSE((prototype.has<int, char>()));

    ASSERT_EQ(prototype.set<int>(2), 2);
    ASSERT_EQ(prototype.set<int>(3), 3);
    ASSERT_EQ(prototype.set<char>('c'), 'c');

    ASSERT_EQ(prototype.get<int>(), 3);
    ASSERT_EQ(cprototype.get<char>(), 'c');
    ASSERT_EQ(std::get<0>(prototype.get<int, char>()), 3);
    ASSERT_EQ(std::get<1>(cprototype.get<int, char>()), 'c');

    const auto e0 = prototype.create(registry);

    ASSERT_TRUE((prototype.has<int, char>()));
    ASSERT_FALSE(registry.orphan(e0));

    const auto e1 = prototype(registry);
    prototype(registry, e0);

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e1));

    ASSERT_TRUE((registry.has<int, char>(e0)));
    ASSERT_TRUE((registry.has<int, char>(e1)));

    registry.remove<int>(e0);
    registry.remove<int>(e1);
    prototype.unset<int>();

    ASSERT_FALSE((prototype.has<int, char>()));
    ASSERT_FALSE((prototype.has<int>()));
    ASSERT_TRUE((prototype.has<char>()));

    prototype(registry, e0);
    prototype(registry, e1);

    ASSERT_FALSE(registry.has<int>(e0));
    ASSERT_FALSE(registry.has<int>(e1));

    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<char>(e1), 'c');

    registry.get<char>(e0) = '*';
    prototype.assign(registry, e0);

    ASSERT_EQ(registry.get<char>(e0), '*');

    registry.get<char>(e1) = '*';
    prototype.accommodate(registry, e1);

    ASSERT_EQ(registry.get<char>(e1), 'c');
}

TEST(Prototype, RAII) {
    entt::DefaultRegistry registry;

    {
        entt::DefaultPrototype prototype{registry};
        prototype.set<int>(0);

        ASSERT_FALSE(registry.empty());
    }

    ASSERT_TRUE(registry.empty());
}

TEST(Prototype, MoveConstructionAssignment) {
    entt::DefaultRegistry registry;

    entt::DefaultPrototype prototype{registry};
    prototype.set<int>(0);
    auto other{std::move(prototype)};
    const auto e0 = other();

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{2});
    ASSERT_TRUE(registry.has<int>(e0));

    prototype = std::move(other);
    const auto e1 = prototype();

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{3});
    ASSERT_TRUE(registry.has<int>(e1));
}
