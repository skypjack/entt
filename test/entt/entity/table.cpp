#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>
#include <entt/entity/table.hpp>
#include "../../common/config.h"
#include "../../common/linter.hpp"

TEST(Table, Constructors) {
    entt::table<int, char> table;

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = table.get_allocator());

    table = entt::table<int, char>{std::allocator<void>{}};

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = table.get_allocator());
}

TEST(Table, Move) {
    entt::table<int, char> table;

    table.emplace(3, 'c');

    static_assert(std::is_move_constructible_v<decltype(table)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(table)>, "Move assignable type required");

    entt::table<int, char> other{std::move(table)};

    test::is_initialized(table);

    ASSERT_TRUE(table.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other[0u], std::make_tuple(3, 'c'));

    entt::table<int, char> extended{std::move(other), std::allocator<void>{}};

    test::is_initialized(other);

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(extended[0u], std::make_tuple(3, 'c'));

    table = std::move(extended);
    test::is_initialized(extended);

    ASSERT_FALSE(table.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(table[0u], std::make_tuple(3, 'c'));

    other = entt::table<int, char>{};
    other.emplace(1, 'a');
    other = std::move(table);
    test::is_initialized(table);

    ASSERT_TRUE(table.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other[0u], std::make_tuple(3, 'c'));
}

TEST(Table, Swap) {
    entt::table<int, char> table;
    entt::table<int, char> other;

    table.emplace(3, 'c');

    other.emplace(1, 'a');
    other.emplace(0, '\0');
    other.erase(0u);

    ASSERT_EQ(table.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    table.swap(other);

    ASSERT_EQ(table.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(table[0u], std::make_tuple(0, '\0'));
    ASSERT_EQ(other[0u], std::make_tuple(3, 'c'));
}

TEST(Table, Capacity) {
    entt::table<int, char> table;

    ASSERT_EQ(table.capacity(), 0u);
    ASSERT_TRUE(table.empty());

    table.reserve(64u);

    ASSERT_EQ(table.capacity(), 64u);
    ASSERT_TRUE(table.empty());

    table.reserve(0);

    ASSERT_EQ(table.capacity(), 64u);
    ASSERT_TRUE(table.empty());
}

TEST(Table, ShrinkToFit) {
    entt::table<int, char> table;

    table.reserve(64u);
    table.emplace(3, 'c');

    ASSERT_EQ(table.capacity(), 64u);
    ASSERT_FALSE(table.empty());

    table.shrink_to_fit();

    ASSERT_EQ(table.capacity(), 1u);
    ASSERT_FALSE(table.empty());

    table.clear();

    ASSERT_EQ(table.capacity(), 1u);
    ASSERT_TRUE(table.empty());

    table.shrink_to_fit();

    ASSERT_EQ(table.capacity(), 0u);
    ASSERT_TRUE(table.empty());
}

TEST(Table, Indexing) {
    entt::table<int, char> table;

    table.emplace(3, 'c');
    table.emplace(0, '\0');

    ASSERT_EQ(table[0u], std::make_tuple(3, 'c'));
    ASSERT_EQ(table[1u], std::make_tuple(0, '\0'));
}

ENTT_DEBUG_TEST(TableDeathTest, Indexing) {
    entt::table<int, char> table;

    ASSERT_DEATH([[maybe_unused]] auto value = table[0u], "");
    ASSERT_DEATH([[maybe_unused]] auto value = std::as_const(table)[0u], "");
}

TEST(Table, Clear) {
    entt::table<int, char> table;

    table.emplace(3, 'c');
    table.emplace(0, '\0');

    ASSERT_EQ(table.size(), 2u);

    table.clear();

    ASSERT_EQ(table.size(), 0u);

    table.emplace(3, 'c');
    table.emplace(0, '\0');
    table.erase(0u);

    ASSERT_EQ(table.size(), 1u);

    table.clear();

    ASSERT_EQ(table.size(), 0u);
}
