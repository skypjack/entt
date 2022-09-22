#include <iterator>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include "../common/throwing_allocator.hpp"
#include "../common/throwing_type.hpp"

struct empty_type {};

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value{};
};

struct non_default_constructible {
    non_default_constructible() = delete;

    non_default_constructible(int v)
        : value{v} {}

    int value{};
};

struct counter {
    int value{};
};

template<typename Registry>
void listener(counter &counter, Registry &, typename Registry::entity_type) {
    ++counter.value;
}

TEST(SighStorageMixin, GenericType) {
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::sigh_storage_mixin<entt::storage<int>> pool;
    entt::sparse_set &base = pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    ASSERT_NE(base.emplace(entities[0u]), base.end());

    pool.emplace(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]), 0);
    ASSERT_EQ(pool.get(entities[1u]), 0);

    base.erase(entities[0u]);
    pool.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 2);
    ASSERT_TRUE(pool.empty());

    ASSERT_NE(base.insert(std::begin(entities), std::end(entities)), base.end());

    ASSERT_EQ(pool.get(entities[0u]), 0);
    ASSERT_EQ(pool.get(entities[1u]), 0);
    ASSERT_FALSE(pool.empty());

    base.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 3);
    ASSERT_FALSE(pool.empty());

    base.erase(entities[0u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_TRUE(pool.empty());

    pool.insert(std::begin(entities), std::end(entities), 3);

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]), 3);
    ASSERT_EQ(pool.get(entities[1u]), 3);

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 6);
    ASSERT_TRUE(pool.empty());
}

TEST(SighStorageMixin, StableType) {
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::sigh_storage_mixin<entt::storage<stable_type>> pool;
    entt::sparse_set &base = pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    ASSERT_NE(base.emplace(entities[0u]), base.end());

    pool.emplace(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]).value, 0);
    ASSERT_EQ(pool.get(entities[1u]).value, 0);

    base.erase(entities[0u]);
    pool.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 2);
    ASSERT_FALSE(pool.empty());

    ASSERT_NE(base.insert(std::begin(entities), std::end(entities)), base.end());

    ASSERT_EQ(pool.get(entities[0u]).value, 0);
    ASSERT_EQ(pool.get(entities[1u]).value, 0);
    ASSERT_FALSE(pool.empty());

    base.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 3);
    ASSERT_FALSE(pool.empty());

    base.erase(entities[0u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_FALSE(pool.empty());

    pool.insert(std::begin(entities), std::end(entities), stable_type{3});

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]).value, 3);
    ASSERT_EQ(pool.get(entities[1u]).value, 3);

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 6);
    ASSERT_FALSE(pool.empty());
}

TEST(SighStorageMixin, EmptyType) {
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::sigh_storage_mixin<entt::storage<empty_type>> pool;
    entt::sparse_set &base = pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    ASSERT_NE(base.emplace(entities[0u]), base.end());

    pool.emplace(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_FALSE(pool.empty());

    ASSERT_TRUE(pool.contains(entities[0u]));
    ASSERT_TRUE(pool.contains(entities[1u]));

    base.erase(entities[0u]);
    pool.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 2);
    ASSERT_TRUE(pool.empty());

    ASSERT_NE(base.insert(std::begin(entities), std::end(entities)), base.end());

    ASSERT_TRUE(pool.contains(entities[0u]));
    ASSERT_TRUE(pool.contains(entities[1u]));
    ASSERT_FALSE(pool.empty());

    base.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 3);
    ASSERT_FALSE(pool.empty());

    base.erase(entities[0u]);

    ASSERT_EQ(on_construct.value, 4);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_TRUE(pool.empty());

    pool.insert(std::begin(entities), std::end(entities));

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_FALSE(pool.empty());

    ASSERT_TRUE(pool.contains(entities[0u]));
    ASSERT_TRUE(pool.contains(entities[1u]));

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 6);
    ASSERT_TRUE(pool.empty());
}

TEST(SighStorageMixin, NonDefaultConstructibleType) {
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::sigh_storage_mixin<entt::storage<non_default_constructible>> pool;
    entt::sparse_set &base = pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    ASSERT_EQ(base.emplace(entities[0u]), base.end());

    pool.emplace(entities[1u], 3);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_FALSE(pool.empty());

    ASSERT_FALSE(pool.contains(entities[0u]));
    ASSERT_EQ(pool.get(entities[1u]).value, 3);

    base.erase(entities[1u]);

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_TRUE(pool.empty());

    ASSERT_EQ(base.insert(std::begin(entities), std::end(entities)), base.end());

    ASSERT_FALSE(pool.contains(entities[0u]));
    ASSERT_FALSE(pool.contains(entities[1u]));
    ASSERT_TRUE(pool.empty());

    pool.insert(std::begin(entities), std::end(entities), 3);

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]).value, 3);
    ASSERT_EQ(pool.get(entities[1u]).value, 3);

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(on_construct.value, 6);
    ASSERT_EQ(on_destroy.value, 3);
    ASSERT_TRUE(pool.empty());
}

