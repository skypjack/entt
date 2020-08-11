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

struct throwing_component {
    struct constructor_exception: std::exception {};

    [[noreturn]] throwing_component() { throw constructor_exception{}; }

    // necessary to disable the empty type optimization
    int data;
};

TEST(Storage, Functionalities) {
    entt::storage<entt::entity, int> pool;

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
    ASSERT_EQ(*pool.try_get(entt::entity{41}), 3);
    ASSERT_EQ(pool.try_get(entt::entity{99}), nullptr);

    pool.erase(entt::entity{41});

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    pool.emplace(entt::entity{41}, 12);

    ASSERT_EQ(pool.get(entt::entity{41}), 12);
    ASSERT_EQ(*pool.try_get(entt::entity{41}), 12);
    ASSERT_EQ(pool.try_get(entt::entity{99}), nullptr);

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

    (void)entt::storage<entt::entity, int>{std::move(pool)};
    entt::storage<entt::entity, int> other;
    other = std::move(pool);
}

TEST(Storage, EmptyType) {
    entt::storage<entt::entity, empty_type> pool;
    pool.emplace(entt::entity{99});

    ASSERT_TRUE(pool.contains(entt::entity{99}));
}

TEST(Storage, BatchAdd) {
    entt::storage<entt::entity, int> pool;
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

TEST(Storage, BatchAddEmptyType) {
    entt::storage<entt::entity, empty_type> pool;
    entt::entity entities[2];

    entities[0] = entt::entity{3};
    entities[1] = entt::entity{42};

    pool.insert(std::begin(entities), std::end(entities));

    ASSERT_TRUE(pool.contains(entities[0]));
    ASSERT_TRUE(pool.contains(entities[1]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
}

TEST(Storage, AggregatesMustWork) {
    struct aggregate_type { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::storage<entt::entity, aggregate_type>{}.emplace(entt::entity{0}, 42);
}

TEST(Storage, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<entt::entity, std::unordered_set<int>> pool;
    pool.emplace(entt::entity{0});
    pool.get(entt::entity{0}).insert(42);
    pool.erase(entt::entity{0});
}

TEST(Storage, Iterator) {
    using iterator = typename entt::storage<entt::entity, boxed_int>::iterator;

    entt::storage<entt::entity, boxed_int> pool;
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
    using iterator = typename entt::storage<entt::entity, boxed_int>::const_iterator;

    entt::storage<entt::entity, boxed_int> pool;
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
    using reverse_iterator = typename entt::storage<entt::entity, boxed_int>::reverse_iterator;

    entt::storage<entt::entity, boxed_int> pool;
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
    using const_reverse_iterator = typename entt::storage<entt::entity, boxed_int>::const_reverse_iterator;

    entt::storage<entt::entity, boxed_int> pool;
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
    entt::storage<entt::entity, int> pool;

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{12}, 6);
    pool.emplace(entt::entity{42}, 9);

    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{12}), 6);
    ASSERT_EQ(pool.get(entt::entity{42}), 9);

    ASSERT_EQ(*(pool.raw() + 0u), 3);
    ASSERT_EQ(*(std::as_const(pool).raw() + 1u), 6);
    ASSERT_EQ(*(pool.raw() + 2u), 9);
}

TEST(Storage, SortOrdered) {
    entt::storage<entt::entity, boxed_int> pool;

    pool.emplace(entt::entity{12}, boxed_int{12});
    pool.emplace(entt::entity{42}, boxed_int{9});
    pool.emplace(entt::entity{7}, boxed_int{6});
    pool.emplace(entt::entity{3}, boxed_int{3});
    pool.emplace(entt::entity{9}, boxed_int{1});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 12);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 1);

    pool.sort(pool.begin(), pool.end(), [](auto lhs, auto rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(pool.index(entt::entity{12}), 0u);
    ASSERT_EQ(pool.index(entt::entity{42}), 1u);
    ASSERT_EQ(pool.index(entt::entity{7}), 2u);
    ASSERT_EQ(pool.index(entt::entity{3}), 3u);
    ASSERT_EQ(pool.index(entt::entity{9}), 4u);

    ASSERT_EQ((pool.raw() + 0u)->value, 12);
    ASSERT_EQ((pool.raw() + 1u)->value, 9);
    ASSERT_EQ((pool.raw() + 2u)->value, 6);
    ASSERT_EQ((pool.raw() + 3u)->value, 3);
    ASSERT_EQ((pool.raw() + 4u)->value, 1);

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, SortReverse) {
    entt::storage<entt::entity, boxed_int> pool;

    pool.emplace(entt::entity{12}, boxed_int{1});
    pool.emplace(entt::entity{42}, boxed_int{3});
    pool.emplace(entt::entity{7}, boxed_int{6});
    pool.emplace(entt::entity{3}, boxed_int{9});
    pool.emplace(entt::entity{9}, boxed_int{12});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(pool.begin(), pool.end(), [&pool](entt::entity lhs, entt::entity rhs) {
        return pool.get(lhs).value < pool.get(rhs).value;
    });

    ASSERT_EQ(pool.index(entt::entity{9}), 0u);
    ASSERT_EQ(pool.index(entt::entity{3}), 1u);
    ASSERT_EQ(pool.index(entt::entity{7}), 2u);
    ASSERT_EQ(pool.index(entt::entity{42}), 3u);
    ASSERT_EQ(pool.index(entt::entity{12}), 4u);

    ASSERT_EQ((pool.raw() + 0u)->value, 12);
    ASSERT_EQ((pool.raw() + 1u)->value, 9);
    ASSERT_EQ((pool.raw() + 2u)->value, 6);
    ASSERT_EQ((pool.raw() + 3u)->value, 3);
    ASSERT_EQ((pool.raw() + 4u)->value, 1);

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, SortUnordered) {
    entt::storage<entt::entity, boxed_int> pool;

    pool.emplace(entt::entity{12}, boxed_int{6});
    pool.emplace(entt::entity{42}, boxed_int{3});
    pool.emplace(entt::entity{7}, boxed_int{1});
    pool.emplace(entt::entity{3}, boxed_int{9});
    pool.emplace(entt::entity{9}, boxed_int{12});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(pool.begin(), pool.end(), [](auto lhs, auto rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(pool.index(entt::entity{9}), 0u);
    ASSERT_EQ(pool.index(entt::entity{3}), 1u);
    ASSERT_EQ(pool.index(entt::entity{12}), 2u);
    ASSERT_EQ(pool.index(entt::entity{42}), 3u);
    ASSERT_EQ(pool.index(entt::entity{7}), 4u);

    ASSERT_EQ((pool.raw() + 0u)->value, 12);
    ASSERT_EQ((pool.raw() + 1u)->value, 9);
    ASSERT_EQ((pool.raw() + 2u)->value, 6);
    ASSERT_EQ((pool.raw() + 3u)->value, 3);
    ASSERT_EQ((pool.raw() + 4u)->value, 1);

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, SortRange) {
    entt::storage<entt::entity, boxed_int> pool;

    pool.emplace(entt::entity{12}, boxed_int{6});
    pool.emplace(entt::entity{42}, boxed_int{3});
    pool.emplace(entt::entity{7}, boxed_int{1});
    pool.emplace(entt::entity{3}, boxed_int{9});
    pool.emplace(entt::entity{9}, boxed_int{12});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(pool.end(), pool.end(), std::less{});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(pool.begin(), pool.begin(), std::less{});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(pool.begin()+2, pool.begin()+3, std::less{});

    ASSERT_EQ(pool.get(entt::entity{12}).value, 6);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{7}).value, 1);
    ASSERT_EQ(pool.get(entt::entity{3}).value, 9);
    ASSERT_EQ(pool.get(entt::entity{9}).value, 12);

    pool.sort(++pool.begin(), --pool.end(), [](auto lhs, auto rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(pool.index(entt::entity{12}), 0u);
    ASSERT_EQ(pool.index(entt::entity{3}), 1u);
    ASSERT_EQ(pool.index(entt::entity{42}), 2u);
    ASSERT_EQ(pool.index(entt::entity{7}), 3u);
    ASSERT_EQ(pool.index(entt::entity{9}), 4u);

    ASSERT_EQ((pool.raw() + 0u)->value, 6);
    ASSERT_EQ((pool.raw() + 1u)->value, 9);
    ASSERT_EQ((pool.raw() + 2u)->value, 3);
    ASSERT_EQ((pool.raw() + 3u)->value, 1);
    ASSERT_EQ((pool.raw() + 4u)->value, 12);

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ(begin, end);
}

TEST(Storage, RespectDisjoint) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.emplace(entt::entity{3}, 3);
    lhs.emplace(entt::entity{12}, 6);
    lhs.emplace(entt::entity{42}, 9);

    ASSERT_EQ(std::as_const(lhs).get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(lhs).get(entt::entity{12}), 6);
    ASSERT_EQ(std::as_const(lhs).get(entt::entity{42}), 9);

    lhs.respect(rhs);

    ASSERT_EQ(*(std::as_const(lhs).raw() + 0u), 3);
    ASSERT_EQ(*(std::as_const(lhs).raw() + 1u), 6);
    ASSERT_EQ(*(std::as_const(lhs).raw() + 2u), 9);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(begin, end);
}

TEST(Storage, RespectOverlap) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.emplace(entt::entity{3}, 3);
    lhs.emplace(entt::entity{12}, 6);
    lhs.emplace(entt::entity{42}, 9);
    rhs.emplace(entt::entity{12}, 6);

    ASSERT_EQ(std::as_const(lhs).get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(lhs).get(entt::entity{12}), 6);
    ASSERT_EQ(std::as_const(lhs).get(entt::entity{42}), 9);
    ASSERT_EQ(rhs.get(entt::entity{12}), 6);

    lhs.respect(rhs);

    ASSERT_EQ(*(std::as_const(lhs).raw() + 0u), 3);
    ASSERT_EQ(*(std::as_const(lhs).raw() + 1u), 9);
    ASSERT_EQ(*(std::as_const(lhs).raw() + 2u), 6);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(begin, end);
}

TEST(Storage, RespectOrdered) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.emplace(entt::entity{1}, 0);
    lhs.emplace(entt::entity{2}, 0);
    lhs.emplace(entt::entity{3}, 0);
    lhs.emplace(entt::entity{4}, 0);
    lhs.emplace(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.emplace(entt::entity{6}, 0);
    rhs.emplace(entt::entity{1}, 0);
    rhs.emplace(entt::entity{2}, 0);
    rhs.emplace(entt::entity{3}, 0);
    rhs.emplace(entt::entity{4}, 0);
    rhs.emplace(entt::entity{5}, 0);

    ASSERT_EQ(rhs.get(entt::entity{6}), 0);
    ASSERT_EQ(rhs.get(entt::entity{1}), 0);
    ASSERT_EQ(rhs.get(entt::entity{2}), 0);
    ASSERT_EQ(rhs.get(entt::entity{3}), 0);
    ASSERT_EQ(rhs.get(entt::entity{4}), 0);
    ASSERT_EQ(rhs.get(entt::entity{5}), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), entt::entity{1});
    ASSERT_EQ(*(lhs.data() + 1u), entt::entity{2});
    ASSERT_EQ(*(lhs.data() + 2u), entt::entity{3});
    ASSERT_EQ(*(lhs.data() + 3u), entt::entity{4});
    ASSERT_EQ(*(lhs.data() + 4u), entt::entity{5});

    ASSERT_EQ(*(rhs.data() + 0u), entt::entity{6});
    ASSERT_EQ(*(rhs.data() + 1u), entt::entity{1});
    ASSERT_EQ(*(rhs.data() + 2u), entt::entity{2});
    ASSERT_EQ(*(rhs.data() + 3u), entt::entity{3});
    ASSERT_EQ(*(rhs.data() + 4u), entt::entity{4});
    ASSERT_EQ(*(rhs.data() + 5u), entt::entity{5});
}

