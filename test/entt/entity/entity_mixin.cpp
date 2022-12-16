#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/entity/storage_mixin.hpp>
#include "../common/config.h"

struct counter {
    int value{};
};

template<typename Registry>
void listener(counter &counter, Registry &, typename Registry::entity_type) {
    ++counter.value;
}

TEST(StorageTagEntity, Functionalities) {
    entt::entity entities[2u]{entt::entity{0}, entt::entity{1}};
    entt::storage<entt::entity_storage_tag> pool;

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);

    ASSERT_EQ(*pool.push(entt::null), entities[0u]);
    ASSERT_EQ(*pool.push(entt::tombstone), entities[1u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.in_use(1u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    pool.erase(entities[0u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
}

TEST(StorageTagEntity, Move) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity_storage_tag> pool;

    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    ASSERT_TRUE(std::is_move_constructible_v<decltype(pool)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(pool)>);

    entt::storage<entt::entity_storage_tag> other{std::move(pool)};

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 1u);
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{1});

    pool = std::move(other);

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 0u);
    ASSERT_EQ(pool.at(0u), entt::entity{1});
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));

    other = entt::storage<entt::entity_storage_tag>{};

    other.push(entt::entity{3});
    other = std::move(pool);

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 1u);
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{1});

    other.clear();

    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(other.in_use(), 0u);

    ASSERT_EQ(*other.push(entt::null), traits_type::construct(1, 1));
    ASSERT_EQ(*other.push(entt::null), entt::entity{0});
    ASSERT_EQ(*other.push(entt::null), entt::entity{2});
}

TEST(StorageTagEntity, Swap) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity_storage_tag> pool;
    entt::storage<entt::entity_storage_tag> other;

    pool.push(entt::entity{1});

    other.push(entt::entity{2});
    other.push(entt::entity{0});
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 3u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 1u);

    ASSERT_EQ(pool.at(0u), entt::entity{0});
    ASSERT_EQ(other.at(0u), entt::entity{1});

    pool.clear();
    other.clear();

    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 0u);

    ASSERT_EQ(*other.push(entt::null), traits_type::construct(1, 1));
    ASSERT_EQ(*other.push(entt::null), entt::entity{0});
    ASSERT_EQ(*other.push(entt::null), entt::entity{2});
}

TEST(StorageTagEntity, Push) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity_storage_tag> pool;

    ASSERT_EQ(*pool.push(entt::null), entt::entity{0});
    ASSERT_EQ(*pool.push(entt::tombstone), entt::entity{1});
    ASSERT_EQ(*pool.push(entt::entity{0}), entt::entity{2});
    ASSERT_EQ(*pool.push(traits_type::construct(1, 1)), entt::entity{3});
    ASSERT_EQ(*pool.push(traits_type::construct(5, 3)), traits_type::construct(5, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{1}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{2}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_GE(pool.index(entt::entity{4}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(5, 3)), pool.in_use());

    ASSERT_EQ(*pool.push(traits_type::construct(4, 42)), traits_type::construct(4, 42));
    ASSERT_EQ(*pool.push(traits_type::construct(4, 43)), entt::entity{6});

    entt::entity entities[2u]{entt::entity{1}, traits_type::construct(5, 3)};

    pool.erase(entities, entities + 2u);
    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.current(entities[0u]), 1);
    ASSERT_EQ(pool.current(entities[1u]), 4);
    ASSERT_EQ(pool.current(entt::entity{2}), 1);

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(1, 1)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(2, 1)), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(4, 42)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(5, 4)), pool.in_use());

    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(2, 1));
    ASSERT_EQ(*pool.push(traits_type::construct(1, 3)), traits_type::construct(1, 3));
    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(5, 4));
    ASSERT_EQ(*pool.push(entt::null), entt::entity{7});
}

ENTT_DEBUG_TEST(StorageTagEntityDeathTest, InUse) {
    entt::storage<entt::entity_storage_tag> pool;

    pool.push(entt::entity{0});
    pool.push(entt::entity{1});

    ASSERT_DEATH(pool.in_use(3u), "");
}

ENTT_DEBUG_TEST(StorageTagEntityDeathTest, SwapElements) {
    entt::storage<entt::entity_storage_tag> pool;

    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.contains(entt::entity{1}));

    ASSERT_DEATH(pool.swap_elements(entt::entity{0}, entt::entity{1}), "");
}

TEST(StorageTagEntity, SighMixin) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sigh_mixin<entt::storage<entt::entity_storage_tag>> pool;
    entt::registry registry;

    counter on_construct{};
    counter on_destroy{};

    pool.bind(entt::forward_as_any(registry));
    pool.on_construct().connect<&listener<entt::registry>>(on_construct);
    pool.on_destroy().connect<&listener<entt::registry>>(on_destroy);

    pool.push(entt::entity{1});

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 0);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    pool.erase(entt::entity{1});

    ASSERT_EQ(on_construct.value, 1);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);

    pool.push(traits_type::construct(0, 2));
    pool.push(traits_type::construct(2, 1));

    ASSERT_TRUE(pool.contains(traits_type::construct(0, 2)));
    ASSERT_TRUE(pool.contains(traits_type::construct(1, 1)));
    ASSERT_TRUE(pool.contains(traits_type::construct(2, 1)));

    ASSERT_EQ(on_construct.value, 3);
    ASSERT_EQ(on_destroy.value, 1);
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.clear();

    ASSERT_TRUE(pool.contains(traits_type::construct(0, 3)));
    ASSERT_TRUE(pool.contains(traits_type::construct(1, 1)));
    ASSERT_TRUE(pool.contains(traits_type::construct(2, 2)));

    ASSERT_EQ(on_construct.value, 3);
    // orphan entities are notified as well
    ASSERT_EQ(on_destroy.value, 4);
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.in_use(), 0u);
}
