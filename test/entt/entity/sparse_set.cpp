#include <unordered_set>
#include <gtest/gtest.h>
#include <entt/entity/sparse_set.hpp>

TEST(SparseSetNoType, Functionalities) {
    entt::SparseSet<unsigned int> set;

    ASSERT_NO_THROW(set.reserve(42));
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_EQ(set.get(42), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_TRUE(set.has(42));
    ASSERT_EQ(set.get(42), 0u);

    set.destroy(42);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42);

    ASSERT_EQ(set.get(42), 0u);

    set.reset();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    (void)entt::SparseSet<unsigned int>{std::move(set)};
    entt::SparseSet<unsigned int> other;
    other = std::move(set);
}

TEST(SparseSetNoType, DataBeginEnd) {
    entt::SparseSet<unsigned int> set;

    set.construct(3);
    set.construct(12);
    set.construct(42);

    ASSERT_EQ(set.get(3), 0u);
    ASSERT_EQ(set.get(12), 1u);
    ASSERT_EQ(set.get(42), 2u);

    ASSERT_EQ(*(set.data() + 0u), 3u);
    ASSERT_EQ(*(set.data() + 1u), 12u);
    ASSERT_EQ(*(set.data() + 2u), 42u);

    auto it = set.begin();

    ASSERT_EQ(*it, 42u);
    ASSERT_EQ(*(it+1), 12u);
    ASSERT_EQ(*(it+2), 3u);
    ASSERT_EQ(it += 3, set.end());

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), 42u);
    ASSERT_EQ(*(begin++), 12u);
    ASSERT_EQ(*(begin++), 3u);

    ASSERT_EQ(begin, end);
}

TEST(SparseSetNoType, RespectDisjoint) {
    entt::SparseSet<unsigned int> lhs;
    entt::SparseSet<unsigned int> rhs;
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
    entt::SparseSet<unsigned int> lhs;
    entt::SparseSet<unsigned int> rhs;
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
    entt::SparseSet<unsigned int> lhs;
    entt::SparseSet<unsigned int> rhs;

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
    entt::SparseSet<unsigned int> lhs;
    entt::SparseSet<unsigned int> rhs;

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
    entt::SparseSet<unsigned int> lhs;
    entt::SparseSet<unsigned int> rhs;

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

TEST(SparseSetWithType, Functionalities) {
    entt::SparseSet<unsigned int, int> set;

    ASSERT_NO_THROW(set.reserve(42));
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42, 3);

    ASSERT_EQ(set.get(42), 3);
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_TRUE(set.has(42));
    ASSERT_EQ(set.get(42), 3);

    set.destroy(42);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    set.construct(42, 12);

    ASSERT_EQ(set.get(42), 12);

    set.reset();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    (void)entt::SparseSet<unsigned int>{std::move(set)};
    entt::SparseSet<unsigned int> other;
    other = std::move(set);
}

TEST(SparseSetWithType, AggregatesMustWork) {
    struct AggregateType { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::SparseSet<unsigned int, AggregateType>{}.construct(0, 42);
}

TEST(SparseSetWithType, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::SparseSet<unsigned int, std::unordered_set<int>> set;
    set.construct(0).insert(42);
    set.destroy(0);
}

TEST(SparseSetWithType, RawBeginEnd) {
    entt::SparseSet<unsigned int, int> set;

    set.construct(3, 3);
    set.construct(12, 6);
    set.construct(42, 9);

    ASSERT_EQ(set.get(3), 3);
    ASSERT_EQ(set.get(12), 6);
    ASSERT_EQ(set.get(42), 9);

    ASSERT_EQ(*(set.raw() + 0u), 3);
    ASSERT_EQ(*(set.raw() + 1u), 6);
    ASSERT_EQ(*(set.raw() + 2u), 9);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(set.get(*(begin++)), 9);
    ASSERT_EQ(set.get(*(begin++)), 6);
    ASSERT_EQ(set.get(*(begin++)), 3);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, SortOrdered) {
    entt::SparseSet<unsigned int, int> set;

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

    ASSERT_EQ(set.get(*(begin++)), 1);
    ASSERT_EQ(set.get(*(begin++)), 3);
    ASSERT_EQ(set.get(*(begin++)), 6);
    ASSERT_EQ(set.get(*(begin++)), 9);
    ASSERT_EQ(set.get(*(begin++)), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, SortReverse) {
    entt::SparseSet<unsigned int, int> set;

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

    ASSERT_EQ(set.get(*(begin++)), 1);
    ASSERT_EQ(set.get(*(begin++)), 3);
    ASSERT_EQ(set.get(*(begin++)), 6);
    ASSERT_EQ(set.get(*(begin++)), 9);
    ASSERT_EQ(set.get(*(begin++)), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, SortUnordered) {
    entt::SparseSet<unsigned int, int> set;

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

    ASSERT_EQ(set.get(*(begin++)), 1);
    ASSERT_EQ(set.get(*(begin++)), 3);
    ASSERT_EQ(set.get(*(begin++)), 6);
    ASSERT_EQ(set.get(*(begin++)), 9);
    ASSERT_EQ(set.get(*(begin++)), 12);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectDisjoint) {
    entt::SparseSet<unsigned int, int> lhs;
    entt::SparseSet<unsigned int, int> rhs;
    const auto &clhs = lhs;

    lhs.construct(3, 3);
    lhs.construct(12, 6);
    lhs.construct(42, 9);

    ASSERT_EQ(lhs.get(3), 3);
    ASSERT_EQ(lhs.get(12), 6);
    ASSERT_EQ(lhs.get(42), 9);

    lhs.respect(rhs);

    ASSERT_EQ(*(clhs.raw() + 0u), 3);
    ASSERT_EQ(*(clhs.raw() + 1u), 6);
    ASSERT_EQ(*(clhs.raw() + 2u), 9);

    auto begin = clhs.begin();
    auto end = clhs.end();

    ASSERT_EQ(clhs.get(*(begin++)), 9);
    ASSERT_EQ(clhs.get(*(begin++)), 6);
    ASSERT_EQ(clhs.get(*(begin++)), 3);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectOverlap) {
    entt::SparseSet<unsigned int, int> lhs;
    entt::SparseSet<unsigned int, int> rhs;
    const auto &clhs = lhs;

    lhs.construct(3, 3);
    lhs.construct(12, 6);
    lhs.construct(42, 9);
    rhs.construct(12, 6);

    ASSERT_EQ(lhs.get(3), 3);
    ASSERT_EQ(lhs.get(12), 6);
    ASSERT_EQ(lhs.get(42), 9);
    ASSERT_EQ(rhs.get(12), 6);

    lhs.respect(rhs);

    ASSERT_EQ(*(clhs.raw() + 0u), 3);
    ASSERT_EQ(*(clhs.raw() + 1u), 9);
    ASSERT_EQ(*(clhs.raw() + 2u), 6);

    auto begin = clhs.begin();
    auto end = clhs.end();

    ASSERT_EQ(clhs.get(*(begin++)), 6);
    ASSERT_EQ(clhs.get(*(begin++)), 9);
    ASSERT_EQ(clhs.get(*(begin++)), 3);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, RespectOrdered) {
    entt::SparseSet<unsigned int, int> lhs;
    entt::SparseSet<unsigned int, int> rhs;

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
    entt::SparseSet<unsigned int, int> lhs;
    entt::SparseSet<unsigned int, int> rhs;

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
    entt::SparseSet<unsigned int, int> lhs;
    entt::SparseSet<unsigned int, int> rhs;

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
