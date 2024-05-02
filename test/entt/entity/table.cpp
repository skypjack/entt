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

TEST(Table, Iterator) {
    using iterator = typename entt::table<int, char>::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<int &, char &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<int &, char &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, std::tuple<int &, char &>>();

    entt::table<int, char> table;
    table.emplace(3, 'c');

    iterator end{table.begin()};
    iterator begin{};

    begin = table.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, table.begin());
    ASSERT_EQ(end, table.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, table.begin());
    ASSERT_EQ(begin--, table.end());

    ASSERT_EQ(begin + 1, table.end());
    ASSERT_EQ(end - 1, table.begin());

    ASSERT_EQ(++begin, table.end());
    ASSERT_EQ(--begin, table.begin());

    ASSERT_EQ(begin += 1, table.end());
    ASSERT_EQ(begin -= 1, table.begin());

    ASSERT_EQ(begin + (end - begin), table.end());
    ASSERT_EQ(begin - (begin - end), table.end());

    ASSERT_EQ(end - (end - begin), table.begin());
    ASSERT_EQ(end + (begin - end), table.begin());

    ASSERT_EQ(begin[0u], *table.begin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, table.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, table.end());

    table.emplace(0, '\0');
    begin = table.begin();

    ASSERT_EQ(begin[0u], std::make_tuple(3, 'c'));
    ASSERT_EQ(begin[1u], std::make_tuple(0, '\0'));
}

TEST(Table, ConstIterator) {
    using iterator = typename entt::table<int, char>::const_iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<const int &, const char &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<const int &, const char &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, std::tuple<const int &, const char &>>();

    entt::table<int, char> table;
    table.emplace(3, 'c');

    iterator cend{table.cbegin()};
    iterator cbegin{};

    cbegin = table.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(table).begin());
    ASSERT_EQ(cend, std::as_const(table).end());
    ASSERT_EQ(cbegin, table.cbegin());
    ASSERT_EQ(cend, table.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, table.cbegin());
    ASSERT_EQ(cbegin--, table.cend());

    ASSERT_EQ(cbegin + 1, table.cend());
    ASSERT_EQ(cend - 1, table.cbegin());

    ASSERT_EQ(++cbegin, table.cend());
    ASSERT_EQ(--cbegin, table.cbegin());

    ASSERT_EQ(cbegin += 1, table.cend());
    ASSERT_EQ(cbegin -= 1, table.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), table.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), table.cend());

    ASSERT_EQ(cend - (cend - cbegin), table.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), table.cbegin());

    ASSERT_EQ(cbegin[0u], *table.cbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, table.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, table.cend());

    table.emplace(0, '\0');
    cbegin = table.cbegin();

    ASSERT_EQ(cbegin[0u], std::make_tuple(3, 'c'));
    ASSERT_EQ(cbegin[1u], std::make_tuple(0, '\0'));
}

TEST(Table, ReverseIterator) {
    using reverse_iterator = typename entt::table<int, char>::reverse_iterator;

    testing::StaticAssertTypeEq<typename reverse_iterator::value_type, std::tuple<int &, char &>>();
    testing::StaticAssertTypeEq<typename reverse_iterator::pointer, entt::input_iterator_pointer<std::tuple<int &, char &>>>();
    testing::StaticAssertTypeEq<typename reverse_iterator::reference, std::tuple<int &, char &>>();

    entt::table<int, char> table;
    table.emplace(3, 'c');

    reverse_iterator end{table.rbegin()};
    reverse_iterator begin{};

    begin = table.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, table.rbegin());
    ASSERT_EQ(end, table.rend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, table.rbegin());
    ASSERT_EQ(begin--, table.rend());

    ASSERT_EQ(begin + 1, table.rend());
    ASSERT_EQ(end - 1, table.rbegin());

    ASSERT_EQ(++begin, table.rend());
    ASSERT_EQ(--begin, table.rbegin());

    ASSERT_EQ(begin += 1, table.rend());
    ASSERT_EQ(begin -= 1, table.rbegin());

    ASSERT_EQ(begin + (end - begin), table.rend());
    ASSERT_EQ(begin - (begin - end), table.rend());

    ASSERT_EQ(end - (end - begin), table.rbegin());
    ASSERT_EQ(end + (begin - end), table.rbegin());

    ASSERT_EQ(begin[0u], *table.rbegin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, table.rbegin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, table.rend());

    table.emplace(0, '\0');
    begin = table.rbegin();

    ASSERT_EQ(begin[0u], std::make_tuple(0, '\0'));
    ASSERT_EQ(begin[1u], std::make_tuple(3, 'c'));
}

TEST(Table, ConstReverseIterator) {
    using const_reverse_iterator = typename entt::table<int, char>::const_reverse_iterator;

    testing::StaticAssertTypeEq<typename const_reverse_iterator::value_type, std::tuple<const int &, const char &>>();
    testing::StaticAssertTypeEq<typename const_reverse_iterator::pointer, entt::input_iterator_pointer<std::tuple<const int &, const char &>>>();
    testing::StaticAssertTypeEq<typename const_reverse_iterator::reference, std::tuple<const int &, const char &>>();

    entt::table<int, char> table;
    table.emplace(3, 'c');

    const_reverse_iterator cend{table.crbegin()};
    const_reverse_iterator cbegin{};

    cbegin = table.crend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(table).rbegin());
    ASSERT_EQ(cend, std::as_const(table).rend());
    ASSERT_EQ(cbegin, table.crbegin());
    ASSERT_EQ(cend, table.crend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, table.crbegin());
    ASSERT_EQ(cbegin--, table.crend());

    ASSERT_EQ(cbegin + 1, table.crend());
    ASSERT_EQ(cend - 1, table.crbegin());

    ASSERT_EQ(++cbegin, table.crend());
    ASSERT_EQ(--cbegin, table.crbegin());

    ASSERT_EQ(cbegin += 1, table.crend());
    ASSERT_EQ(cbegin -= 1, table.crbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), table.crend());
    ASSERT_EQ(cbegin - (cbegin - cend), table.crend());

    ASSERT_EQ(cend - (cend - cbegin), table.crbegin());
    ASSERT_EQ(cend + (cbegin - cend), table.crbegin());

    ASSERT_EQ(cbegin[0u], *table.crbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, table.crbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, table.crend());

    table.emplace(0, '\0');
    cbegin = table.crbegin();

    ASSERT_EQ(cbegin[0u], std::make_tuple(0, '\0'));
    ASSERT_EQ(cbegin[1u], std::make_tuple(3, 'c'));
}

TEST(Table, IteratorConversion) {
    entt::table<int, char> table;

    table.emplace(3, 'c');

    const typename entt::table<int, char>::iterator it = table.begin();
    typename entt::table<int, char>::const_iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<int &, char &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<const int &, const char &>>();

    ASSERT_EQ(*it.operator->(), std::make_tuple(3, 'c'));
    ASSERT_EQ(*it.operator->(), *cit);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
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
