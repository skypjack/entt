#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/space.hpp>

TEST(Space, SpaceContainsView) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    auto e0 = space.create();
    auto e1 = registry.create();

    space.assign(e1);
    registry.assign<int>(e1);

    ASSERT_TRUE(space.contains(e0));
    ASSERT_TRUE(space.contains(e1));

    space.view<int>([e0, e1](auto entity, auto&&...) {
        ASSERT_NE(entity, e0);
        ASSERT_EQ(entity, e1);
    });

    space.view<double>([e0, e1](auto, auto&&...) {
        FAIL();
    });

    auto count = 0;

    for(auto entity: space) {
        (void)entity;
        ++count;
    }

    ASSERT_EQ(count, 2);

    const auto view = registry.view<int>();

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});
    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{2});
    ASSERT_FALSE(space.empty());

    registry.reset();
    space.reset();

    ASSERT_TRUE(space.empty());

    for(auto i = 0; i < 5; ++i) {
        registry.destroy(space.create());
        registry.create<int>();
    }

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{5});
    ASSERT_FALSE(space.empty());

    space.view<int>([](auto, auto&&...) {
        FAIL();
    });

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{0});
    ASSERT_TRUE(space.empty());
}

TEST(Space, ViewContainsSpace) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    auto e0 = registry.create();
    auto e1 = space.create();

    registry.assign<int>(e0);
    registry.assign<int>(e1);

    ASSERT_FALSE(space.contains(e0));
    ASSERT_TRUE(space.contains(e1));

    space.view<int>([e0, e1](auto entity, auto&&...) {
        ASSERT_NE(entity, e0);
        ASSERT_EQ(entity, e1);
    });

    space.view<double>([e0, e1](auto, auto&&...) {
        FAIL();
    });

    auto count = 0;

    for(auto entity: space) {
        (void)entity;
        ++count;
    }

    ASSERT_EQ(count, 1);

    const auto view = registry.view<int>();

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});
    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{1});
    ASSERT_FALSE(space.empty());

    registry.reset();
    space.reset();

    ASSERT_TRUE(space.empty());

    for(auto i = 0; i < 5; ++i) {
        registry.destroy(space.create());
        registry.create<int>();
        registry.create<int>();
    }

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{5});
    ASSERT_FALSE(space.empty());

    space.view<int>([](auto, auto&&...) {
        FAIL();
    });

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{0});
    ASSERT_TRUE(space.empty());
}

TEST(Space, AssignRemove) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    ASSERT_TRUE(space.empty());

    space.remove(space.create());

    ASSERT_TRUE(space.empty());
}

TEST(Space, Shrink) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    for(auto i = 0; i < 5; ++i) {
        space.create();
    }

    for(auto entity: space) {
        registry.destroy(entity);
    }

    space.create();

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{5});
    ASSERT_FALSE(space.empty());

    space.shrink();

    ASSERT_EQ(space.size(), entt::DefaultSpace::size_type{1});
    ASSERT_FALSE(space.empty());
}