TEST(SighStorageMixin, VoidType) {
    entt::sigh_storage_mixin<entt::storage<void>> pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entt::entity{99});

    ASSERT_EQ(pool.type(), entt::type_id<void>());
    ASSERT_TRUE(pool.contains(entt::entity{99}));

    entt::sigh_storage_mixin<entt::storage<void>> other{std::move(pool)};

    ASSERT_FALSE(pool.contains(entt::entity{99}));
    ASSERT_TRUE(other.contains(entt::entity{99}));

    pool = std::move(other);

    ASSERT_TRUE(pool.contains(entt::entity{99}));
    ASSERT_FALSE(other.contains(entt::entity{99}));

    pool.clear();

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 1);
}

TEST(SighStorageMixin, Move) {
    entt::sigh_storage_mixin<entt::storage<int>> pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entt::entity{3}, 3);

    ASSERT_TRUE(std::is_move_constructible_v<decltype(pool)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(pool)>);
    ASSERT_EQ(pool.type(), entt::type_id<int>());

    entt::sigh_storage_mixin<entt::storage<int>> other{std::move(pool)};

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});
    ASSERT_EQ(other.get(entt::entity{3}), 3);

    pool = std::move(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));

    other = entt::sigh_storage_mixin<entt::storage<int>>{};
    other.bind(entt::forward_as_any(registry));

    other.emplace(entt::entity{42}, 42);
    other = std::move(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});
    ASSERT_EQ(other.get(entt::entity{3}), 3);

    other.clear();

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 1);
}

TEST(SighStorageMixin, Swap) {
    entt::sigh_storage_mixin<entt::storage<int>> pool;
    entt::sigh_storage_mixin<entt::storage<int>> other;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    other.bind(entt::forward_as_any(registry));
    other.on_construct().connect<&listener<entt::registry>>(on_construct);
    other.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.emplace(entt::entity{42}, 41);

    other.emplace(entt::entity{9}, 8);
    other.emplace(entt::entity{3}, 2);
    other.erase(entt::entity{9});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(pool.get(entt::entity{3}), 2);

    ASSERT_EQ(other.at(0u), entt::entity{42});
    ASSERT_EQ(other.get(entt::entity{42}), 41);

    pool.clear();
    other.clear();

    ASSERT_EQ(on_construct.value, 3);
    ASSERT_EQ(on_destroy.value, 3);
}

TEST(SighStorageMixin, CustomAllocator) {
    auto test = [](auto pool, auto alloc) {
        using registry_type = typename decltype(pool)::registry_type;
        registry_type registry;

        counter on_construct{};
        counter on_destroy{};

        pool.bind(entt::forward_as_any(registry));
        pool.on_construct().template connect<&listener<registry_type>>(on_construct);
        pool.on_destroy().template connect<&listener<registry_type>>(on_destroy);

        pool.reserve(1u);

        ASSERT_NE(pool.capacity(), 0u);

        pool.emplace(entt::entity{0});
        pool.emplace(entt::entity{1});

        decltype(pool) other{std::move(pool), alloc};

        ASSERT_TRUE(pool.empty());
        ASSERT_FALSE(other.empty());
        ASSERT_EQ(pool.capacity(), 0u);
        ASSERT_NE(other.capacity(), 0u);
        ASSERT_EQ(other.size(), 2u);

        pool = std::move(other);

        ASSERT_FALSE(pool.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(other.capacity(), 0u);
        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 2u);

        pool.swap(other);
        pool = std::move(other);

        ASSERT_FALSE(pool.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(other.capacity(), 0u);
        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 2u);

        pool.clear();

        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 0u);

        ASSERT_EQ(on_construct.value, 2);
        ASSERT_EQ(on_destroy.value, 2);
    };

    test::throwing_allocator<entt::entity> allocator{};

    test(entt::sigh_storage_mixin<entt::basic_storage<int, entt::entity, test::throwing_allocator<int>>>{allocator}, allocator);
    test(entt::sigh_storage_mixin<entt::basic_storage<std::true_type, entt::entity, test::throwing_allocator<std::true_type>>>{allocator}, allocator);
    test(entt::sigh_storage_mixin<entt::basic_storage<stable_type, entt::entity, test::throwing_allocator<stable_type>>>{allocator}, allocator);
}

