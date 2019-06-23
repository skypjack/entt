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

    // necessary to avoid the short-circuit construct() logic for empty objects
    int data;
};

TEST(Storage, Functionalities) {
    entt::storage<entt::entity, int> set;

    set.reserve(42);

    ASSERT_EQ(set.capacity(), 42);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_FALSE(set.has(entt::entity{41}));

    set.construct(entt::entity{41}, 3);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_TRUE(set.has(entt::entity{41}));
    ASSERT_EQ(set.get(entt::entity{41}), 3);
    ASSERT_EQ(*set.try_get(entt::entity{41}), 3);
    ASSERT_EQ(set.try_get(entt::entity{99}), nullptr);

    set.destroy(entt::entity{41});

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_FALSE(set.has(entt::entity{41}));

    set.construct(entt::entity{41}, 12);

    ASSERT_EQ(set.get(entt::entity{41}), 12);
    ASSERT_EQ(*set.try_get(entt::entity{41}), 12);
    ASSERT_EQ(set.try_get(entt::entity{99}), nullptr);

    set.reset();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_FALSE(set.has(entt::entity{41}));

    ASSERT_EQ(set.capacity(), 42);

    set.shrink_to_fit();

    ASSERT_EQ(set.capacity(), 0);

    (void)entt::storage<entt::entity, int>{std::move(set)};
    entt::storage<entt::entity, int> other;
    other = std::move(set);
}

TEST(Storage, EmptyType) {
    entt::storage<entt::entity, empty_type> set;

    set.construct(entt::entity{42});
    set.construct(entt::entity{99});

    ASSERT_TRUE(set.has(entt::entity{42}));
    ASSERT_TRUE(set.has(entt::entity{99}));

    auto &&component = set.get(entt::entity{42});

    ASSERT_TRUE((std::is_same_v<decltype(component), empty_type &&>));
}

TEST(Storage, BatchAdd) {
    entt::storage<entt::entity, int> set;
    entt::storage<entt::entity, int>::entity_type entities[2];

    entities[0] = entt::entity{3};
    entities[1] = entt::entity{42};

    set.reserve(4);
    set.construct(entt::entity{12}, 21);
    auto *component = set.batch(std::begin(entities), std::end(entities));
    set.construct(entt::entity{24}, 42);

    ASSERT_TRUE(set.has(entities[0]));
    ASSERT_TRUE(set.has(entities[1]));
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_FALSE(set.has(entt::entity{9}));
    ASSERT_TRUE(set.has(entt::entity{12}));
    ASSERT_TRUE(set.has(entt::entity{24}));

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set.get(entt::entity{12}), 21);
    ASSERT_EQ(set.get(entities[0]), 0);
    ASSERT_EQ(set.get(entities[1]), 0);
    ASSERT_EQ(set.get(entt::entity{24}), 42);

    component[0] = 1;
    component[1] = 2;

    ASSERT_EQ(set.get(entities[0]), 1);
    ASSERT_EQ(set.get(entities[1]), 2);
}

TEST(Storage, BatchAddEmptyType) {
    entt::storage<entt::entity, empty_type> set;
    entt::storage<entt::entity, empty_type>::entity_type entities[2];

    entities[0] = entt::entity{3};
    entities[1] = entt::entity{42};

    set.reserve(4);
    set.construct(entt::entity{12});
    set.batch(std::begin(entities), std::end(entities));
    set.construct(entt::entity{24});

    ASSERT_TRUE(set.has(entities[0]));
    ASSERT_TRUE(set.has(entities[1]));
    ASSERT_FALSE(set.has(entt::entity{0}));
    ASSERT_FALSE(set.has(entt::entity{9}));
    ASSERT_TRUE(set.has(entt::entity{12}));
    ASSERT_TRUE(set.has(entt::entity{24}));

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4u);

    auto &&component = set.get(entities[0]);

    ASSERT_TRUE((std::is_same_v<decltype(component), empty_type &&>));
}

