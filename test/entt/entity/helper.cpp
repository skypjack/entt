#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/type_traits.hpp>

TEST(Helper, AsView) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::view<entt::exclude_t<>, int>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, char, double>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, const char, double>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, const char, const double>) {})(entt::as_view{registry});
}

TEST(Helper, AsGroup) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::exclude_t<int>, entt::get_t<char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, const double>) {})(entt::as_group{registry});
}
