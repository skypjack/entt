#include <cstdint>
#include <utility>
#include <iterator>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/sparse_set.hpp>

struct empty_type {};
struct boxed_int { int value; };

TEST(SparseSet, Functionalities) {
    entt::sparse_set<std::uint64_t> set;

    set.reserve(42);

    ASSERT_EQ(set.capacity(), 42);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_EQ(set.get(42), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_TRUE(set.has(42));
    ASSERT_EQ(set.get(42), 0u);

    set.destroy(42);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.get(42), 0u);

    ASSERT_TRUE(std::is_move_constructible_v<decltype(set)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(set)>);

    entt::sparse_set<std::uint64_t> cpy{set};
    set = cpy;

    ASSERT_FALSE(set.empty());
    ASSERT_FALSE(cpy.empty());
    ASSERT_EQ(set.get(42), 0u);
    ASSERT_EQ(cpy.get(42), 0u);

    entt::sparse_set<std::uint64_t> other{std::move(set)};

    set = std::move(other);
    other = std::move(set);

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(other.get(42), 0u);

    other.reset();

    ASSERT_TRUE(other.empty());
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(std::as_const(other).begin(), std::as_const(other).end());
    ASSERT_EQ(other.begin(), other.end());
    ASSERT_FALSE(other.has(0));
    ASSERT_FALSE(other.has(42));
}

TEST(SparseSet, Pagination) {
    entt::sparse_set<std::uint64_t> set;
    constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(std::uint64_t);

    ASSERT_EQ(set.extent(), 0);

    set.construct(entt_per_page-1);

    ASSERT_EQ(set.extent(), entt_per_page);
    ASSERT_TRUE(set.has(entt_per_page-1));

    set.construct(entt_per_page);

    ASSERT_EQ(set.extent(), 2 * entt_per_page);
    ASSERT_TRUE(set.has(entt_per_page-1));
    ASSERT_TRUE(set.has(entt_per_page));
    ASSERT_FALSE(set.has(entt_per_page+1));

    set.destroy(entt_per_page-1);

    ASSERT_EQ(set.extent(), 2 * entt_per_page);
    ASSERT_FALSE(set.has(entt_per_page-1));
    ASSERT_TRUE(set.has(entt_per_page));

    set.shrink_to_fit();
    set.destroy(entt_per_page);

    ASSERT_EQ(set.extent(), 2 * entt_per_page);
    ASSERT_FALSE(set.has(entt_per_page-1));
    ASSERT_FALSE(set.has(entt_per_page));

    set.shrink_to_fit();

    ASSERT_EQ(set.extent(), 0);
}

TEST(SparseSet, BatchAdd) {
    entt::sparse_set<std::uint64_t> set;
    entt::sparse_set<std::uint64_t>::entity_type entities[2];

    entities[0] = 3;
    entities[1] = 42;

    set.construct(12);
    set.batch(std::begin(entities), std::end(entities));
    set.construct(24);

    ASSERT_TRUE(set.has(entities[0]));
    ASSERT_TRUE(set.has(entities[1]));
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(9));
    ASSERT_TRUE(set.has(12));
    ASSERT_TRUE(set.has(24));

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set.get(12), 0u);
    ASSERT_EQ(set.get(entities[0]), 1u);
    ASSERT_EQ(set.get(entities[1]), 2u);
    ASSERT_EQ(set.get(24), 3u);
}

TEST(SparseSet, Iterator) {
    using iterator_type = typename entt::sparse_set<std::uint64_t>::iterator_type;

    entt::sparse_set<std::uint64_t> set;
    set.construct(3);

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

    ASSERT_EQ(begin[0], *set.begin());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.end());

    ASSERT_EQ(*begin, 3);
    ASSERT_EQ(*begin.operator->(), 3);
}

TEST(SparseSet, Find) {
    entt::sparse_set<std::uint64_t> set;
    set.construct(3);
    set.construct(42);
    set.construct(99);

    ASSERT_NE(set.find(3), set.end());
    ASSERT_NE(set.find(42), set.end());
    ASSERT_NE(set.find(99), set.end());
    ASSERT_EQ(set.find(0), set.end());

    auto it = set.find(99);

    ASSERT_EQ(*it, 99);
    ASSERT_EQ(*(++it), 42);
    ASSERT_EQ(*(++it), 3);
    ASSERT_EQ(++it, set.end());
    ASSERT_EQ(++set.find(3), set.end());
}

TEST(SparseSet, Data) {
    entt::sparse_set<std::uint64_t> set;

    set.construct(3);
    set.construct(12);
    set.construct(42);

    ASSERT_EQ(set.get(3), 0u);
    ASSERT_EQ(set.get(12), 1u);
    ASSERT_EQ(set.get(42), 2u);

    ASSERT_EQ(*(set.data() + 0u), 3u);
    ASSERT_EQ(*(set.data() + 1u), 12u);
    ASSERT_EQ(*(set.data() + 2u), 42u);
}

TEST(SparseSet, RespectDisjoint) {
    entt::sparse_set<std::uint64_t> lhs;
    entt::sparse_set<std::uint64_t> rhs;

    lhs.construct(3);
    lhs.construct(12);
    lhs.construct(42);

    ASSERT_EQ(lhs.get(3), 0u);
    ASSERT_EQ(lhs.get(12), 1u);
    ASSERT_EQ(lhs.get(42), 2u);

    lhs.respect(rhs);

    ASSERT_EQ(std::as_const(lhs).get(3), 0u);
    ASSERT_EQ(std::as_const(lhs).get(12), 1u);
    ASSERT_EQ(std::as_const(lhs).get(42), 2u);
}