TEST(Storage, RespectReverse) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.emplace(entt::entity{1}, 0);
    lhs.emplace(entt::entity{2}, 0);
    lhs.emplace(entt::entity{3}, 0);
    lhs.emplace(entt::entity{4}, 0);
    lhs.emplace(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.emplace(entt::entity{5}, 0);
    rhs.emplace(entt::entity{4}, 0);
    rhs.emplace(entt::entity{3}, 0);
    rhs.emplace(entt::entity{2}, 0);
    rhs.emplace(entt::entity{1}, 0);
    rhs.emplace(entt::entity{6}, 0);

    ASSERT_EQ(rhs.get(entt::entity{5}), 0);
    ASSERT_EQ(rhs.get(entt::entity{4}), 0);
    ASSERT_EQ(rhs.get(entt::entity{3}), 0);
    ASSERT_EQ(rhs.get(entt::entity{2}), 0);
    ASSERT_EQ(rhs.get(entt::entity{1}), 0);
    ASSERT_EQ(rhs.get(entt::entity{6}), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), entt::entity{1});
    ASSERT_EQ(*(lhs.data() + 1u), entt::entity{2});
    ASSERT_EQ(*(lhs.data() + 2u), entt::entity{3});
    ASSERT_EQ(*(lhs.data() + 3u), entt::entity{4});
    ASSERT_EQ(*(lhs.data() + 4u), entt::entity{5});

    ASSERT_EQ(*(rhs.data() + 0u), entt::entity{6});
    ASSERT_EQ(*(rhs.data() + 1u), entt::entity{1});
    ASSERT_EQ(*(rhs.data() + 2u), entt::entity{2});
    ASSERT_EQ(*(rhs.data() + 3u), entt::entity{3});
    ASSERT_EQ(*(rhs.data() + 4u), entt::entity{4});
    ASSERT_EQ(*(rhs.data() + 5u), entt::entity{5});
}

