#include <cstddef>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "common/boxed_type.h"
#include <entt/core/iterator.hpp>

TEST(InputIteratorPointer, Functionalities) {
    entt::input_iterator_pointer ptr{test::boxed_int{0}};

    ASSERT_EQ(ptr->value, 0);

    ptr->value = 2;

    ASSERT_EQ(ptr->value, 2);
    ASSERT_EQ(ptr->value, (*ptr).value);
    ASSERT_EQ(ptr.operator->(), &ptr.operator*());
}

TEST(IotaIterator, Functionalities) {
    entt::iota_iterator<std::size_t> first{};
    const entt::iota_iterator<std::size_t> last{2u};

    ASSERT_NE(first, last);
    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_EQ(*first++, 0u);
    ASSERT_EQ(*first, 1u);
    ASSERT_EQ(*++first, *last);
    ASSERT_EQ(*first, 2u);
}

TEST(IterableAdaptor, Functionalities) {
    std::vector<int> vec{1, 2};
    entt::iterable_adaptor iterable{vec.begin(), vec.end()};
    decltype(iterable) other{};

    ASSERT_NO_THROW(other = iterable);
    ASSERT_NO_THROW(std::swap(other, iterable));

    ASSERT_EQ(iterable.begin(), vec.begin());
    ASSERT_EQ(iterable.end(), vec.end());

    ASSERT_EQ(*iterable.cbegin(), 1);
    ASSERT_EQ(*++iterable.cbegin(), 2);
    ASSERT_EQ(++iterable.cbegin(), --iterable.end());

    for(auto value: entt::iterable_adaptor<const int *, const void *>{&vec[0u], &vec[1u]}) {
        ASSERT_EQ(value, 1);
    }
}
