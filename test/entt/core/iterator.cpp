#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>

struct clazz {
    int value{0};
};

TEST(Iterator, InputIteratorProxy) {
    entt::input_iterator_proxy proxy{clazz{}};
    proxy->value = 42;

    ASSERT_EQ(proxy->value, 42);
}
