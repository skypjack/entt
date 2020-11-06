#include <memory>
#include <utility>
#include <iterator>
#include <exception>
#include <type_traits>
#include <unordered_set>
#include <gtest/gtest.h>
#include <entt/entity/storage.hpp>
#include <entt/entity/fwd.hpp>

struct empty_type {};
struct boxed_int { int value; };

bool operator==(const boxed_int &lhs, const boxed_int &rhs) {
    return lhs.value == rhs.value;
}

struct throwing_component {
    struct constructor_exception: std::exception {};

    [[noreturn]] throwing_component() { throw constructor_exception{}; }

    // necessary to disable the empty type optimization
    int data;
};

TEST(Storage, Functionalities) {
    entt::storage<int> pool;

    pool.reserve(42);

    ASSERT_EQ(pool.capacity(), 42u);
    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    pool.emplace(entt::entity{41}, 3);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_NE(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_NE(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.contains(entt::entity{41}));
    ASSERT_EQ(pool.get(entt::entity{41}), 3);

    pool.remove(entt::entity{41});

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    pool.emplace(entt::entity{41}, 12);

    ASSERT_EQ(pool.get(entt::entity{41}), 12);

    pool.clear();

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    ASSERT_EQ(pool.capacity(), 42u);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);

    (void)entt::storage<int>{std::move(pool)};
    entt::storage<int> other;
    other = std::move(pool);
}

TEST(Storage, EmptyType) {
    entt::storage<empty_type> pool;
    pool.emplace(entt::entity{99});

    ASSERT_TRUE(pool.contains(entt::entity{99}));
}

TEST(Storage, Insert) {
    entt::storage<int> pool;
    entt::entity entities[2];

    entities[0] = entt::entity{3};
    entities[1] = entt::entity{42};
    pool.insert(std::begin(entities), std::end(entities), {});

    ASSERT_TRUE(pool.contains(entities[0]));
    ASSERT_TRUE(pool.contains(entities[1]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.get(entities[0]), 0);
    ASSERT_EQ(pool.get(entities[1]), 0);
}

TEST(Storage, InsertEmptyType) {
    entt::storage<empty_type> pool;
    entt::entity entities[2];

    entities[0] = entt::entity{3};
    entities[1] = entt::entity{42};

    pool.insert(std::begin(entities), std::end(entities));

    ASSERT_TRUE(pool.contains(entities[0]));
    ASSERT_TRUE(pool.contains(entities[1]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
}

TEST(Storage, Remove) {
    entt::storage<int> pool;
    entt::sparse_set &base = pool;

    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});
    base.remove(base.begin(), base.end());

    ASSERT_TRUE(pool.empty());

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{42}, 42);
    pool.emplace(entt::entity{9}, 9);
    base.remove(base.rbegin(), base.rbegin() + 2u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 9);

    pool.clear();
    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{42}, 42);
    pool.emplace(entt::entity{9}, 9);

    entt::entity entities[2]{entt::entity{3}, entt::entity{9}};
    base.remove(std::begin(entities), std::end(entities));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 42);
}

TEST(Storage, AggregatesMustWork) {
    struct aggregate_type { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::storage<aggregate_type>{}.emplace(entt::entity{0}, 42);
}

TEST(Storage, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<std::unordered_set<int>> pool;
    pool.emplace(entt::entity{0}).insert(42);
    pool.remove(entt::entity{0});
}

TEST(Storage, Iterator) {
    using iterator = typename entt::storage<boxed_int>::iterator;

    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    iterator end{pool.begin()};
    iterator begin{};
    begin = pool.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.begin());
    ASSERT_EQ(end, pool.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, pool.begin());
    ASSERT_EQ(begin--, pool.end());

    ASSERT_EQ(begin+1, pool.end());
    ASSERT_EQ(end-1, pool.begin());

    ASSERT_EQ(++begin, pool.end());
    ASSERT_EQ(--begin, pool.begin());

    ASSERT_EQ(begin += 1, pool.end());
    ASSERT_EQ(begin -= 1, pool.begin());

    ASSERT_EQ(begin + (end - begin), pool.end());
    ASSERT_EQ(begin - (begin - end), pool.end());

    ASSERT_EQ(end - (end - begin), pool.begin());
    ASSERT_EQ(end + (begin - end), pool.begin());

    ASSERT_EQ(begin[0].value, pool.begin()->value);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.end());
}

