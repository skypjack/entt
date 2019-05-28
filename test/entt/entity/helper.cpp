#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/type_traits.hpp>

TEST(Helper, AsView) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::view<int, char>) {})(entt::as_view{registry});
    ([](entt::view<const int, char>) {})(entt::as_view{registry});
    ([](entt::view<const double>) {})(entt::as_view{cregistry});
}

TEST(Helper, AsGroup) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::exclude_t<int>, entt::get_t<char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<const int>, entt::get_t<char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<const int>, entt::get_t<const char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<const int>, entt::get_t<const char>, const double>) {})(entt::as_group{registry});
}

TEST(Helper, Dependency) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::connect<double, float>(registry.on_construct<int>());

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));

    registry.assign<char>(entity);

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));

    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<double>(entity));
    ASSERT_TRUE(registry.has<float>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);
    ASSERT_EQ(registry.get<float>(entity), .0f);

    registry.get<double>(entity) = .3;
    registry.get<float>(entity) = .1f;
    registry.remove<int>(entity);
    registry.assign<int>(entity);

    ASSERT_EQ(registry.get<double>(entity), .3);
    ASSERT_EQ(registry.get<float>(entity), .1f);

    registry.remove<int>(entity);
    registry.remove<float>(entity);
    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<float>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);
    ASSERT_EQ(registry.get<float>(entity), .0f);

    registry.remove<int>(entity);
    registry.remove<double>(entity);
    registry.remove<float>(entity);
    entt::disconnect<double, float>(registry.on_construct<int>());
    registry.assign<int>(entity);

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));
}

TEST(Dependency, MultipleListenersOnTheSameType) {
    entt::registry registry;
    entt::connect<double>(registry.on_construct<int>());
    entt::connect<char>(registry.on_construct<int>());

    const auto entity = registry.create();
    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<double>(entity));
    ASSERT_TRUE(registry.has<char>(entity));
}

TEST(Helper, Tag) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.assign<entt::tag<"foobar"_hs>>(entity);
    registry.assign<int>(entity, 42);
    int counter{};

    ASSERT_FALSE(registry.has<entt::tag<"barfoo"_hs>>(entity));
    ASSERT_TRUE(registry.has<entt::tag<"foobar"_hs>>(entity));

    for(auto entt: registry.view<int, entt::tag<"foobar"_hs>>()) {
        (void)entt;
        ++counter;
    }

    ASSERT_NE(counter, 0);

    for(auto entt: registry.view<entt::tag<"foobar"_hs>>()) {
        (void)entt;
        --counter;
    }

    ASSERT_EQ(counter, 0);
}
