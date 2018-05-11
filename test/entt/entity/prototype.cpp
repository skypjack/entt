#include <gtest/gtest.h>
#include <entt/entity/prototype.hpp>
#include <entt/entity/registry.hpp>

TEST(Prototype, Functionalities) {
    entt::DefaultRegistry registry;
    entt::DefaultPrototype prototype;
    const auto &cprototype = prototype;

    ASSERT_FALSE((prototype.has<int, char>()));
    ASSERT_TRUE(registry.empty());

    prototype.set<int>(3);
    prototype.set<char>('c');

    ASSERT_EQ(prototype.get<int>(), 3);
    ASSERT_EQ(cprototype.get<char>(), 'c');
    ASSERT_EQ(std::get<0>(prototype.get<int, char>()), 3);
    ASSERT_EQ(std::get<1>(cprototype.get<int, char>()), 'c');

    const auto e0 = prototype(registry);

    ASSERT_TRUE((prototype.has<int, char>()));
    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.empty());

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