TEST(Storage, ConstIterator) {
    using iterator = typename entt::storage<boxed_int>::const_iterator;

    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    iterator cend{pool.cbegin()};
    iterator cbegin{};
    cbegin = pool.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, pool.cbegin());
    ASSERT_EQ(cend, pool.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, pool.cbegin());
    ASSERT_EQ(cbegin--, pool.cend());

    ASSERT_EQ(cbegin+1, pool.cend());
    ASSERT_EQ(cend-1, pool.cbegin());

    ASSERT_EQ(++cbegin, pool.cend());
    ASSERT_EQ(--cbegin, pool.cbegin());

    ASSERT_EQ(cbegin += 1, pool.cend());
    ASSERT_EQ(cbegin -= 1, pool.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.cend());

    ASSERT_EQ(cend - (cend - cbegin), pool.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.cbegin());

    ASSERT_EQ(cbegin[0].value, pool.cbegin()->value);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.cend());
}

TEST(Storage, ReverseIterator) {
    using reverse_iterator = typename entt::storage<boxed_int>::reverse_iterator;

    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    reverse_iterator end{pool.rbegin()};
    reverse_iterator begin{};
    begin = pool.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.rbegin());
    ASSERT_EQ(end, pool.rend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, pool.rbegin());
    ASSERT_EQ(begin--, pool.rend());

    ASSERT_EQ(begin+1, pool.rend());
    ASSERT_EQ(end-1, pool.rbegin());

    ASSERT_EQ(++begin, pool.rend());
    ASSERT_EQ(--begin, pool.rbegin());

    ASSERT_EQ(begin += 1, pool.rend());
    ASSERT_EQ(begin -= 1, pool.rbegin());

    ASSERT_EQ(begin + (end - begin), pool.rend());
    ASSERT_EQ(begin - (begin - end), pool.rend());

    ASSERT_EQ(end - (end - begin), pool.rbegin());
    ASSERT_EQ(end + (begin - end), pool.rbegin());

    ASSERT_EQ(begin[0].value, pool.rbegin()->value);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.rbegin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.rend());
}

TEST(Storage, ConstReverseIterator) {
    using const_reverse_iterator = typename entt::storage<boxed_int>::const_reverse_iterator;

    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    const_reverse_iterator cend{pool.crbegin()};
    const_reverse_iterator cbegin{};
    cbegin = pool.crend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, pool.crbegin());
    ASSERT_EQ(cend, pool.crend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, pool.crbegin());
    ASSERT_EQ(cbegin--, pool.crend());

    ASSERT_EQ(cbegin+1, pool.crend());
    ASSERT_EQ(cend-1, pool.crbegin());

    ASSERT_EQ(++cbegin, pool.crend());
    ASSERT_EQ(--cbegin, pool.crbegin());

    ASSERT_EQ(cbegin += 1, pool.crend());
    ASSERT_EQ(cbegin -= 1, pool.crbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.crend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.crend());

    ASSERT_EQ(cend - (cend - cbegin), pool.crbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.crbegin());

    ASSERT_EQ(cbegin[0].value, pool.crbegin()->value);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.crbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.crend());
}

TEST(Storage, Raw) {
    entt::storage<int> pool;

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{12}, 6);
    pool.emplace(entt::entity{42}, 9);

    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{12}), 6);
    ASSERT_EQ(pool.get(entt::entity{42}), 9);

    ASSERT_EQ(pool.raw()[0u], 3);
    ASSERT_EQ(std::as_const(pool).raw()[1u], 6);
    ASSERT_EQ(pool.raw()[2u], 9);
}

