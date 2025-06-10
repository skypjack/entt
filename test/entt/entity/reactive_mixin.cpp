#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/signal/sigh.hpp>
#include "../../common/config.h"
#include "../../common/empty.h"
#include "../../common/entity.h"
#include "../../common/linter.hpp"
#include "../../common/registry.h"
#include "../../common/throwing_allocator.hpp"

template<typename Type, std::size_t Value>
void emplace(Type &storage, const typename Type::registry_type &, const typename Type::entity_type entity) {
    if((entity == typename Type::entity_type{Value}) && !storage.contains(entity)) {
        storage.emplace(entity);
    }
}

template<typename Type>
void remove(Type &storage, const typename Type::registry_type &, const typename Type::entity_type entity) {
    storage.remove(entity);
}

template<typename Type>
struct entt::storage_type<Type, test::entity, std::allocator<Type>, std::enable_if_t<!std::is_same_v<Type, test::entity>>> {
    using type = entt::basic_sigh_mixin<entt::basic_storage<Type, test::entity>, test::custom_registry<test::entity>>;
};

template<typename Type>
struct ReactiveMixin: testing::Test {
    using type = Type;
};

template<typename Type>
using ReactiveMixinDeathTest = ReactiveMixin<Type>;

using ReactiveMixinTypes = ::testing::Types<void, bool>;

TYPED_TEST_SUITE(ReactiveMixin, ReactiveMixinTypes, );
TYPED_TEST_SUITE(ReactiveMixinDeathTest, ReactiveMixinTypes, );

TYPED_TEST(ReactiveMixin, Constructors) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::reactive_mixin<entt::storage<value_type>> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());

    pool = entt::reactive_mixin<entt::storage<value_type>>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
}

TYPED_TEST(ReactiveMixin, Move) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>().template on_update<test::empty>();
    registry.emplace<test::empty>(entity[0u]);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());

    entt::reactive_mixin<entt::storage<value_type>> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(other.index(entity[0u]), 0u);
    ASSERT_EQ(&other.registry(), &registry);

    other.clear();
    registry.replace<test::empty>(entity[0u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());

    std::swap(other, pool);
    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());

    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(&pool.registry(), &registry);

    other = entt::reactive_mixin<entt::storage<value_type>>{};
    other.bind(registry);
    other.template on_construct<test::empty>();
    registry.on_construct<test::empty>().disconnect(&pool);

    registry.emplace<test::empty>(entity[1u]);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.index(entity[0u]), 0u);
}

TYPED_TEST(ReactiveMixin, Swap) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    entt::reactive_mixin<entt::storage<value_type>> other;
    const std::array entity{registry.create(), registry.create()};

    registry.emplace<test::empty>(entity[0u]);

    pool.bind(registry);
    pool.template on_construct<test::empty>();

    other.bind(registry);
    other.template on_destroy<test::empty>();

    registry.emplace<test::empty>(entity[1u]);
    registry.erase<test::empty>(entity[0u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(other.index(entity[1u]), 0u);
}

TYPED_TEST(ReactiveMixin, OnConstruct) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const entt::entity entity{registry.create()};

    pool.bind(registry);
    registry.emplace<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    registry.clear<test::empty>();
    pool.template on_construct<test::other_empty>();
    registry.emplace<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    registry.on_construct<test::other_empty>().disconnect(&pool);
    registry.clear<test::empty>();
    pool.template on_construct<test::empty>();
    registry.emplace<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.clear<test::empty>();

    ASSERT_TRUE(pool.contains(entity));

    registry.emplace<test::empty>(entity);
    registry.emplace_or_replace<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.destroy(entity);

    ASSERT_TRUE(pool.contains(entity));
}

TYPED_TEST(ReactiveMixin, OnConstructCallback) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create(entt::entity{3})};

    pool.bind(registry);
    pool.template on_construct<test::empty, &emplace<entt::reactive_mixin<entt::storage<value_type>>, 3u>>();
    registry.emplace<test::empty>(entity[0u]);

    ASSERT_TRUE(pool.empty());

    registry.emplace<test::empty>(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));

    pool.clear();
    registry.clear<test::empty>();

    ASSERT_TRUE(pool.empty());

    registry.insert<test::empty>(entity.begin(), entity.end());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, OnConstruct) {
    using value_type = typename TestFixture::type;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH(pool.template on_construct<test::empty>(), "");
}

TYPED_TEST(ReactiveMixin, OnUpdate) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const entt::entity entity{registry.create()};

    pool.bind(registry);
    registry.emplace<test::empty>(entity);
    registry.patch<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    pool.template on_update<test::other_empty>();
    registry.patch<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    registry.on_update<test::other_empty>().disconnect(&pool);
    pool.template on_update<test::empty>();
    registry.patch<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.clear<test::empty>();

    ASSERT_TRUE(pool.contains(entity));

    registry.emplace<test::empty>(entity);
    registry.emplace_or_replace<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.destroy(entity);

    ASSERT_TRUE(pool.contains(entity));
}

