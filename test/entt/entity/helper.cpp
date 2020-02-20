#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/type_traits.hpp>

struct clazz {
    void func(entt::registry &, entt::entity curr) { entt = curr; }
    entt::entity entt{entt::null};
};

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

TEST(Invoke, MemberFunction) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.on_construct<clazz>().connect<entt::invoke<&clazz::func>>();
    registry.assign<clazz>(entity);

    ASSERT_EQ(entity, registry.get<clazz>(entity).entt);
}