TEST(Storage, RespectUnordered) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.emplace(entt::entity{1}, 0);
    lhs.emplace(entt::entity{2}, 0);
    lhs.emplace(entt::entity{3}, 0);
    lhs.emplace(entt::entity{4}, 0);
    lhs.emplace(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.emplace(entt::entity{3}, 0);
    rhs.emplace(entt::entity{2}, 0);
    rhs.emplace(entt::entity{6}, 0);
    rhs.emplace(entt::entity{1}, 0);
    rhs.emplace(entt::entity{4}, 0);
    rhs.emplace(entt::entity{5}, 0);

    ASSERT_EQ(rhs.get(entt::entity{3}), 0);
    ASSERT_EQ(rhs.get(entt::entity{2}), 0);
    ASSERT_EQ(rhs.get(entt::entity{6}), 0);
    ASSERT_EQ(rhs.get(entt::entity{1}), 0);
    ASSERT_EQ(rhs.get(entt::entity{4}), 0);
    ASSERT_EQ(rhs.get(entt::entity{5}), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), entt::entity{1});
    ASSERT_EQ(*(lhs.data() + 1u), entt::entity{2});
    ASSERT_EQ(*(lhs.data() + 2u), entt::entity{3});
    ASSERT_EQ(*(lhs.data() + 3u), entt::entity{4});
    ASSERT_EQ(*(lhs.data() + 4u), entt::entity{5});

    ASSERT_EQ(*(rhs.data() + 0u), entt::entity{6});
    ASSERT_EQ(*(rhs.data() + 1u), entt::entity{1});
    ASSERT_EQ(*(rhs.data() + 2u), entt::entity{2});
    ASSERT_EQ(*(rhs.data() + 3u), entt::entity{3});
    ASSERT_EQ(*(rhs.data() + 4u), entt::entity{4});
    ASSERT_EQ(*(rhs.data() + 5u), entt::entity{5});
}

TEST(Storage, CanModifyDuringIteration) {
    entt::storage<entt::entity, int> pool;
    pool.emplace(entt::entity{0}, 42);

    ASSERT_EQ(pool.capacity(), (entt::storage<entt::entity, int>::size_type{1}));

    const auto it = pool.cbegin();
    pool.reserve(entt::storage<entt::entity, int>::size_type{2});

    ASSERT_EQ(pool.capacity(), (entt::storage<entt::entity, int>::size_type{2}));

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}

TEST(Storage, ReferencesGuaranteed) {
    entt::storage<entt::entity, boxed_int> pool;

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
    entt::storage<entt::entity, std::unique_ptr<int>> pool;
    (void)pool;
}

TEST(Storage, ConstructorExceptionDoesNotAddToStorage) {
    entt::storage<entt::entity, throwing_component> pool;

    try {
        pool.emplace(entt::entity{0});
    } catch (const throwing_component::constructor_exception &) {
        ASSERT_TRUE(pool.empty());
    }

    ASSERT_TRUE(pool.empty());
}