TEST(SparseSet, RespectOverlap) {
    entt::sparse_set<std::uint64_t> lhs;
    entt::sparse_set<std::uint64_t> rhs;

    lhs.construct(3);
    lhs.construct(12);
    lhs.construct(42);

    rhs.construct(12);

    ASSERT_EQ(lhs.get(3), 0u);
    ASSERT_EQ(lhs.get(12), 1u);
    ASSERT_EQ(lhs.get(42), 2u);

    lhs.respect(rhs);

    ASSERT_EQ(std::as_const(lhs).get(3), 0u);
    ASSERT_EQ(std::as_const(lhs).get(12), 2u);
    ASSERT_EQ(std::as_const(lhs).get(42), 1u);
}

TEST(SparseSet, RespectOrdered) {
    entt::sparse_set<std::uint64_t> lhs;
    entt::sparse_set<std::uint64_t> rhs;

    lhs.construct(1);
    lhs.construct(2);
    lhs.construct(3);
    lhs.construct(4);
    lhs.construct(5);

    ASSERT_EQ(lhs.get(1), 0u);
    ASSERT_EQ(lhs.get(2), 1u);
    ASSERT_EQ(lhs.get(3), 2u);
    ASSERT_EQ(lhs.get(4), 3u);
    ASSERT_EQ(lhs.get(5), 4u);

    rhs.construct(6);
    rhs.construct(1);
    rhs.construct(2);
    rhs.construct(3);
    rhs.construct(4);
    rhs.construct(5);

    ASSERT_EQ(rhs.get(6), 0u);
    ASSERT_EQ(rhs.get(1), 1u);
    ASSERT_EQ(rhs.get(2), 2u);
    ASSERT_EQ(rhs.get(3), 3u);
    ASSERT_EQ(rhs.get(4), 4u);
    ASSERT_EQ(rhs.get(5), 5u);

    rhs.respect(lhs);

    ASSERT_EQ(rhs.get(6), 0u);
    ASSERT_EQ(rhs.get(1), 1u);
    ASSERT_EQ(rhs.get(2), 2u);
    ASSERT_EQ(rhs.get(3), 3u);
    ASSERT_EQ(rhs.get(4), 4u);
    ASSERT_EQ(rhs.get(5), 5u);
}

TEST(SparseSet, RespectReverse) {
    entt::sparse_set<std::uint64_t> lhs;
    entt::sparse_set<std::uint64_t> rhs;

    lhs.construct(1);
    lhs.construct(2);
    lhs.construct(3);
    lhs.construct(4);
    lhs.construct(5);

    ASSERT_EQ(lhs.get(1), 0u);
    ASSERT_EQ(lhs.get(2), 1u);
    ASSERT_EQ(lhs.get(3), 2u);
    ASSERT_EQ(lhs.get(4), 3u);
    ASSERT_EQ(lhs.get(5), 4u);

    rhs.construct(5);
    rhs.construct(4);
    rhs.construct(3);
    rhs.construct(2);
    rhs.construct(1);
    rhs.construct(6);

    ASSERT_EQ(rhs.get(5), 0u);
    ASSERT_EQ(rhs.get(4), 1u);
    ASSERT_EQ(rhs.get(3), 2u);
    ASSERT_EQ(rhs.get(2), 3u);
    ASSERT_EQ(rhs.get(1), 4u);
    ASSERT_EQ(rhs.get(6), 5u);

    rhs.respect(lhs);

    ASSERT_EQ(rhs.get(6), 0u);
    ASSERT_EQ(rhs.get(1), 1u);
    ASSERT_EQ(rhs.get(2), 2u);
    ASSERT_EQ(rhs.get(3), 3u);
    ASSERT_EQ(rhs.get(4), 4u);
    ASSERT_EQ(rhs.get(5), 5u);
}

TEST(SparseSet, RespectUnordered) {
    entt::sparse_set<std::uint64_t> lhs;
    entt::sparse_set<std::uint64_t> rhs;

    lhs.construct(1);
    lhs.construct(2);
    lhs.construct(3);
    lhs.construct(4);
    lhs.construct(5);

    ASSERT_EQ(lhs.get(1), 0u);
    ASSERT_EQ(lhs.get(2), 1u);
    ASSERT_EQ(lhs.get(3), 2u);
    ASSERT_EQ(lhs.get(4), 3u);
    ASSERT_EQ(lhs.get(5), 4u);

    rhs.construct(3);
    rhs.construct(2);
    rhs.construct(6);
    rhs.construct(1);
    rhs.construct(4);
    rhs.construct(5);

    ASSERT_EQ(rhs.get(3), 0u);
    ASSERT_EQ(rhs.get(2), 1u);
    ASSERT_EQ(rhs.get(6), 2u);
    ASSERT_EQ(rhs.get(1), 3u);
    ASSERT_EQ(rhs.get(4), 4u);
    ASSERT_EQ(rhs.get(5), 5u);

    rhs.respect(lhs);

    ASSERT_EQ(rhs.get(6), 0u);
    ASSERT_EQ(rhs.get(1), 1u);
    ASSERT_EQ(rhs.get(2), 2u);
    ASSERT_EQ(rhs.get(3), 3u);
    ASSERT_EQ(rhs.get(4), 4u);
    ASSERT_EQ(rhs.get(5), 5u);
}

TEST(SparseSet, CanModifyDuringIteration) {
    entt::sparse_set<std::uint64_t> set;
    set.construct(0);

    ASSERT_EQ(set.capacity(), entt::sparse_set<std::uint64_t>::size_type{1});

    const auto it = set.begin();
    set.reserve(entt::sparse_set<std::uint64_t>::size_type{2});

    ASSERT_EQ(set.capacity(), entt::sparse_set<std::uint64_t>::size_type{2});

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}
