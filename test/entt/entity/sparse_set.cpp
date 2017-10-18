#include <gtest/gtest.h>
#include <entt/entity/sparse_set.hpp>

TEST(SparseSetNoType, Functionalities) {
    entt::SparseSet<unsigned int> set;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    ASSERT_EQ(set.construct(42), 0u);

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

    ASSERT_EQ(set.construct(42), 0u);

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

    ASSERT_EQ(set.construct(3), 0u);
    ASSERT_EQ(set.construct(12), 1u);
    ASSERT_EQ(set.construct(42), 2u);

    ASSERT_EQ(*(set.data() + 0u), 3u);
    ASSERT_EQ(*(set.data() + 1u), 12u);
    ASSERT_EQ(*(set.data() + 2u), 42u);

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), 42u);
    ASSERT_EQ(*(begin++), 12u);
    ASSERT_EQ(*(begin++), 3u);
    ASSERT_EQ(begin, end);
}

TEST(SparseSetWithType, Functionalities) {
    entt::SparseSet<unsigned int, int> set;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.has(0));
    ASSERT_FALSE(set.has(42));

    ASSERT_EQ(set.construct(42, 3), 3);

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

    ASSERT_EQ(set.construct(42, 12), 12);

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

TEST(SparseSetWithType, RawBeginEnd) {
    entt::SparseSet<unsigned int, int> set;

    ASSERT_EQ(set.construct(3, 3), 3);
    ASSERT_EQ(set.construct(12, 6), 6);
    ASSERT_EQ(set.construct(42, 9), 9);

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

    ASSERT_EQ(set.construct(12, 12), 12);
    ASSERT_EQ(set.construct(42, 9), 9);
    ASSERT_EQ(set.construct(7, 6), 6);
    ASSERT_EQ(set.construct(3, 3), 3);
    ASSERT_EQ(set.construct(9, 1), 1);

    set.sort([&set](auto lhs, auto rhs) {
        return set.get(lhs) < set.get(rhs);
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

    ASSERT_EQ(set.construct(12, 1), 1);
    ASSERT_EQ(set.construct(42, 3), 3);
    ASSERT_EQ(set.construct(7, 6), 6);
    ASSERT_EQ(set.construct(3, 9), 9);
    ASSERT_EQ(set.construct(9, 12), 12);

    set.sort([&set](auto lhs, auto rhs) {
        return set.get(lhs) < set.get(rhs);
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

    ASSERT_EQ(set.construct(12, 6), 6);
    ASSERT_EQ(set.construct(42, 3), 3);
    ASSERT_EQ(set.construct(7, 1), 1);
    ASSERT_EQ(set.construct(3, 9), 9);
    ASSERT_EQ(set.construct(9, 12), 12);

    set.sort([&set](auto lhs, auto rhs) {
        return set.get(lhs) < set.get(rhs);
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

    ASSERT_EQ(lhs.construct(3, 3), 3);
    ASSERT_EQ(lhs.construct(12, 6), 6);
    ASSERT_EQ(lhs.construct(42, 9), 9);

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

    ASSERT_EQ(lhs.construct(3, 3), 3);
    ASSERT_EQ(lhs.construct(12, 6), 6);
    ASSERT_EQ(lhs.construct(42, 9), 9);
    ASSERT_EQ(rhs.construct(12, 6), 6);

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

    ASSERT_EQ(lhs.construct(1, 0), 0);
    ASSERT_EQ(lhs.construct(2, 0), 0);
    ASSERT_EQ(lhs.construct(3, 0), 0);
    ASSERT_EQ(lhs.construct(4, 0), 0);
    ASSERT_EQ(lhs.construct(5, 0), 0);

    ASSERT_EQ(rhs.construct(6, 0), 0);
    ASSERT_EQ(rhs.construct(1, 0), 0);
    ASSERT_EQ(rhs.construct(2, 0), 0);
    ASSERT_EQ(rhs.construct(3, 0), 0);
    ASSERT_EQ(rhs.construct(4, 0), 0);
    ASSERT_EQ(rhs.construct(5, 0), 0);

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

    ASSERT_EQ(lhs.construct(1, 0), 0);
    ASSERT_EQ(lhs.construct(2, 0), 0);
    ASSERT_EQ(lhs.construct(3, 0), 0);
    ASSERT_EQ(lhs.construct(4, 0), 0);
    ASSERT_EQ(lhs.construct(5, 0), 0);

    ASSERT_EQ(rhs.construct(5, 0), 0);
    ASSERT_EQ(rhs.construct(4, 0), 0);
    ASSERT_EQ(rhs.construct(3, 0), 0);
    ASSERT_EQ(rhs.construct(2, 0), 0);
    ASSERT_EQ(rhs.construct(1, 0), 0);
    ASSERT_EQ(rhs.construct(6, 0), 0);

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

    ASSERT_EQ(lhs.construct(1, 0), 0);
    ASSERT_EQ(lhs.construct(2, 0), 0);
    ASSERT_EQ(lhs.construct(3, 0), 0);
    ASSERT_EQ(lhs.construct(4, 0), 0);
    ASSERT_EQ(lhs.construct(5, 0), 0);

    ASSERT_EQ(rhs.construct(3, 0), 0);
    ASSERT_EQ(rhs.construct(2, 0), 0);
    ASSERT_EQ(rhs.construct(6, 0), 0);
    ASSERT_EQ(rhs.construct(1, 0), 0);
    ASSERT_EQ(rhs.construct(4, 0), 0);
    ASSERT_EQ(rhs.construct(5, 0), 0);

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