TYPED_TEST(ReactiveMixin, OnUpdateCallback) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create(entt::entity{3})};

    pool.bind(registry);
    pool.template on_update<test::empty, &emplace<entt::reactive_mixin<entt::storage<value_type>>, 3u>>();
    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.patch<test::empty>(entity[0u]);

    ASSERT_TRUE(pool.empty());

    registry.patch<test::empty>(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));

    pool.clear();
    registry.clear<test::empty>();

    ASSERT_TRUE(pool.empty());

    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.patch<test::empty>(entity[0u]);
    registry.patch<test::empty>(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, OnUpdate) {
    using value_type = typename TestFixture::type;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH(pool.template on_update<test::empty>(), "");
}

TYPED_TEST(ReactiveMixin, OnDestroy) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const entt::entity entity{registry.create()};

    pool.bind(registry);
    registry.emplace<test::empty>(entity);
    registry.erase<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    pool.template on_destroy<test::other_empty>();
    registry.emplace<test::empty>(entity);
    registry.erase<test::empty>(entity);

    ASSERT_FALSE(pool.contains(entity));

    registry.on_destroy<test::other_empty>().disconnect(&pool);
    pool.template on_destroy<test::empty>();
    registry.emplace<test::empty>(entity);
    registry.erase<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.clear<test::empty>();

    ASSERT_TRUE(pool.contains(entity));

    registry.emplace<test::empty>(entity);
    registry.erase<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.destroy(entity);

    ASSERT_TRUE(pool.contains(entity));
}

TYPED_TEST(ReactiveMixin, OnDestroyCallback) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create(entt::entity{3})};

    pool.bind(registry);
    pool.template on_destroy<test::empty, &emplace<entt::reactive_mixin<entt::storage<value_type>>, 3u>>();
    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.erase<test::empty>(entity[0u]);

    ASSERT_TRUE(pool.empty());

    registry.erase<test::empty>(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));

    pool.clear();

    ASSERT_TRUE(pool.empty());

    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.erase<test::empty>(entity.begin(), entity.end());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, OnDestroy) {
    using value_type = typename TestFixture::type;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH(pool.template on_destroy<test::empty>(), "");
}