TEST(Storage, SortOrdered) {
    entt::storage<boxed_int> pool;
    entt::entity entities[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{12}, {9}, {6}, {3}, {1}};

    pool.insert(std::begin(entities), std::end(entities), std::begin(values), std::end(values));
    pool.sort([](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    ASSERT_TRUE(std::equal(std::rbegin(entities), std::rend(entities), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(values), std::rend(values), pool.begin(), pool.end()));
}

TEST(Storage, SortReverse) {
    entt::storage<boxed_int> pool;
    entt::entity entities[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{1}, {3}, {6}, {9}, {12}};

    pool.insert(std::begin(entities), std::end(entities), std::begin(values), std::end(values));
    pool.sort([](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    ASSERT_TRUE(std::equal(std::begin(entities), std::end(entities), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::begin(values), std::end(values), pool.begin(), pool.end()));
}

TEST(Storage, SortUnordered) {
    entt::storage<boxed_int> pool;
    entt::entity entities[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{6}, {3}, {1}, {9}, {12}};

    pool.insert(std::begin(entities), std::end(entities), std::begin(values), std::end(values));
    pool.sort([](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), boxed_int{1});
    ASSERT_EQ(*(begin++), boxed_int{3});
    ASSERT_EQ(*(begin++), boxed_int{6});
    ASSERT_EQ(*(begin++), boxed_int{9});
    ASSERT_EQ(*(begin++), boxed_int{12});
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entt::entity{9});
    ASSERT_EQ(pool.data()[1u], entt::entity{3});
    ASSERT_EQ(pool.data()[2u], entt::entity{12});
    ASSERT_EQ(pool.data()[3u], entt::entity{42});
    ASSERT_EQ(pool.data()[4u], entt::entity{7});
}

TEST(Storage, SortRange) {
    entt::storage<boxed_int> pool;
    entt::entity entities[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{3}, {6}, {1}, {9}, {12}};

    pool.insert(std::begin(entities), std::end(entities), std::begin(values), std::end(values));
    pool.sort_n(0u, [](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    ASSERT_TRUE(std::equal(std::rbegin(entities), std::rend(entities), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(values), std::rend(values), pool.begin(), pool.end()));

    pool.sort_n(2u, [](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    ASSERT_EQ(pool.raw()[0u], boxed_int{6});
    ASSERT_EQ(pool.raw()[1u], boxed_int{3});
    ASSERT_EQ(pool.raw()[2u], boxed_int{1});

    ASSERT_EQ(pool.data()[0u], entt::entity{42});
    ASSERT_EQ(pool.data()[1u], entt::entity{12});
    ASSERT_EQ(pool.data()[2u], entt::entity{7});

    pool.sort_n(5u, [](auto lhs, auto rhs) { return lhs.value < rhs.value; });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), boxed_int{1});
    ASSERT_EQ(*(begin++), boxed_int{3});
    ASSERT_EQ(*(begin++), boxed_int{6});
    ASSERT_EQ(*(begin++), boxed_int{9});
    ASSERT_EQ(*(begin++), boxed_int{12});
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entt::entity{9});
    ASSERT_EQ(pool.data()[1u], entt::entity{3});
    ASSERT_EQ(pool.data()[2u], entt::entity{42});
    ASSERT_EQ(pool.data()[3u], entt::entity{12});
    ASSERT_EQ(pool.data()[4u], entt::entity{7});
}

TEST(Storage, RespectDisjoint) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entities[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    int lhs_values[3u]{3, 6, 9};
    lhs.insert(std::begin(lhs_entities), std::end(lhs_entities), std::begin(lhs_values), std::end(lhs_values));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    lhs.respect(rhs);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));
}

TEST(Storage, RespectOverlap) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entities[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    int lhs_values[3u]{3, 6, 9};
    lhs.insert(std::begin(lhs_entities), std::end(lhs_entities), std::begin(lhs_values), std::end(lhs_values));

    entt::entity rhs_entities[1u]{entt::entity{12}};
    int rhs_values[1u]{6};
    rhs.insert(std::begin(rhs_entities), std::end(rhs_entities), std::begin(rhs_values), std::end(rhs_values));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entities), std::rend(rhs_entities), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    lhs.respect(rhs);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(lhs.data()[0u], entt::entity{3});
    ASSERT_EQ(lhs.data()[1u], entt::entity{42});
    ASSERT_EQ(lhs.data()[2u], entt::entity{12});
}

TEST(Storage, RespectOrdered) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entities[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entities), std::end(lhs_entities), std::begin(lhs_values), std::end(lhs_values));

    entt::entity rhs_entities[6u]{entt::entity{6}, entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int rhs_values[6u]{6, 1, 2, 3, 4, 5};
    rhs.insert(std::begin(rhs_entities), std::end(rhs_entities), std::begin(rhs_values), std::end(rhs_values));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entities), std::rend(rhs_entities), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.respect(lhs);

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entities), std::rend(rhs_entities), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));
}