TEST(SighStorageMixin, ThrowingAllocator) {
    auto test = [](auto pool) {
        using pool_allocator_type = typename decltype(pool)::allocator_type;
        using value_type = typename decltype(pool)::value_type;
        using registry_type = typename decltype(pool)::registry_type;

        typename std::decay_t<decltype(pool)>::base_type &base = pool;
        constexpr auto packed_page_size = entt::component_traits<typename decltype(pool)::value_type>::page_size;
        constexpr auto sparse_page_size = entt::entt_traits<typename decltype(pool)::entity_type>::page_size;
        registry_type registry;

        counter on_construct{};
        counter on_destroy{};

        pool.bind(entt::forward_as_any(registry));
        pool.on_construct().template connect<&listener<registry_type>>(on_construct);
        pool.on_destroy().template connect<&listener<registry_type>>(on_destroy);

        pool_allocator_type::trigger_on_allocate = true;

        ASSERT_THROW(pool.reserve(1u), typename pool_allocator_type::exception_type);
        ASSERT_EQ(pool.capacity(), 0u);

        pool_allocator_type::trigger_after_allocate = true;

        ASSERT_THROW(pool.reserve(2 * packed_page_size), typename pool_allocator_type::exception_type);
        ASSERT_EQ(pool.capacity(), packed_page_size);

        pool.shrink_to_fit();

        ASSERT_EQ(pool.capacity(), 0u);

        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

        ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_FALSE(pool.contains(entt::entity{0}));
        ASSERT_TRUE(pool.empty());

        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

        ASSERT_THROW(base.emplace(entt::entity{0}), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_FALSE(base.contains(entt::entity{0}));
        ASSERT_TRUE(base.empty());

        pool_allocator_type::trigger_on_allocate = true;

        ASSERT_THROW(pool.emplace(entt::entity{0}, 0), typename pool_allocator_type::exception_type);
        ASSERT_FALSE(pool.contains(entt::entity{0}));
        ASSERT_NO_FATAL_FAILURE(pool.compact());
        ASSERT_TRUE(pool.empty());

        pool.emplace(entt::entity{0}, 0);
        const entt::entity entities[2u]{entt::entity{1}, entt::entity{sparse_page_size}};
        test::throwing_allocator<entt::entity>::trigger_after_allocate = true;

        ASSERT_THROW(pool.insert(std::begin(entities), std::end(entities), value_type{0}), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_TRUE(pool.contains(entt::entity{1}));
        ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));

        pool.erase(entt::entity{1});
        const value_type components[2u]{value_type{1}, value_type{sparse_page_size}};
        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;
        pool.compact();

        ASSERT_THROW(pool.insert(std::begin(entities), std::end(entities), std::begin(components)), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_TRUE(pool.contains(entt::entity{1}));
        ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));

        ASSERT_EQ(on_construct.value, 1);
        ASSERT_EQ(on_destroy.value, 1);
    };

    test(entt::sigh_storage_mixin<entt::basic_storage<int, entt::entity, test::throwing_allocator<int>>>{});
    test(entt::sigh_storage_mixin<entt::basic_storage<stable_type, entt::entity, test::throwing_allocator<stable_type>>>{});
}

TEST(SighStorageMixin, ThrowingComponent) {
    entt::sigh_storage_mixin<entt::storage<test::throwing_type>> pool;
    using registry_type = typename decltype(pool)::registry_type;
    registry_type registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<registry_type>>(on_construct);
    pool.on_destroy().connect<&listener<registry_type>>(on_destroy);

    test::throwing_type::trigger_on_value = 42;

    // strong exception safety
    ASSERT_THROW(pool.emplace(entt::entity{0}, test::throwing_type{42}), typename test::throwing_type::exception_type);
    ASSERT_TRUE(pool.empty());

    const entt::entity entities[2u]{entt::entity{42}, entt::entity{1}};
    const test::throwing_type components[2u]{42, 1};

    // basic exception safety
    ASSERT_THROW(pool.insert(std::begin(entities), std::end(entities), test::throwing_type{42}), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entt::entity{1}));

    // basic exception safety
    ASSERT_THROW(pool.insert(std::begin(entities), std::end(entities), std::begin(components)), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entt::entity{1}));

    // basic exception safety
    ASSERT_THROW(pool.insert(std::rbegin(entities), std::rend(entities), std::rbegin(components)), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.get(entt::entity{1}), 1);

    pool.clear();
    pool.emplace(entt::entity{1}, 1);
    pool.emplace(entt::entity{42}, 42);

    // basic exception safety
    ASSERT_THROW(pool.erase(entt::entity{1}), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entt::entity{42}));
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.at(0u), entt::entity{1});
    ASSERT_EQ(pool.at(1u), entt::entity{42});
    ASSERT_EQ(pool.get(entt::entity{42}), 42);
    // the element may have been moved but it's still there
    ASSERT_EQ(pool.get(entt::entity{1}), test::throwing_type::moved_from_value);

    test::throwing_type::trigger_on_value = 99;
    pool.erase(entt::entity{1});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{42}));
    ASSERT_FALSE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.at(0u), entt::entity{42});
    ASSERT_EQ(pool.get(entt::entity{42}), 42);

    ASSERT_EQ(on_construct.value, 2);
    ASSERT_EQ(on_destroy.value, 3);
}
