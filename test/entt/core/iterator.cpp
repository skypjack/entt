#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>

struct clazz {
    int value{0};
};

TEST(Iterator, InputIteratorPointer) {
    static_assert(!std::is_default_constructible_v<entt::input_iterator_pointer<clazz>>);
    static_assert(!std::is_copy_constructible_v<entt::input_iterator_pointer<clazz>>);
    static_assert(std::is_move_constructible_v<entt::input_iterator_pointer<clazz>>);
    static_assert(!std::is_copy_assignable_v<entt::input_iterator_pointer<clazz>>);
    static_assert(std::is_move_assignable_v<entt::input_iterator_pointer<clazz>>);

    clazz instance{};
    entt::input_iterator_pointer ptr{std::move(instance)};
    ptr->value = 42;

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(ptr->value, 42);
}

TEST(Iterator, IterableAdaptor) {
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
}
