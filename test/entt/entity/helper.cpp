#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/group.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/entity/view.hpp>
#include <entt/signal/sigh.hpp>
#include "../../common/pointer_stable.h"

struct clazz {
    void func(entt::registry &, entt::entity curr) {
        entt = curr;
    }

    entt::entity entt{entt::null};
};

void sigh_callback(int &value) {
    ++value;
}

template<typename Type>
struct ToEntity: testing::Test {
    using type = Type;
};

template<typename Type>
using ToEntityDeprecated = ToEntity<Type>;

using ToEntityTypes = ::testing::Types<int, test::pointer_stable>;

TYPED_TEST_SUITE(ToEntity, ToEntityTypes, );
TYPED_TEST_SUITE(ToEntityDeprecated, ToEntityTypes, );

TEST(AsView, Functionalities) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::view<entt::get_t<int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<char, double>, entt::exclude_t<int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<const char, double>, entt::exclude_t<const int>>) {})(entt::as_view{registry});
    ([](entt::view<entt::get_t<const char, const double>, entt::exclude_t<const int>>) {})(entt::as_view{cregistry});
}

TEST(AsGroup, Functionalities) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::owned_t<double>, entt::get_t<char>, entt::exclude_t<int>>) {})(entt::as_group{registry});
    ([](entt::group<entt::owned_t<double>, entt::get_t<const char>, entt::exclude_t<const int>>) {})(entt::as_group{registry});
    ([](entt::group<entt::owned_t<const double>, entt::get_t<const char>, entt::exclude_t<const int>>) {})(entt::as_group{cregistry});
}

TEST(Invoke, Functionalities) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.on_construct<clazz>().connect<entt::invoke<&clazz::func>>();
    registry.emplace<clazz>(entity);

    ASSERT_EQ(entity, registry.get<clazz>(entity).entt);
}

TYPED_TEST(ToEntity, Functionalities) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::registry registry;
    const entt::entity null = entt::null;

    auto &storage = registry.storage<value_type>();
    const value_type value{4};

    ASSERT_EQ(entt::to_entity(storage, value), null);

    const auto entity = registry.create();
    storage.emplace(entity);

    while(storage.size() < (traits_type::page_size - (1u + traits_type::in_place_delete))) {
        storage.emplace(registry.create(), value);
    }

    const auto other = registry.create();
    const auto next = registry.create();

    storage.emplace(other);
    storage.emplace(next);

    ASSERT_EQ(entt::to_entity(storage, storage.get(entity)), entity);
    ASSERT_EQ(entt::to_entity(storage, storage.get(other)), other);
    ASSERT_EQ(entt::to_entity(storage, storage.get(next)), next);

    ASSERT_EQ(*storage.entt::sparse_set::rbegin(), entity);
    ASSERT_EQ(&*(storage.rbegin() + traits_type::page_size - (1u + traits_type::in_place_delete)), &storage.get(other));

    // erase in the middle
    storage.erase(other);

    ASSERT_EQ(entt::to_entity(storage, storage.get(entity)), entity);
    ASSERT_EQ(entt::to_entity(storage, storage.get(next)), next);

    ASSERT_EQ(*storage.entt::sparse_set::rbegin(), entity);
    ASSERT_EQ(&*(storage.rbegin() + traits_type::page_size - 1u), &storage.get(next));

    ASSERT_EQ(entt::to_entity(storage, value), null);

    storage.clear();

    storage.emplace(entity);
    storage.emplace(other);
    storage.emplace(next);

    // erase first
    storage.erase(entity);

    ASSERT_EQ(entt::to_entity(storage, value), null);
    ASSERT_EQ(entt::to_entity(storage, storage.get(other)), other);
}

TEST(SighHelper, Functionalities) {
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
