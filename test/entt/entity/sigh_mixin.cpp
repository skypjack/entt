#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/signal/sigh.hpp>
#include "../../common/config.h"
#include "../../common/entity.h"
#include "../../common/linter.hpp"
#include "../../common/non_default_constructible.h"
#include "../../common/pointer_stable.h"
#include "../../common/registry.h"
#include "../../common/throwing_allocator.hpp"
#include "../../common/throwing_type.hpp"

struct auto_signal final {
    auto_signal(bool &cflag, bool &uflag, bool &dflag)
        : created{&cflag},
          updated{&uflag},
          destroyed{&dflag} {}

    static void on_construct(entt::registry &registry, const entt::entity entt) {
        *registry.get<auto_signal>(entt).created = true;
    }

    static void on_update(entt::registry &registry, const entt::entity entt) {
        *registry.get<auto_signal>(entt).updated = true;
    }

    static void on_destroy(entt::registry &registry, const entt::entity entt) {
        *registry.get<auto_signal>(entt).destroyed = true;
    }

private:
    bool *created{};
    bool *updated{};
    bool *destroyed{};
};

template<typename Registry>
void listener(std::size_t &counter, Registry &, typename Registry::entity_type) {
    ++counter;
}

template<typename Type>
struct SighMixin: testing::Test {
    using type = Type;
};

template<typename Type>
using SighMixinDeathTest = SighMixin<Type>;

using SighMixinTypes = ::testing::Types<int, test::pointer_stable>;

TYPED_TEST_SUITE(SighMixin, SighMixinTypes, );
TYPED_TEST_SUITE(SighMixinDeathTest, SighMixinTypes, );

TYPED_TEST(SighMixin, Functionalities) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::registry registry;
    auto &pool = registry.storage<value_type>();
    const std::array entity{registry.create(), registry.create()};

    testing::StaticAssertTypeEq<decltype(pool), entt::sigh_mixin<entt::storage<value_type>> &>();

    std::size_t on_construct{};
    std::size_t on_destroy{};

    ASSERT_EQ(pool.size(), 0u);

    pool.insert(entity.begin(), entity.begin() + 1u);
    pool.erase(entity[0u]);

    ASSERT_EQ(pool.size(), traits_type::in_place_delete);
    ASSERT_EQ(on_construct, 0u);
    ASSERT_EQ(on_destroy, 0u);

    pool.on_construct().template connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().template connect<&listener<entt::registry>>(on_destroy);

    ASSERT_NE(pool.push(entity[0u]), pool.entt::sparse_set::end());

    pool.emplace(entity[1u]);

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 0u);
    ASSERT_EQ(pool.size(), 2u);

    ASSERT_EQ(pool.get(entity[0u]), value_type{0});
    ASSERT_EQ(pool.get(entity[1u]), value_type{0});

    pool.erase(entity.begin(), entity.end());

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 2u);
    ASSERT_EQ(pool.size(), 2u * traits_type::in_place_delete);

    ASSERT_NE(pool.push(entity.begin(), entity.end()), pool.entt::sparse_set::end());

    ASSERT_EQ(pool.get(entity[0u]), value_type{0});
    ASSERT_EQ(pool.get(entity[1u]), value_type{0});
    ASSERT_EQ(pool.size(), traits_type::in_place_delete ? 4u : 2u);

    pool.erase(entity[1u]);

    ASSERT_EQ(on_construct, 4u);
    ASSERT_EQ(on_destroy, 3u);
    ASSERT_EQ(pool.size(), traits_type::in_place_delete ? 4u : 1u);

    pool.erase(entity[0u]);

    ASSERT_EQ(on_construct, 4u);
    ASSERT_EQ(on_destroy, 4u);
    ASSERT_EQ(pool.size(), traits_type::in_place_delete ? 4u : 0u);

    pool.insert(entity.begin(), entity.end(), value_type{3});

    ASSERT_EQ(on_construct, 6u);
    ASSERT_EQ(on_destroy, 4u);
    ASSERT_EQ(pool.size(), traits_type::in_place_delete ? 6u : 2u);

    ASSERT_EQ(pool.get(entity[0u]), value_type{3});
    ASSERT_EQ(pool.get(entity[1u]), value_type{3});

    pool.clear();

    ASSERT_EQ(on_construct, 6u);
    ASSERT_EQ(on_destroy, 6u);
    ASSERT_EQ(pool.size(), 0u);
}

