#include <unordered_set>
#include <gtest/gtest.h>
#include <entt/entity/sparse_set.hpp>

TEST(SparseSetNoType, Functionalities) {
    entt::SparseSet<std::uint64_t> set;
    const auto &cset = set;

    set.reserve(42);

    ASSERT_EQ(set.capacity(), 42);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_EQ(set.get(42), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(cset.begin(), cset.end());
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_TRUE(set.has(42));
    ASSERT_TRUE(set.fast(42));
    ASSERT_EQ(set.get(42), 0u);

    set.destroy(42);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_EQ(set.get(42), 0u);

    set.reset();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    (void)entt::SparseSet<std::uint64_t>{std::move(set)};
    entt::SparseSet<std::uint64_t> other;
    other = std::move(set);
}

TEST(SparseSetNoType, ElementAccess) {
    entt::SparseSet<std::uint64_t> set;
    const auto &cset = set;

    set.construct(42);
    set.construct(3);

    for(typename entt::SparseSet<std::uint64_t>::size_type i{}; i < set.size(); ++i) {
        ASSERT_EQ(set[i], i ? 42 : 3);
        ASSERT_EQ(cset[i], i ? 42 : 3);
    }
}

TEST(SparseSetNoType, Iterator) {
    using iterator_type = typename entt::SparseSet<std::uint64_t>::iterator_type;

    entt::SparseSet<std::uint64_t> set;
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

TEST(SparseSetNoType, ConstIterator) {
    using iterator_type = typename entt::SparseSet<std::uint64_t>::const_iterator_type;

    entt::SparseSet<std::uint64_t> set;
    set.construct(3);

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

    ASSERT_EQ(cbegin[0], *set.cbegin());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, set.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, set.cend());

    ASSERT_EQ(*cbegin, 3);
    ASSERT_EQ(*cbegin.operator->(), 3);
}

TEST(SparseSetNoType, Data) {
    entt::SparseSet<std::uint64_t> set;

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

TEST(SparseSetNoType, RespectDisjoint) {
    entt::SparseSet<std::uint64_t> lhs;
    entt::SparseSet<std::uint64_t> rhs;
    const auto &clhs = lhs;

    lhs.construct(3);
    lhs.construct(12);
    lhs.construct(42);

    ASSERT_EQ(lhs.get(3), 0u);
    ASSERT_EQ(lhs.get(12), 1u);
    ASSERT_EQ(lhs.get(42), 2u);

    lhs.respect(rhs);

    ASSERT_EQ(clhs.get(3), 0u);
    ASSERT_EQ(clhs.get(12), 1u);
    ASSERT_EQ(clhs.get(42), 2u);
}

TEST(SparseSetNoType, RespectOverlap) {
    entt::SparseSet<std::uint64_t> lhs;
    entt::SparseSet<std::uint64_t> rhs;
    const auto &clhs = lhs;

    lhs.construct(3);
    lhs.construct(12);
    lhs.construct(42);

    rhs.construct(12);

    ASSERT_EQ(lhs.get(3), 0u);
    ASSERT_EQ(lhs.get(12), 1u);
    ASSERT_EQ(lhs.get(42), 2u);

    lhs.respect(rhs);

    ASSERT_EQ(clhs.get(3), 0u);
    ASSERT_EQ(clhs.get(12), 2u);
    ASSERT_EQ(clhs.get(42), 1u);
}

TEST(SparseSetNoType, RespectOrdered) {
    entt::SparseSet<std::uint64_t> lhs;
    entt::SparseSet<std::uint64_t> rhs;

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

TEST(SparseSetNoType, RespectReverse) {
    entt::SparseSet<std::uint64_t> lhs;
    entt::SparseSet<std::uint64_t> rhs;

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

TEST(SparseSetNoType, RespectUnordered) {
    entt::SparseSet<std::uint64_t> lhs;
    entt::SparseSet<std::uint64_t> rhs;

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

TEST(SparseSetNoType, CanModifyDuringIteration) {
    entt::SparseSet<std::uint64_t> set;
    set.construct(0);

    ASSERT_EQ(set.capacity(), entt::SparseSet<std::uint64_t>::size_type{1});

    const auto it = set.cbegin();
    set.reserve(entt::SparseSet<std::uint64_t>::size_type{2});

    ASSERT_EQ(set.capacity(), entt::SparseSet<std::uint64_t>::size_type{2});

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}

TEST(SparseSetWithType, Functionalities) {
    entt::SparseSet<std::uint64_t, int> set;
    const auto &cset = set;

    set.reserve(42);

    ASSERT_EQ(set.capacity(), 42);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42, 3);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(cset.begin(), cset.end());
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_TRUE(set.has(42));
    ASSERT_TRUE(set.fast(42));
    ASSERT_EQ(set.get(42), 3);

    set.destroy(42);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42, 12);

    ASSERT_EQ(set.get(42), 12);

    set.reset();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(cset.begin(), cset.end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    (void)entt::SparseSet<std::uint64_t, int>{std::move(set)};
    entt::SparseSet<std::uint64_t, int> other;
    other = std::move(set);
}

TEST(SparseSetWithType, ElementAccess) {
    entt::SparseSet<std::uint64_t, int> set;
    const auto &cset = set;

    set.construct(42, 1);
    set.construct(3, 0);

    for(typename entt::SparseSet<std::uint64_t, int>::size_type i{}; i < set.size(); ++i) {
        ASSERT_EQ(set[i], i);
        ASSERT_EQ(cset[i], i);
    }
}

TEST(SparseSetWithType, AggregatesMustWork) {
    struct AggregateType { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::SparseSet<std::uint64_t, AggregateType>{}.construct(0, 42);
}

TEST(SparseSetWithType, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::SparseSet<std::uint64_t, std::unordered_set<int>> set;
    set.construct(0).insert(42);
    set.destroy(0);
}

TEST(SparseSetWithType, Iterator) {
    struct InternalType { int value; };

    using iterator_type = typename entt::SparseSet<std::uint64_t, InternalType>::iterator_type;

    entt::SparseSet<std::uint64_t, InternalType> set;
    set.construct(3, 42);

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

TEST(SparseSetWithType, ConstIterator) {
    struct InternalType { int value; };

    using iterator_type = typename entt::SparseSet<std::uint64_t, InternalType>::const_iterator_type;

    entt::SparseSet<std::uint64_t, InternalType> set;
    set.construct(3, 42);

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

TEST(SparseSetWithType, Raw) {
    entt::SparseSet<std::uint64_t, int> set;

    set.construct(3, 3);
    set.construct(12, 6);
    set.construct(42, 9);

    ASSERT_EQ(set.get(3), 3);
    ASSERT_EQ(set.get(12), 6);
    ASSERT_EQ(set.get(42), 9);

    ASSERT_EQ(*(set.raw() + 0u), 3);
    ASSERT_EQ(*(set.raw() + 1u), 6);
    ASSERT_EQ(*(set.raw() + 2u), 9);
}

TEST(SparseSetWithType, SortOrdered) {
    entt::SparseSet<std::uint64_t, int> set;

    set.construct(12, 12);
    set.construct(42, 9);
    set.construct(7, 6);
    set.construct(3, 3);
    set.construct(9, 1);

    ASSERT_EQ(set.get(12), 12);
    ASSERT_EQ(set.get(42), 9);
    ASSERT_EQ(set.get(7), 6);
    ASSERT_EQ(set.get(3), 3);
    ASSERT_EQ(set.get(9), 1);

    set.sort([](auto lhs, auto rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(set.raw() + 0u), 12);
    ASSERT_EQ(*(set.raw() + 1u), 9);
    ASSERT_EQ(*(set.raw() + 2u), 6);
    ASSERT_EQ(*(set.raw() + 3u), 3);
    ASSERT_EQ(*(set.raw() + 4u), 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), 1);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, SortReverse) {
    entt::SparseSet<std::uint64_t, int> set;

    set.construct(12, 1);
    set.construct(42, 3);
    set.construct(7, 6);
    set.construct(3, 9);
    set.construct(9, 12);

    ASSERT_EQ(set.get(12), 1);
    ASSERT_EQ(set.get(42), 3);
    ASSERT_EQ(set.get(7), 6);
    ASSERT_EQ(set.get(3), 9);
    ASSERT_EQ(set.get(9), 12);

    set.sort([](auto lhs, auto rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(set.raw() + 0u), 12);
    ASSERT_EQ(*(set.raw() + 1u), 9);
    ASSERT_EQ(*(set.raw() + 2u), 6);
    ASSERT_EQ(*(set.raw() + 3u), 3);
    ASSERT_EQ(*(set.raw() + 4u), 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), 1);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, SortUnordered) {
    entt::SparseSet<std::uint64_t, int> set;

    set.construct(12, 6);
    set.construct(42, 3);
    set.construct(7, 1);
    set.construct(3, 9);
    set.construct(9, 12);

    ASSERT_EQ(set.get(12), 6);
    ASSERT_EQ(set.get(42), 3);
    ASSERT_EQ(set.get(7), 1);
    ASSERT_EQ(set.get(3), 9);
    ASSERT_EQ(set.get(9), 12);

    set.sort([](auto lhs, auto rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(set.raw() + 0u), 12);
    ASSERT_EQ(*(set.raw() + 1u), 9);
    ASSERT_EQ(*(set.raw() + 2u), 6);
    ASSERT_EQ(*(set.raw() + 3u), 3);
    ASSERT_EQ(*(set.raw() + 4u), 1);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), 1);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectDisjoint) {
    entt::SparseSet<std::uint64_t, int> lhs;
    entt::SparseSet<std::uint64_t, int> rhs;
    const auto &clhs = lhs;

    lhs.construct(3, 3);
    lhs.construct(12, 6);
    lhs.construct(42, 9);

    ASSERT_EQ(clhs.get(3), 3);
    ASSERT_EQ(clhs.get(12), 6);
    ASSERT_EQ(clhs.get(42), 9);

    lhs.respect(rhs);

    ASSERT_EQ(*(clhs.raw() + 0u), 3);
    ASSERT_EQ(*(clhs.raw() + 1u), 6);
    ASSERT_EQ(*(clhs.raw() + 2u), 9);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectOverlap) {
    entt::SparseSet<std::uint64_t, int> lhs;
    entt::SparseSet<std::uint64_t, int> rhs;
    const auto &clhs = lhs;

    lhs.construct(3, 3);
    lhs.construct(12, 6);
    lhs.construct(42, 9);
    rhs.construct(12, 6);

    ASSERT_EQ(clhs.get(3), 3);
    ASSERT_EQ(clhs.get(12), 6);
    ASSERT_EQ(clhs.get(42), 9);
    ASSERT_EQ(rhs.get(12), 6);

    lhs.respect(rhs);

    ASSERT_EQ(*(clhs.raw() + 0u), 3);
    ASSERT_EQ(*(clhs.raw() + 1u), 9);
    ASSERT_EQ(*(clhs.raw() + 2u), 6);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), 6);
    ASSERT_EQ(*(begin++), 9);
    ASSERT_EQ(*(begin++), 3);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectOrdered) {
    entt::SparseSet<std::uint64_t, int> lhs;
    entt::SparseSet<std::uint64_t, int> rhs;

    lhs.construct(1, 0);
    lhs.construct(2, 0);
    lhs.construct(3, 0);
    lhs.construct(4, 0);
    lhs.construct(5, 0);

    ASSERT_EQ(lhs.get(1), 0);
    ASSERT_EQ(lhs.get(2), 0);
    ASSERT_EQ(lhs.get(3), 0);
    ASSERT_EQ(lhs.get(4), 0);
    ASSERT_EQ(lhs.get(5), 0);

    rhs.construct(6, 0);
    rhs.construct(1, 0);
    rhs.construct(2, 0);
    rhs.construct(3, 0);
    rhs.construct(4, 0);
    rhs.construct(5, 0);

    ASSERT_EQ(rhs.get(6), 0);
    ASSERT_EQ(rhs.get(1), 0);
    ASSERT_EQ(rhs.get(2), 0);
    ASSERT_EQ(rhs.get(3), 0);
    ASSERT_EQ(rhs.get(4), 0);
    ASSERT_EQ(rhs.get(5), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), 1u);
    ASSERT_EQ(*(lhs.data() + 1u), 2u);
    ASSERT_EQ(*(lhs.data() + 2u), 3u);
    ASSERT_EQ(*(lhs.data() + 3u), 4u);
    ASSERT_EQ(*(lhs.data() + 4u), 5u);

    ASSERT_EQ(*(rhs.data() + 0u), 6u);
    ASSERT_EQ(*(rhs.data() + 1u), 1u);
    ASSERT_EQ(*(rhs.data() + 2u), 2u);
    ASSERT_EQ(*(rhs.data() + 3u), 3u);
    ASSERT_EQ(*(rhs.data() + 4u), 4u);
    ASSERT_EQ(*(rhs.data() + 5u), 5u);
}

TEST(SparseSetWithType, RespectReverse) {
    entt::SparseSet<std::uint64_t, int> lhs;
    entt::SparseSet<std::uint64_t, int> rhs;

    lhs.construct(1, 0);
    lhs.construct(2, 0);
    lhs.construct(3, 0);
    lhs.construct(4, 0);
    lhs.construct(5, 0);

    ASSERT_EQ(lhs.get(1), 0);
    ASSERT_EQ(lhs.get(2), 0);
    ASSERT_EQ(lhs.get(3), 0);
    ASSERT_EQ(lhs.get(4), 0);
    ASSERT_EQ(lhs.get(5), 0);

    rhs.construct(5, 0);
    rhs.construct(4, 0);
    rhs.construct(3, 0);
    rhs.construct(2, 0);
    rhs.construct(1, 0);
    rhs.construct(6, 0);

    ASSERT_EQ(rhs.get(5), 0);
    ASSERT_EQ(rhs.get(4), 0);
    ASSERT_EQ(rhs.get(3), 0);
    ASSERT_EQ(rhs.get(2), 0);
    ASSERT_EQ(rhs.get(1), 0);
    ASSERT_EQ(rhs.get(6), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), 1u);
    ASSERT_EQ(*(lhs.data() + 1u), 2u);
    ASSERT_EQ(*(lhs.data() + 2u), 3u);
    ASSERT_EQ(*(lhs.data() + 3u), 4u);
    ASSERT_EQ(*(lhs.data() + 4u), 5u);

    ASSERT_EQ(*(rhs.data() + 0u), 6u);
    ASSERT_EQ(*(rhs.data() + 1u), 1u);
    ASSERT_EQ(*(rhs.data() + 2u), 2u);
    ASSERT_EQ(*(rhs.data() + 3u), 3u);
    ASSERT_EQ(*(rhs.data() + 4u), 4u);
    ASSERT_EQ(*(rhs.data() + 5u), 5u);
}

TEST(SparseSetWithType, RespectUnordered) {
    entt::SparseSet<std::uint64_t, int> lhs;
    entt::SparseSet<std::uint64_t, int> rhs;

    lhs.construct(1, 0);
    lhs.construct(2, 0);
    lhs.construct(3, 0);
    lhs.construct(4, 0);
    lhs.construct(5, 0);

    ASSERT_EQ(lhs.get(1), 0);
    ASSERT_EQ(lhs.get(2), 0);
    ASSERT_EQ(lhs.get(3), 0);
    ASSERT_EQ(lhs.get(4), 0);
    ASSERT_EQ(lhs.get(5), 0);

    rhs.construct(3, 0);
    rhs.construct(2, 0);
    rhs.construct(6, 0);
    rhs.construct(1, 0);
    rhs.construct(4, 0);
    rhs.construct(5, 0);

    ASSERT_EQ(rhs.get(3), 0);
    ASSERT_EQ(rhs.get(2), 0);
    ASSERT_EQ(rhs.get(6), 0);
    ASSERT_EQ(rhs.get(1), 0);
    ASSERT_EQ(rhs.get(4), 0);
    ASSERT_EQ(rhs.get(5), 0);

    rhs.respect(lhs);

    ASSERT_EQ(*(lhs.data() + 0u), 1u);
    ASSERT_EQ(*(lhs.data() + 1u), 2u);
    ASSERT_EQ(*(lhs.data() + 2u), 3u);
    ASSERT_EQ(*(lhs.data() + 3u), 4u);
    ASSERT_EQ(*(lhs.data() + 4u), 5u);

    ASSERT_EQ(*(rhs.data() + 0u), 6u);
    ASSERT_EQ(*(rhs.data() + 1u), 1u);
    ASSERT_EQ(*(rhs.data() + 2u), 2u);
    ASSERT_EQ(*(rhs.data() + 3u), 3u);
    ASSERT_EQ(*(rhs.data() + 4u), 4u);
    ASSERT_EQ(*(rhs.data() + 5u), 5u);
}

TEST(SparseSetWithType, CanModifyDuringIteration) {
    entt::SparseSet<std::uint64_t, int> set;
    set.construct(0, 42);

    ASSERT_EQ(set.capacity(), entt::SparseSet<std::uint64_t>::size_type{1});

    const auto it = set.cbegin();
    set.reserve(entt::SparseSet<std::uint64_t>::size_type{2});

    ASSERT_EQ(set.capacity(), entt::SparseSet<std::uint64_t>::size_type{2});

    // this should crash with asan enabled if we break the constraint
    const auto entity = *it;
    (void)entity;
}

TEST(SparseSetWithType, ReferencesGuaranteed) {
    struct InternalType { int value; };

    entt::SparseSet<std::uint64_t, InternalType> set;

    set.construct(0, 0);
    set.construct(1, 1);

    ASSERT_EQ(set.get(0).value, 0);
    ASSERT_EQ(set.get(1).value, 1);

    for(auto &&type: set) {
        if(type.value) {
            type.value = 42;
        }
    }

    ASSERT_EQ(set.get(0).value, 0);
    ASSERT_EQ(set.get(1).value, 42);

    auto begin = set.begin();

    while(begin != set.end()) {
        (begin++)->value = 3;
    }

    ASSERT_EQ(set.get(0).value, 3);
    ASSERT_EQ(set.get(1).value, 3);
}

TEST(SparseSetWithType, MoveOnlyComponent) {
    struct MoveOnlyComponent {
        MoveOnlyComponent() = default;
        ~MoveOnlyComponent() = default;
        MoveOnlyComponent(const MoveOnlyComponent &) = delete;
        MoveOnlyComponent(MoveOnlyComponent &&) = default;
        MoveOnlyComponent & operator=(const MoveOnlyComponent &) = delete;
        MoveOnlyComponent & operator=(MoveOnlyComponent &&) = default;
    };

    // it's purpose is to ensure that move only components are always accepted
    entt::SparseSet<std::uint64_t, MoveOnlyComponent> set;
    (void)set;
}
