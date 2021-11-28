#include <iterator>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/component.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>

struct empty_type {};

struct stable_type {
    int value;
};

struct non_default_constructible {
    non_default_constructible() = delete;

    non_default_constructible(int v)
        : value{v} {}

    int value;
};

template<>
struct entt::component_traits<stable_type>: basic_component_traits {
    static constexpr auto in_place_delete = true;
};

struct counter {
    int value{};
};

void listener(counter &counter, entt::registry &, entt::entity) {
    ++counter.value;
}

TEST(SighStorageMixin, GenericType) {
    entt::sigh_storage_mixin<entt::storage<int>> pool;
    entt::sparse_set &base = pool;
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::registry registry{};

    pool.bind(entt::forward_as_any(registry));

    counter on_construct{};
    counter on_destroy{};

    pool.on_construct().connect<&listener>(on_construct);
    pool.on_destroy().connect<&listener>(on_destroy);

    base.emplace(entities[0u]);
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

    base.insert(std::begin(entities), std::end(entities));

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

TEST(SighStorageMixin, EmptyType) {
    entt::sigh_storage_mixin<entt::storage<empty_type>> pool;
    entt::sparse_set &base = pool;
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::registry registry{};

    pool.bind(entt::forward_as_any(registry));

    counter on_construct{};
    counter on_destroy{};

    pool.on_construct().connect<&listener>(on_construct);
    pool.on_destroy().connect<&listener>(on_destroy);

    base.emplace(entities[0u]);
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

    base.insert(std::begin(entities), std::end(entities));

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
    entt::sigh_storage_mixin<entt::storage<non_default_constructible>> pool;
    entt::sparse_set &base = pool;
    entt::entity entities[2u]{entt::entity{3}, entt::entity{42}};
    entt::registry registry{};

    pool.bind(entt::forward_as_any(registry));

    counter on_construct{};
    counter on_destroy{};

    pool.on_construct().connect<&listener>(on_construct);
    pool.on_destroy().connect<&listener>(on_destroy);

    ASSERT_DEATH(base.emplace(entities[0u]), "");

    pool.emplace(entities[1u], 3);

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_FALSE(pool.empty());

    ASSERT_FALSE(pool.contains(entities[0u]));
    ASSERT_EQ(pool.get(entities[1u]).value, 3);

    base.erase(entities[1u]);

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_TRUE(pool.empty());

    ASSERT_DEATH(base.insert(std::begin(entities), std::end(entities)), "");

    ASSERT_FALSE(pool.contains(entities[0u]));
    ASSERT_FALSE(pool.contains(entities[1u]));
    ASSERT_TRUE(pool.empty());

    pool.insert(std::begin(entities), std::end(entities), 3);

    ASSERT_EQ(on_construct.value, 3);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_FALSE(pool.empty());

    ASSERT_EQ(pool.get(entities[0u]).value, 3);
    ASSERT_EQ(pool.get(entities[1u]).value, 3);

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_EQ(on_construct.value, 3);
    ASSERT_EQ(on_destroy.value, 3);
    ASSERT_TRUE(pool.empty());
}
