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
    ([](entt::view<entt::exclude_t<int>, const char, const double>) {})(entt::as_view{cregistry});
}

TEST(Helper, AsGroup) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::exclude_t<int>, entt::get_t<char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, const double>) {})(entt::as_group{cregistry});
}

TEST(Helper, Invoke) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.on_construct<clazz>().connect<entt::invoke<&clazz::func>>();
    registry.emplace<clazz>(entity);

    ASSERT_EQ(entity, registry.get<clazz>(entity).entt);
}

TEST(Helper, ToEntity) {
    entt::registry registry;
    const entt::entity null = entt::null;
    const int value = 42;

    ASSERT_EQ(entt::to_entity(registry, 42), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);

    const auto entity = registry.create();
    registry.emplace<int>(entity);

    while(registry.size<int>() < (ENTT_PACKED_PAGE - 1u)) {
        registry.emplace<int>(registry.create(), value);
    }

    const auto other = registry.create();
    const auto next = registry.create();

    registry.emplace<int>(other);
    registry.emplace<int>(next);

    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(other)), other);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(next)), next);

    ASSERT_EQ(&registry.get<int>(entity) + ENTT_PACKED_PAGE - 1u, &registry.get<int>(other));
    ASSERT_NE(&registry.get<int>(entity) + ENTT_PACKED_PAGE, &registry.get<int>(next));

    registry.destroy(other);

    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(next)), next);

    ASSERT_EQ(&registry.get<int>(entity) + ENTT_PACKED_PAGE - 1u, &registry.get<int>(next));

    ASSERT_EQ(entt::to_entity(registry, 42), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);
}
