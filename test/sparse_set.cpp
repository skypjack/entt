#include <gtest/gtest.h>
#include <sparse_set.hpp>

TEST(SparseSetNoType, Functionalities) {
    using SparseSet = entt::SparseSet<unsigned int>;

    SparseSet set;

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

    auto begin = set.begin();

    ASSERT_EQ(*begin, 42u);
    ASSERT_EQ(++begin, set.end());
    ASSERT_EQ(set.get(42), 0u);

    ASSERT_EQ(set.destroy(42), 0u);

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
}

TEST(SparseSetWithType, Functionalities) {
    using SparseSet = entt::SparseSet<unsigned int, int>;

    SparseSet set;

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

    auto begin = set.begin();

    ASSERT_EQ(*begin, 42u);
    ASSERT_EQ(set.get(42), 3);
    ASSERT_EQ(++begin, set.end());

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
}