TYPED_TEST(SighMixin, InsertWeakRange) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    auto &pool = registry.storage<value_type>();
    const auto view = registry.view<entt::entity>(entt::exclude<value_type>);
    [[maybe_unused]] const std::array entity{registry.create(), registry.create()};
    std::size_t on_construct{};

    ASSERT_EQ(on_construct, 0u);

    pool.on_construct().template connect<&listener<entt::registry>>(on_construct);
    pool.insert(view.begin(), view.end());

    ASSERT_EQ(on_construct, 2u);
}

TEST(SighMixin, NonDefaultConstructibleType) {
    entt::registry registry;
    auto &pool = registry.storage<test::non_default_constructible>();
    const std::array entity{registry.create(), registry.create()};

    testing::StaticAssertTypeEq<decltype(pool), entt::sigh_mixin<entt::storage<test::non_default_constructible>> &>();

    std::size_t on_construct{};
    std::size_t on_destroy{};

    ASSERT_EQ(pool.size(), 0u);

    pool.insert(entity.begin(), entity.begin() + 1u, 0);
    pool.erase(entity[0u]);

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(on_construct, 0u);
    ASSERT_EQ(on_destroy, 0u);

    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    ASSERT_EQ(pool.push(entity[0u]), pool.entt::sparse_set::end());

    pool.emplace(entity[1u], 3);

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 0u);
    ASSERT_EQ(pool.size(), 1u);

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_EQ(pool.get(entity[1u]).value, 3);

    pool.erase(entity[1u]);

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 1u);
    ASSERT_EQ(pool.size(), 0u);

    ASSERT_EQ(pool.push(entity.begin(), entity.end()), pool.entt::sparse_set::end());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.size(), 0u);

    pool.insert(entity.begin(), entity.end(), 3);

    ASSERT_EQ(on_construct, 3u);
    ASSERT_EQ(on_destroy, 1u);
    ASSERT_EQ(pool.size(), 2u);

    ASSERT_EQ(pool.get(entity[0u]).value, 3);
    ASSERT_EQ(pool.get(entity[1u]).value, 3);

    pool.erase(entity.begin(), entity.end());

    ASSERT_EQ(on_construct, 3u);
    ASSERT_EQ(on_destroy, 3u);
    ASSERT_EQ(pool.size(), 0u);
}

TEST(SighMixin, VoidType) {
    entt::registry registry;
    auto &pool = registry.storage<void>();
    const auto entity = registry.create();

    testing::StaticAssertTypeEq<decltype(pool), entt::sigh_mixin<entt::storage<void>> &>();

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entity);

    ASSERT_EQ(pool.info(), entt::type_id<void>());
    ASSERT_TRUE(pool.contains(entity));

    entt::sigh_mixin<entt::storage<void>> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_TRUE(other.contains(entity));

    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_TRUE(pool.contains(entity));
    ASSERT_TRUE(other.empty());

    pool.clear();

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 1u);
}

TEST(SighMixin, StorageEntity) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    auto &pool = registry.storage<entt::entity>();

    testing::StaticAssertTypeEq<decltype(pool), entt::sigh_mixin<entt::storage<entt::entity>> &>();

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.push(entt::entity{1});

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 0u);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.free_list(), 1u);

    pool.erase(entt::entity{1});

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 1u);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.free_list(), 0u);

    pool.push(traits_type::construct(0, 2));
    pool.push(traits_type::construct(2, 1));

    ASSERT_TRUE(pool.contains(traits_type::construct(0, 2)));
    ASSERT_TRUE(pool.contains(traits_type::construct(1, 1)));
    ASSERT_TRUE(pool.contains(traits_type::construct(2, 1)));

    ASSERT_EQ(on_construct, 3u);
    ASSERT_EQ(on_destroy, 1u);
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.free_list(), 2u);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.free_list(), 0u);

    ASSERT_EQ(on_construct, 3u);
    ASSERT_EQ(on_destroy, 3u);

    pool.generate();
    pool.generate(entt::entity{0});

    std::array<entt::entity, 1u> entity{};
    pool.generate(entity.begin(), entity.end());

    ASSERT_EQ(on_construct, 6u);
    ASSERT_EQ(on_destroy, 3u);
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.free_list(), 3u);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.free_list(), 0u);
}

TYPED_TEST(SighMixin, Move) {
    using value_type = typename TestFixture::type;

    entt::sigh_mixin<entt::storage<value_type>> pool;
    entt::registry registry;

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.bind(registry);
    pool.on_construct().template connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().template connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entt::entity{3}, 3);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());

    entt::sigh_mixin<entt::storage<value_type>> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(other.index(entt::entity{3}), 0u);
    ASSERT_EQ(other.get(entt::entity{3}), value_type{3});

    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());

    ASSERT_EQ(pool.index(entt::entity{3}), 0u);
    ASSERT_EQ(pool.get(entt::entity{3}), value_type{3});

    other = entt::sigh_mixin<entt::storage<value_type>>{};
    other.bind(registry);

    other.emplace(entt::entity{1}, 1);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.index(entt::entity{3}), 0u);
    ASSERT_EQ(other.get(entt::entity{3}), value_type{3});

    other.clear();

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 1u);
}

