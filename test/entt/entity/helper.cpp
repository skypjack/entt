#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>

struct clazz {
    void func(entt::registry &, entt::entity curr) {
        entt = curr;
    }

    entt::entity entt{entt::null};
};

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value;
};

void sigh_callback(int &value) {
    ++value;
}

TEST(Helper, AsView) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::view<entt::get_t<int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<char, double>, entt::exclude_t<int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<const char, double>, entt::exclude_t<const int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<const char, const double>, entt::exclude_t<const int>>) {})(entt::as_view{cregistry});
}

TEST(Helper, AsGroup) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::owned_t<double>, entt::get_t<char>, entt::exclude_t<int>>) {})(entt::as_group{registry});
    ([](entt::group<entt::owned_t<double>, entt::get_t<const char>, entt::exclude_t<const int>>) {})(entt::as_group{registry});
    ([](entt::group<entt::owned_t<const double>, entt::get_t<const char>, entt::exclude_t<const int>>) {})(entt::as_group{cregistry});
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
    constexpr auto page_size = entt::storage_type_t<int>::traits_type::page_size;
    const int value = 42;

    ASSERT_EQ(entt::to_entity(registry, 42), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);

    const auto entity = registry.create();
    auto &&storage = registry.storage<int>();
    storage.emplace(entity);

    while(storage.size() < (page_size - 1u)) {
        storage.emplace(registry.create(), value);
    }

    const auto other = registry.create();
    const auto next = registry.create();

    registry.emplace<int>(other);
    registry.emplace<int>(next);

    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(other)), other);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(next)), next);

    ASSERT_EQ(&registry.get<int>(entity) + page_size - 1u, &registry.get<int>(other));

    registry.destroy(other);

    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<int>(next)), next);

    ASSERT_EQ(&registry.get<int>(entity) + page_size - 1u, &registry.get<int>(next));

    ASSERT_EQ(entt::to_entity(registry, 42), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);
}

TEST(Helper, ToEntityStableType) {
    entt::registry registry;
    const entt::entity null = entt::null;
    constexpr auto page_size = entt::storage_type_t<stable_type>::traits_type::page_size;
    const stable_type value{42};

    ASSERT_EQ(entt::to_entity(registry, stable_type{42}), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);

    const auto entity = registry.create();
    auto &&storage = registry.storage<stable_type>();
    registry.emplace<stable_type>(entity);

    while(storage.size() < (page_size - 2u)) {
        storage.emplace(registry.create(), value);
    }

    const auto other = registry.create();
    const auto next = registry.create();

    registry.emplace<stable_type>(other);
    registry.emplace<stable_type>(next);

    ASSERT_EQ(entt::to_entity(registry, registry.get<stable_type>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<stable_type>(other)), other);
    ASSERT_EQ(entt::to_entity(registry, registry.get<stable_type>(next)), next);

    ASSERT_EQ(&registry.get<stable_type>(entity) + page_size - 2u, &registry.get<stable_type>(other));

    registry.destroy(other);

    ASSERT_EQ(entt::to_entity(registry, registry.get<stable_type>(entity)), entity);
    ASSERT_EQ(entt::to_entity(registry, registry.get<stable_type>(next)), next);

    ASSERT_EQ(&registry.get<stable_type>(entity) + page_size - 1u, &registry.get<stable_type>(next));

    ASSERT_EQ(entt::to_entity(registry, stable_type{42}), null);
    ASSERT_EQ(entt::to_entity(registry, value), null);
}

TEST(Helper, SighHelper) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entt = registry.create();
    entt::sigh_helper helper{registry};
    int counter{};

    ASSERT_EQ(&helper.registry(), &registry);

    helper.with<int>()
        .on_construct<&sigh_callback>(counter)
        .on_update<&sigh_callback>(counter)
        .on_destroy<&sigh_callback>(counter);

    ASSERT_EQ(counter, 0);

    registry.emplace<int>(entt);
    registry.replace<int>(entt);
    registry.erase<int>(entt);

    ASSERT_EQ(counter, 3);

    helper.with<double>("other"_hs)
        .on_construct<&sigh_callback>(counter)
        .on_update<&sigh_callback>(counter)
        .on_destroy<&sigh_callback>(counter);

    registry.emplace<double>(entt);
    registry.replace<double>(entt);
    registry.erase<double>(entt);

    ASSERT_EQ(counter, 3);

    registry.storage<double>("other"_hs).emplace(entt);
    registry.storage<double>("other"_hs).patch(entt);
    registry.storage<double>("other"_hs).erase(entt);

    ASSERT_EQ(counter, 6);
}
