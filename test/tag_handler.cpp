#include <gtest/gtest.h>
#include <tag_handler.hpp>

TEST(TagHandler, Functionalities) {
    using TagHandler = entt::TagHandler<unsigned int, int>;

    TagHandler handler;

    ASSERT_TRUE(handler.empty());
    ASSERT_EQ(handler.size(), 0u);
    ASSERT_EQ(handler.begin(), handler.end());
    ASSERT_FALSE(handler.has(0));
    ASSERT_FALSE(handler.has(1));

    ASSERT_EQ(handler.construct(0, 42), 42);

    ASSERT_FALSE(handler.empty());
    ASSERT_EQ(handler.size(), 1u);
    ASSERT_NE(handler.begin(), handler.end());
    ASSERT_TRUE(handler.has(0));
    ASSERT_FALSE(handler.has(1));

    auto begin = handler.begin();

    ASSERT_EQ(*begin, 42);
    ASSERT_EQ(handler.get(0), 42);
    ASSERT_EQ(++begin, handler.end());

    handler.destroy(0);

    ASSERT_TRUE(handler.empty());
    ASSERT_EQ(handler.size(), 0u);
    ASSERT_EQ(handler.begin(), handler.end());
    ASSERT_FALSE(handler.has(0));
    ASSERT_FALSE(handler.has(1));

    ASSERT_EQ(handler.construct(0, 12), 12);

    handler.reset();

    ASSERT_TRUE(handler.empty());
    ASSERT_EQ(handler.size(), 0u);
    ASSERT_EQ(handler.begin(), handler.end());
    ASSERT_FALSE(handler.has(0));
    ASSERT_FALSE(handler.has(1));
}
