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