TEST(Storage, RespectReverse) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entities[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entities), std::end(lhs_entities), std::begin(lhs_values), std::end(lhs_values));

    entt::entity rhs_entities[6u]{entt::entity{5}, entt::entity{4}, entt::entity{3}, entt::entity{2}, entt::entity{1}, entt::entity{6}};
    int rhs_values[6u]{5, 4, 3, 2, 1, 6};
    rhs.insert(std::begin(rhs_entities), std::end(rhs_entities), std::begin(rhs_values), std::end(rhs_values));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entities), std::rend(rhs_entities), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.respect(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), 5);
    ASSERT_EQ(*(begin++), 4);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(*(begin++), 2);
    ASSERT_EQ(*(begin++), 1);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], entt::entity{6});
    ASSERT_EQ(rhs.data()[1u], entt::entity{1});
    ASSERT_EQ(rhs.data()[2u], entt::entity{2});
    ASSERT_EQ(rhs.data()[3u], entt::entity{3});
    ASSERT_EQ(rhs.data()[4u], entt::entity{4});
    ASSERT_EQ(rhs.data()[5u], entt::entity{5});
}

TEST(Storage, RespectUnordered) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entities[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entities), std::end(lhs_entities), std::begin(lhs_values), std::end(lhs_values));

    entt::entity rhs_entities[6u]{entt::entity{3}, entt::entity{2}, entt::entity{6}, entt::entity{1}, entt::entity{4}, entt::entity{5}};
    int rhs_values[6u]{3, 2, 6, 1, 4, 5};
    rhs.insert(std::begin(rhs_entities), std::end(rhs_entities), std::begin(rhs_values), std::end(rhs_values));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entities), std::rend(lhs_entities), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entities), std::rend(rhs_entities), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.respect(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), 5);
    ASSERT_EQ(*(begin++), 4);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(*(begin++), 2);
    ASSERT_EQ(*(begin++), 1);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], entt::entity{6});
    ASSERT_EQ(rhs.data()[1u], entt::entity{1});
    ASSERT_EQ(rhs.data()[2u], entt::entity{2});
    ASSERT_EQ(rhs.data()[3u], entt::entity{3});
    ASSERT_EQ(rhs.data()[4u], entt::entity{4});
    ASSERT_EQ(rhs.data()[5u], entt::entity{5});
}

TEST(Storage, CanModifyDuringIteration) {
    entt::storage<int> pool;
    pool.emplace(entt::entity{0}, 42);

    ASSERT_EQ(pool.capacity(), 1u);

    const auto it = pool.cbegin();
    pool.reserve(2u);

    ASSERT_EQ(pool.capacity(), 2u);

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}

TEST(Storage, ReferencesGuaranteed) {
    entt::storage<boxed_int> pool;

    pool.emplace(entt::entity{0}, 0);
    pool.emplace(entt::entity{1}, 1);

    ASSERT_EQ(pool.get(entt::entity{0}).value, 0);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 1);

    for(auto &&type: pool) {
        if(type.value) {
            type.value = 42;
        }
    }

    ASSERT_EQ(pool.get(entt::entity{0}).value, 0);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 42);

    auto begin = pool.begin();

    while(begin != pool.end()) {
        (begin++)->value = 3;
    }

    ASSERT_EQ(pool.get(entt::entity{0}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 3);
}

TEST(Storage, MoveOnlyComponent) {
    // the purpose is to ensure that move only components are always accepted
    entt::storage<std::unique_ptr<int>> pool;
    (void)pool;
}

TEST(Storage, ConstructorExceptionDoesNotAddToStorage) {
    entt::storage<throwing_component> pool;

    try {
        pool.emplace(entt::entity{0});
    } catch (const throwing_component::constructor_exception &) {
        ASSERT_TRUE(pool.empty());
    }

    ASSERT_TRUE(pool.empty());
}