TEST(Storage, AggregatesMustWork) {
    struct aggregate_type { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::storage<entt::entity, aggregate_type>{}.construct(entt::entity{0}, 42);
}

TEST(Storage, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<entt::entity, std::unordered_set<int>> set;
    set.construct(entt::entity{0}).insert(42);
    set.destroy(entt::entity{0});
}

TEST(Storage, Iterator) {
    using iterator_type = typename entt::storage<entt::entity, boxed_int>::iterator_type;

    entt::storage<entt::entity, boxed_int> set;
    set.construct(entt::entity{3}, 42);

    iterator_type end{set.begin()};
    iterator_type begin{};
    begin = set.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, set.begin());
    ASSERT_EQ(end, set.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, set.begin());
    ASSERT_EQ(begin--, set.end());

    ASSERT_EQ(begin+1, set.end());
    ASSERT_EQ(end-1, set.begin());

    ASSERT_EQ(++begin, set.end());
    ASSERT_EQ(--begin, set.begin());

    ASSERT_EQ(begin += 1, set.end());
    ASSERT_EQ(begin -= 1, set.begin());

    ASSERT_EQ(begin + (end - begin), set.end());
    ASSERT_EQ(begin - (begin - end), set.end());

    ASSERT_EQ(end - (end - begin), set.begin());
    ASSERT_EQ(end + (begin - end), set.begin());

    ASSERT_EQ(begin[0].value, set.begin()->value);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.end());
}

TEST(Storage, ConstIterator) {
    using iterator_type = typename entt::storage<entt::entity, boxed_int>::const_iterator_type;

    entt::storage<entt::entity, boxed_int> set;
    set.construct(entt::entity{3}, 42);

    iterator_type cend{set.cbegin()};
    iterator_type cbegin{};
    cbegin = set.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, set.cbegin());
    ASSERT_EQ(cend, set.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, set.cbegin());
    ASSERT_EQ(cbegin--, set.cend());

    ASSERT_EQ(cbegin+1, set.cend());
    ASSERT_EQ(cend-1, set.cbegin());

    ASSERT_EQ(++cbegin, set.cend());
    ASSERT_EQ(--cbegin, set.cbegin());

    ASSERT_EQ(cbegin += 1, set.cend());
    ASSERT_EQ(cbegin -= 1, set.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), set.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), set.cend());

    ASSERT_EQ(cend - (cend - cbegin), set.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), set.cbegin());

    ASSERT_EQ(cbegin[0].value, set.cbegin()->value);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, set.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, set.cend());
}

TEST(Storage, IteratorEmptyType) {
    using iterator_type = typename entt::storage<entt::entity, empty_type>::iterator_type;
    entt::storage<entt::entity, empty_type> set;
    set.construct(entt::entity{3});

    iterator_type end{set.begin()};
    iterator_type begin{};
    begin = set.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, set.begin());
    ASSERT_EQ(end, set.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, set.begin());
    ASSERT_EQ(begin--, set.end());

    ASSERT_EQ(begin+1, set.end());
    ASSERT_EQ(end-1, set.begin());

    ASSERT_EQ(++begin, set.end());
    ASSERT_EQ(--begin, set.begin());

    ASSERT_EQ(begin += 1, set.end());
    ASSERT_EQ(begin -= 1, set.begin());

    ASSERT_EQ(begin + (end - begin), set.end());
    ASSERT_EQ(begin - (begin - end), set.end());

    ASSERT_EQ(end - (end - begin), set.begin());
    ASSERT_EQ(end + (begin - end), set.begin());

    ASSERT_EQ(set.begin().operator->(), nullptr);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.end());

    set.construct(entt::entity{33});
    auto &&component = *begin;

    ASSERT_TRUE((std::is_same_v<decltype(component), empty_type &&>));
}

TEST(Storage, Raw) {
    entt::storage<entt::entity, int> set;

    set.construct(entt::entity{3}, 3);
    set.construct(entt::entity{12}, 6);
    set.construct(entt::entity{42}, 9);

    ASSERT_EQ(set.get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(set).get(entt::entity{12}), 6);
    ASSERT_EQ(set.get(entt::entity{42}), 9);

    ASSERT_EQ(*(set.raw() + 0u), 3);
    ASSERT_EQ(*(std::as_const(set).raw() + 1u), 6);
    ASSERT_EQ(*(set.raw() + 2u), 9);
}