TYPED_TEST(SighMixin, Swap) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::sigh_mixin<entt::storage<value_type>> pool;
    entt::sigh_mixin<entt::storage<value_type>> other;
    entt::registry registry;

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.bind(registry);
    pool.on_construct().template connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().template connect<&listener<entt::registry>>(on_destroy);

    other.bind(registry);
    other.on_construct().template connect<&listener<entt::registry>>(on_construct);
    other.on_destroy().template connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entt::entity{4}, 1);

    other.emplace(entt::entity{2}, 2);
    other.emplace(entt::entity{1}, 3);
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u + traits_type::in_place_delete);

    pool.swap(other);

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(pool.size(), 1u + traits_type::in_place_delete);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entt::entity{1}), traits_type::in_place_delete);
    ASSERT_EQ(other.index(entt::entity{4}), 0u);

    ASSERT_EQ(pool.get(entt::entity{1}), value_type{3});
    ASSERT_EQ(other.get(entt::entity{4}), value_type{1});

    pool.clear();
    other.clear();

    ASSERT_EQ(on_construct, 3u);
    ASSERT_EQ(on_destroy, 3u);
}

TEST(SighMixin, AutoSignal) {
    entt::registry registry;
    const auto entity = registry.create();

    bool created{};
    bool updated{};
    bool destroyed{};

    registry.emplace<auto_signal>(entity, created, updated, destroyed);
    registry.replace<auto_signal>(entity, created, updated, destroyed);
    registry.erase<auto_signal>(entity);

    ASSERT_TRUE(created);
    ASSERT_TRUE(updated);
    ASSERT_TRUE(destroyed);

    ASSERT_TRUE(registry.storage<auto_signal>().empty());
    ASSERT_TRUE(registry.valid(entity));

    created = updated = destroyed = false;

    registry.emplace<auto_signal>(entity, created, updated, destroyed);
    registry.replace<auto_signal>(entity, created, updated, destroyed);
    registry.destroy(entity);

    ASSERT_TRUE(created);
    ASSERT_TRUE(updated);
    ASSERT_TRUE(destroyed);

    ASSERT_TRUE(registry.storage<auto_signal>().empty());
    ASSERT_FALSE(registry.valid(entity));
}

TYPED_TEST(SighMixin, Registry) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::sigh_mixin<entt::storage<value_type>> pool;

    ASSERT_FALSE(pool);

    pool.bind(registry);

    ASSERT_TRUE(pool);
    ASSERT_EQ(&pool.registry(), &registry);
    ASSERT_EQ(&std::as_const(pool).registry(), &registry);
}

ENTT_DEBUG_TYPED_TEST(SighMixinDeathTest, Registry) {
    using value_type = typename TestFixture::type;
    entt::sigh_mixin<entt::storage<value_type>> pool;
    ASSERT_DEATH([[maybe_unused]] auto &registry = pool.registry(), "");
    ASSERT_DEATH([[maybe_unused]] const auto &registry = std::as_const(pool).registry(), "");
}

TYPED_TEST(SighMixin, CustomRegistry) {
    using value_type = typename TestFixture::type;
    using registry_type = test::custom_registry<test::entity>;

    registry_type registry;
    entt::basic_sigh_mixin<entt::basic_storage<value_type, test::entity>, registry_type> pool;
    const std::array entity{registry.create(), registry.create()};

    ASSERT_FALSE(pool);

    pool.bind(registry);

    ASSERT_TRUE(pool);

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.on_construct().template connect<&listener<registry_type>>(on_construct);
    pool.on_destroy().template connect<&listener<registry_type>>(on_destroy);

    pool.emplace(entity[0u]);
    pool.emplace(entity[1u]);

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 0u);

    pool.clear();

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 2u);
}

ENTT_DEBUG_TYPED_TEST(SighMixinDeathTest, CustomRegistry) {
    using value_type = typename TestFixture::type;
    using registry_type = test::custom_registry<test::entity>;
    entt::basic_sigh_mixin<entt::basic_storage<value_type, test::entity>, registry_type> pool;
    ASSERT_DEATH([[maybe_unused]] auto &registry = pool.registry(), "");
    ASSERT_DEATH([[maybe_unused]] const auto &registry = std::as_const(pool).registry(), "");
}