TYPED_TEST(ReactiveMixin, EntityLifecycle) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const entt::entity entity{registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>().template on_destroy<entt::entity, &remove<decltype(pool)>>();

    ASSERT_FALSE(pool.contains(entity));

    registry.emplace<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.erase<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.emplace<test::empty>(entity);
    registry.destroy(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TYPED_TEST(ReactiveMixin, ManagedStorage) {
    entt::registry registry;
    entt::storage_for_t<entt::reactive> &pool = registry.storage<entt::reactive>();
    const entt::entity entity{registry.create()};

    pool.template on_construct<test::empty>();
    registry.emplace<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.erase<test::empty>(entity);

    ASSERT_TRUE(pool.contains(entity));

    registry.emplace<test::empty>(entity);
    registry.destroy(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TYPED_TEST(ReactiveMixin, Registry) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;

    ASSERT_FALSE(pool);

    pool.bind(registry);

    ASSERT_TRUE(pool);
    ASSERT_EQ(&pool.registry(), &registry);
    ASSERT_EQ(&std::as_const(pool).registry(), &registry);
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, Registry) {
    using value_type = typename TestFixture::type;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH([[maybe_unused]] auto &registry = pool.registry(), "");
    ASSERT_DEATH([[maybe_unused]] const auto &registry = std::as_const(pool).registry(), "");
}

TYPED_TEST(ReactiveMixin, CustomRegistry) {
    using value_type = typename TestFixture::type;
    using registry_type = test::custom_registry<test::entity>;

    registry_type registry;
    entt::basic_reactive_mixin<entt::basic_storage<value_type, test::entity>, registry_type> pool;
    const std::array entity{registry.create(), registry.create()};

    ASSERT_FALSE(pool);

    pool.bind(registry);

    ASSERT_TRUE(pool);

    pool.template on_construct<test::empty>();
    registry.insert<test::empty>(entity.begin(), entity.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, CustomRegistry) {
    using value_type = typename TestFixture::type;
    using registry_type = test::custom_registry<test::entity>;
    entt::basic_reactive_mixin<entt::basic_storage<value_type, test::entity>, registry_type> pool;
    ASSERT_DEATH([[maybe_unused]] auto &registry = pool.registry(), "");
    ASSERT_DEATH([[maybe_unused]] const auto &registry = std::as_const(pool).registry(), "");
}

TYPED_TEST(ReactiveMixin, View) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>();
    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.insert<double>(entity.begin(), entity.end());
    registry.emplace<int>(entity[1u], 1);

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    const auto view = pool.view();
    const auto cview = std::as_const(pool).view();

    ASSERT_EQ(view.size(), 2u);
    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[0u]);

    ASSERT_EQ(cview.size(), view.size());

    for(const auto entt: cview) {
        ASSERT_TRUE(view.contains(entt));
    }

    const auto filtered = pool.template view<double>(entt::exclude<int>);
    const auto cfiltered = std::as_const(pool).template view<double>(entt::exclude<int>);

    ASSERT_EQ(filtered.size_hint(), 2u);
    ASSERT_EQ(std::distance(filtered.begin(), filtered.end()), 1);
    ASSERT_TRUE(filtered.contains(entity[0u]));
    ASSERT_FALSE(filtered.contains(entity[1u]));

    ASSERT_NE(std::distance(cfiltered.begin(), cfiltered.end()), 0);

    for(const auto entt: cfiltered) {
        ASSERT_TRUE(filtered.contains(entt));
    }
}

ENTT_DEBUG_TYPED_TEST(ReactiveMixinDeathTest, View) {
    using value_type = typename TestFixture::type;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH([[maybe_unused]] const auto view = pool.view(), "");
    ASSERT_DEATH([[maybe_unused]] const auto cview = std::as_const(pool).view(), "");
}

TYPED_TEST(ReactiveMixin, AutoDisconnection) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create(), registry.create()};

    ASSERT_TRUE(pool.empty());

    ASSERT_TRUE(registry.on_construct<test::empty>().empty());
    ASSERT_TRUE(registry.on_update<test::empty>().empty());
    ASSERT_TRUE(registry.on_destroy<test::empty>().empty());

    pool.bind(registry);
    pool.template on_construct<test::empty>();
    pool.template on_update<test::empty>();
    pool.template on_destroy<test::empty>();
    registry.emplace<test::empty>(entity[0u]);

    ASSERT_FALSE(pool.empty());

    ASSERT_FALSE(registry.on_construct<test::empty>().empty());
    ASSERT_FALSE(registry.on_update<test::empty>().empty());
    ASSERT_FALSE(registry.on_destroy<test::empty>().empty());

    pool.reset();

    ASSERT_FALSE(pool.empty());

    ASSERT_TRUE(registry.on_construct<test::empty>().empty());
    ASSERT_TRUE(registry.on_update<test::empty>().empty());
    ASSERT_TRUE(registry.on_destroy<test::empty>().empty());
}

TYPED_TEST(ReactiveMixin, CustomAllocator) {
    using value_type = typename TestFixture::type;
    using storage_type = entt::reactive_mixin<entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>>>;
    using registry_type = typename storage_type::registry_type;

    const test::throwing_allocator<entt::entity> allocator{};
    storage_type pool{allocator};
    registry_type registry;
    const std::array entity{registry.create(), registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>();

    pool.reserve(1u);

    ASSERT_NE(pool.capacity(), 0u);

    registry.template emplace<test::empty>(entity[0u]);
    registry.template emplace<test::empty>(entity[1u]);

    decltype(pool) other{std::move(pool), allocator};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_NE(other.capacity(), 0u);
    ASSERT_EQ(other.size(), 2u);

    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 2u);

    other = {};
    pool.swap(other);
    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 2u);

    pool.clear();

    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TYPED_TEST(ReactiveMixin, ThrowingAllocator) {
    using value_type = typename TestFixture::type;
    using storage_type = entt::reactive_mixin<entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>>>;
    using registry_type = typename storage_type::registry_type;

    storage_type pool{};
    registry_type registry;
    const std::array entity{registry.create(), registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>();

    pool.get_allocator().template throw_counter<entt::entity>(0u);

    ASSERT_THROW(pool.reserve(1u), test::throwing_allocator_exception);
    ASSERT_EQ(pool.capacity(), 0u);

    pool.get_allocator().template throw_counter<entt::entity>(1u);

    ASSERT_THROW(registry.template emplace<test::empty>(entity[0u]), test::throwing_allocator_exception);
    ASSERT_TRUE(registry.template all_of<test::empty>(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[0u]));

    registry.template clear<test::empty>();
    pool.get_allocator().template throw_counter<entt::entity>(1u);

    ASSERT_THROW(registry.template insert<test::empty>(entity.begin(), entity.end()), test::throwing_allocator_exception);
    ASSERT_TRUE(registry.template all_of<test::empty>(entity[0u]));
    ASSERT_TRUE(registry.template all_of<test::empty>(entity[1u]));
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
}