TEST(Storage, SortOrdered) {
    entt::storage<entt::entity, boxed_int> set;

    set.construct(entt::entity{12}, boxed_int{12});
    set.construct(entt::entity{42}, boxed_int{9});
    set.construct(entt::entity{7}, boxed_int{6});
    set.construct(entt::entity{3}, boxed_int{3});
    set.construct(entt::entity{9}, boxed_int{1});

    ASSERT_EQ(set.get(entt::entity{12}).value, 12);
    ASSERT_EQ(set.get(entt::entity{42}).value, 9);
    ASSERT_EQ(set.get(entt::entity{7}).value, 6);
    ASSERT_EQ(set.get(entt::entity{3}).value, 3);
    ASSERT_EQ(set.get(entt::entity{9}).value, 1);

    set.sort([](auto lhs, auto rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ((set.raw() + 0u)->value, 12);
    ASSERT_EQ((set.raw() + 1u)->value, 9);
    ASSERT_EQ((set.raw() + 2u)->value, 6);
    ASSERT_EQ((set.raw() + 3u)->value, 3);
    ASSERT_EQ((set.raw() + 4u)->value, 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, SortReverse) {
    entt::storage<entt::entity, boxed_int> set;

    set.construct(entt::entity{12}, boxed_int{1});
    set.construct(entt::entity{42}, boxed_int{3});
    set.construct(entt::entity{7}, boxed_int{6});
    set.construct(entt::entity{3}, boxed_int{9});
    set.construct(entt::entity{9}, boxed_int{12});

    ASSERT_EQ(set.get(entt::entity{12}).value, 1);
    ASSERT_EQ(set.get(entt::entity{42}).value, 3);
    ASSERT_EQ(set.get(entt::entity{7}).value, 6);
    ASSERT_EQ(set.get(entt::entity{3}).value, 9);
    ASSERT_EQ(set.get(entt::entity{9}).value, 12);

    set.sort([&set](entt::entity lhs, entt::entity rhs) {
        return set.get(lhs).value < set.get(rhs).value;
    });

    ASSERT_EQ((set.raw() + 0u)->value, 12);
    ASSERT_EQ((set.raw() + 1u)->value, 9);
    ASSERT_EQ((set.raw() + 2u)->value, 6);
    ASSERT_EQ((set.raw() + 3u)->value, 3);
    ASSERT_EQ((set.raw() + 4u)->value, 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, SortUnordered) {
    entt::storage<entt::entity, boxed_int> set;

    set.construct(entt::entity{12}, boxed_int{6});
    set.construct(entt::entity{42}, boxed_int{3});
    set.construct(entt::entity{7}, boxed_int{1});
    set.construct(entt::entity{3}, boxed_int{9});
    set.construct(entt::entity{9}, boxed_int{12});

    ASSERT_EQ(set.get(entt::entity{12}).value, 6);
    ASSERT_EQ(set.get(entt::entity{42}).value, 3);
    ASSERT_EQ(set.get(entt::entity{7}).value, 1);
    ASSERT_EQ(set.get(entt::entity{3}).value, 9);
    ASSERT_EQ(set.get(entt::entity{9}).value, 12);

    set.sort([](auto lhs, auto rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ((set.raw() + 0u)->value, 12);
    ASSERT_EQ((set.raw() + 1u)->value, 9);
    ASSERT_EQ((set.raw() + 2u)->value, 6);
    ASSERT_EQ((set.raw() + 3u)->value, 3);
    ASSERT_EQ((set.raw() + 4u)->value, 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ((begin++)->value, 1);
    ASSERT_EQ((begin++)->value, 3);
    ASSERT_EQ((begin++)->value, 6);
    ASSERT_EQ((begin++)->value, 9);
    ASSERT_EQ((begin++)->value, 12);
    ASSERT_EQ(begin, end);
}

TEST(Storage, RespectDisjoint) {
    entt::storage<entt::entity, int> lhs;
    entt::storage<entt::entity, int> rhs;

    lhs.construct(entt::entity{3}, 3);
    lhs.construct(entt::entity{12}, 6);
    lhs.construct(entt::entity{42}, 9);

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

    lhs.construct(entt::entity{3}, 3);
    lhs.construct(entt::entity{12}, 6);
    lhs.construct(entt::entity{42}, 9);
    rhs.construct(entt::entity{12}, 6);

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

    lhs.construct(entt::entity{1}, 0);
    lhs.construct(entt::entity{2}, 0);
    lhs.construct(entt::entity{3}, 0);
    lhs.construct(entt::entity{4}, 0);
    lhs.construct(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.construct(entt::entity{6}, 0);
    rhs.construct(entt::entity{1}, 0);
    rhs.construct(entt::entity{2}, 0);
    rhs.construct(entt::entity{3}, 0);
    rhs.construct(entt::entity{4}, 0);
    rhs.construct(entt::entity{5}, 0);

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

    lhs.construct(entt::entity{1}, 0);
    lhs.construct(entt::entity{2}, 0);
    lhs.construct(entt::entity{3}, 0);
    lhs.construct(entt::entity{4}, 0);
    lhs.construct(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.construct(entt::entity{5}, 0);
    rhs.construct(entt::entity{4}, 0);
    rhs.construct(entt::entity{3}, 0);
    rhs.construct(entt::entity{2}, 0);
    rhs.construct(entt::entity{1}, 0);
    rhs.construct(entt::entity{6}, 0);

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

    lhs.construct(entt::entity{1}, 0);
    lhs.construct(entt::entity{2}, 0);
    lhs.construct(entt::entity{3}, 0);
    lhs.construct(entt::entity{4}, 0);
    lhs.construct(entt::entity{5}, 0);

    ASSERT_EQ(lhs.get(entt::entity{1}), 0);
    ASSERT_EQ(lhs.get(entt::entity{2}), 0);
    ASSERT_EQ(lhs.get(entt::entity{3}), 0);
    ASSERT_EQ(lhs.get(entt::entity{4}), 0);
    ASSERT_EQ(lhs.get(entt::entity{5}), 0);

    rhs.construct(entt::entity{3}, 0);
    rhs.construct(entt::entity{2}, 0);
    rhs.construct(entt::entity{6}, 0);
    rhs.construct(entt::entity{1}, 0);
    rhs.construct(entt::entity{4}, 0);
    rhs.construct(entt::entity{5}, 0);

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

TEST(Storage, RespectOverlapEmptyType) {
    entt::storage<entt::entity, empty_type> lhs;
    entt::storage<entt::entity, empty_type> rhs;

    lhs.construct(entt::entity{3});
    lhs.construct(entt::entity{12});
    lhs.construct(entt::entity{42});

    rhs.construct(entt::entity{12});

    ASSERT_EQ(lhs.sparse_set<entt::entity>::get(entt::entity{3}), 0u);
    ASSERT_EQ(lhs.sparse_set<entt::entity>::get(entt::entity{12}), 1u);
    ASSERT_EQ(lhs.sparse_set<entt::entity>::get(entt::entity{42}), 2u);

    lhs.respect(rhs);

    ASSERT_EQ(std::as_const(lhs).sparse_set<entt::entity>::get(entt::entity{3}), 0u);
    ASSERT_EQ(std::as_const(lhs).sparse_set<entt::entity>::get(entt::entity{12}), 2u);
    ASSERT_EQ(std::as_const(lhs).sparse_set<entt::entity>::get(entt::entity{42}), 1u);
}

TEST(Storage, CanModifyDuringIteration) {
    entt::storage<entt::entity, int> set;
    set.construct(entt::entity{0}, 42);

    ASSERT_EQ(set.capacity(), (entt::storage<entt::entity, int>::size_type{1}));

    const auto it = set.cbegin();
    set.reserve(entt::storage<entt::entity, int>::size_type{2});

    ASSERT_EQ(set.capacity(), (entt::storage<entt::entity, int>::size_type{2}));

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}

TEST(Storage, ReferencesGuaranteed) {
    entt::storage<entt::entity, boxed_int> set;

    set.construct(entt::entity{0}, 0);
    set.construct(entt::entity{1}, 1);

    ASSERT_EQ(set.get(entt::entity{0}).value, 0);
    ASSERT_EQ(set.get(entt::entity{1}).value, 1);

    for(auto &&type: set) {
        if(type.value) {
            type.value = 42;
        }
    }

    ASSERT_EQ(set.get(entt::entity{0}).value, 0);
    ASSERT_EQ(set.get(entt::entity{1}).value, 42);

    auto begin = set.begin();

    while(begin != set.end()) {
        (begin++)->value = 3;
    }

    ASSERT_EQ(set.get(entt::entity{0}).value, 3);
    ASSERT_EQ(set.get(entt::entity{1}).value, 3);
}

TEST(Storage, MoveOnlyComponent) {
    // the purpose is to ensure that move only components are always accepted
    entt::storage<entt::entity, std::unique_ptr<int>> set;
    (void)set;
}

TEST(Storage, ConstructorExceptionDoesNotAddToSet) {
    entt::storage<entt::entity, throwing_component> set;

    try {
        set.construct(entt::entity{0});
        FAIL() << "Expected constructor_exception to be thrown";
    } catch (const throwing_component::constructor_exception &) {
        ASSERT_TRUE(set.empty());
    }
}