TYPED_TEST(SighMixin, CustomAllocator) {
    using value_type = typename TestFixture::type;
    using storage_type = entt::sigh_mixin<entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>>>;
    using registry_type = typename storage_type::registry_type;

    const test::throwing_allocator<entt::entity> allocator{};
    storage_type pool{allocator};
    registry_type registry;

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.bind(registry);
    pool.on_construct().template connect<&listener<registry_type>>(on_construct);
    pool.on_destroy().template connect<&listener<registry_type>>(on_destroy);

    pool.reserve(1u);

    ASSERT_NE(pool.capacity(), 0u);

    pool.emplace(entt::entity{0});
    pool.emplace(entt::entity{1});

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

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 2u);
}

TYPED_TEST(SighMixin, ThrowingAllocator) {
    using value_type = typename TestFixture::type;
    using storage_type = entt::sigh_mixin<entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>>>;
    using registry_type = typename storage_type::registry_type;

    storage_type pool{};
    typename storage_type::base_type &base = pool;
    registry_type registry;

    constexpr auto packed_page_size = entt::component_traits<value_type>::page_size;
    constexpr auto sparse_page_size = entt::entt_traits<entt::entity>::page_size;

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.bind(registry);
    pool.on_construct().template connect<&listener<registry_type>>(on_construct);
    pool.on_destroy().template connect<&listener<registry_type>>(on_destroy);

    pool.get_allocator().template throw_counter<value_type>(0u);

    ASSERT_THROW(pool.reserve(1u), test::throwing_allocator_exception);
    ASSERT_EQ(pool.capacity(), 0u);

    pool.get_allocator().template throw_counter<value_type>(1u);

    ASSERT_THROW(pool.reserve(2 * packed_page_size), test::throwing_allocator_exception);
    ASSERT_EQ(pool.capacity(), packed_page_size);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);

    pool.get_allocator().template throw_counter<entt::entity>(0u);

    ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator_exception);
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.empty());

    pool.get_allocator().template throw_counter<entt::entity>(0u);

    ASSERT_THROW(base.push(entt::entity{0}), test::throwing_allocator_exception);
    ASSERT_FALSE(base.contains(entt::entity{0}));
    ASSERT_TRUE(base.empty());

    pool.get_allocator().template throw_counter<value_type>(0u);

    ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator_exception);
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_NO_THROW(pool.compact());
    ASSERT_TRUE(pool.empty());

    pool.emplace(entt::entity{0}, 0);
    const std::array entity{entt::entity{1}, entt::entity{sparse_page_size}};
    pool.get_allocator().template throw_counter<entt::entity>(1u);

    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value_type{0}), test::throwing_allocator_exception);
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));

    pool.erase(entt::entity{1});
    const std::array component{value_type{1}, value_type{sparse_page_size}};
    pool.get_allocator().template throw_counter<entt::entity>(0u);
    pool.compact();

    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), component.begin()), test::throwing_allocator_exception);
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));

    ASSERT_EQ(on_construct, 1u);
    ASSERT_EQ(on_destroy, 1u);
}

TEST(SighMixin, ThrowingComponent) {
    using storage_type = entt::sigh_mixin<entt::storage<test::throwing_type>>;
    using registry_type = typename storage_type::registry_type;

    storage_type pool;
    registry_type registry;

    std::size_t on_construct{};
    std::size_t on_destroy{};

    pool.bind(registry);
    pool.on_construct().connect<&listener<registry_type>>(on_construct);
    pool.on_destroy().connect<&listener<registry_type>>(on_destroy);

    const std::array entity{entt::entity{3}, entt::entity{1}};
    const std::array<test::throwing_type, 2u> value{true, false};

    // strong exception safety
    ASSERT_THROW(pool.emplace(entity[0u], value[0u]), test::throwing_type_exception);
    ASSERT_TRUE(pool.empty());

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value[0u]), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entity[1u]));

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value.begin()), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entity[1u]));

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.rbegin(), entity.rend(), value.rbegin()), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.get(entity[1u]), value[1u]);

    pool.clear();
    pool.emplace(entity[1u], value[0u].throw_on_copy());
    pool.emplace(entity[0u], value[1u].throw_on_copy());

    // basic exception safety
    ASSERT_THROW(pool.erase(entity[1u]), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
    ASSERT_EQ(pool.get(entity[0u]), value[1u]);
    // the element may have been moved but it's still there
    ASSERT_EQ(pool.get(entity[1u]), value[0u]);

    pool.get(entity[1u]).throw_on_copy(false);
    pool.erase(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.get(entity[0u]), value[1u]);

    ASSERT_EQ(on_construct, 2u);
    ASSERT_EQ(on_destroy, 3u);
}
