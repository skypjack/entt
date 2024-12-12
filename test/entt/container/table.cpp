#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/container/table.hpp>
#include <entt/core/iterator.hpp>
#include "../../common/config.h"
#include "../../common/linter.hpp"
#include "../../common/throwing_allocator.hpp"

TEST(Table, Constructors) {
    const std::vector<int> vec_of_int{1};
    const std::vector<char> vec_of_char{'a'};
    entt::table<int, char> table{};

    ASSERT_TRUE(table.empty());

    table = entt::table<int, char>{std::allocator<void>{}};

    ASSERT_TRUE(table.empty());

    table = entt::table<int, char>{vec_of_int, vec_of_char};

    ASSERT_EQ(table.size(), 1);

    table = entt::table<int, char>{std::vector<int>{1, 2}, std::vector<char>{'a', 'b'}};

    ASSERT_EQ(table.size(), 2);

    table = entt::table<int, char>{vec_of_int, vec_of_char, std::allocator<void>{}};

    ASSERT_EQ(table.size(), 1);

    table = entt::table<int, char>{std::vector<int>{1, 2}, std::vector<char>{'a', 'b'}, std::allocator<void>{}};

    ASSERT_EQ(table.size(), 2);
}

ENTT_DEBUG_TEST(TableDeathTest, Constructors) {
    const std::vector<int> vec_of_int{0};
    const std::vector<char> vec_of_char{};
    entt::table<int, char> table{};

    ASSERT_DEATH((table = entt::table<int, char>{vec_of_int, vec_of_char}), "");
    ASSERT_DEATH((table = entt::table<int, char>{std::vector<int>{}, std::vector<char>{'\0'}}), "");

    ASSERT_DEATH((table = entt::table<int, char>{vec_of_int, vec_of_char, std::allocator<void>{}}), "");
    ASSERT_DEATH((table = entt::table<int, char>{std::vector<int>{}, std::vector<char>{'\0'}, std::allocator<void>{}}), "");
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

    ASSERT_FALSE(table.empty());
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

TEST(Table, Emplace) {
    entt::table<int, char> table;

    testing::StaticAssertTypeEq<decltype(table.emplace()), std::tuple<int &, char &>>();

    ASSERT_EQ(table.emplace(), std::make_tuple(int{}, char{}));
    ASSERT_EQ(table.emplace(3, 'c'), std::make_tuple(3, 'c'));
}

TEST(Table, Erase) {
    entt::table<int, char> table;

    table.emplace(3, 'c');
    table.emplace(0, '\0');
    table.erase(table.begin());

    ASSERT_EQ(table.size(), 1u);
    ASSERT_EQ(table[0u], std::make_tuple(0, '\0'));

    table.emplace(3, 'c');
    table.erase(1u);

    ASSERT_EQ(table.size(), 1u);
    ASSERT_EQ(table[0u], std::make_tuple(0, '\0'));

    table.erase(0u);

    ASSERT_EQ(table.size(), 0u);
}

ENTT_DEBUG_TEST(TableDeathTest, Erase) {
    entt::table<int, char> table;

    ASSERT_DEATH(table.erase(0u), "");

    table.emplace(3, 'c');

    ASSERT_DEATH(table.erase(1u), "");
}

TEST(Table, Indexing) {
    entt::table<int, char> table;

    table.emplace(3, 'c');
    table.emplace(0, '\0');

    ASSERT_EQ(table[0u], std::make_tuple(3, 'c'));
    ASSERT_EQ(std::as_const(table)[1u], std::make_tuple(0, '\0'));
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

TEST(Table, CustomAllocator) {
    const test::throwing_allocator<void> allocator{};
    entt::basic_table<std::vector<int, test::throwing_allocator<int>>, std::vector<char, test::throwing_allocator<char>>> table{allocator};

    table.reserve(1u);

    ASSERT_NE(table.capacity(), 0u);

    table.emplace(3, 'c');
    table.emplace(0, '\0');

    decltype(table) other{std::move(table), allocator};

    test::is_initialized(table);

    ASSERT_TRUE(table.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_NE(other.capacity(), 0u);
    ASSERT_EQ(other.size(), 2u);

    table = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(table.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(table.capacity(), 0u);
    ASSERT_EQ(table.size(), 2u);

    other = {};
    table.swap(other);
    table = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(table.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(table.capacity(), 0u);
    ASSERT_EQ(table.size(), 2u);

    table.clear();

    ASSERT_NE(table.capacity(), 0u);
    ASSERT_EQ(table.size(), 0u);
}

TEST(Table, ThrowingAllocator) {
    test::throwing_allocator<void> allocator{};
    entt::basic_table<std::vector<int, test::throwing_allocator<int>>, std::vector<char, test::throwing_allocator<char>>> table{allocator};

    allocator.throw_counter<int>(0u);

    ASSERT_THROW(table.reserve(1u), test::throwing_allocator_exception);

    allocator.throw_counter<int>(0u);
    allocator.throw_counter<char>(0u);

    ASSERT_THROW(table.emplace(), test::throwing_allocator_exception);
    ASSERT_THROW(table.emplace(3, 'c'), test::throwing_allocator_exception);
}
