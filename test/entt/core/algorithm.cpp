#include <algorithm>
#include <array>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/algorithm.hpp>
#include "../../common/boxed_type.h"

TEST(Algorithm, StdSort) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array arr{4, 1, 3, 2, 0};
    const entt::std_sort sort;

    sort(arr.begin(), arr.end());

    ASSERT_TRUE(std::is_sorted(arr.begin(), arr.end()));
}

TEST(Algorithm, StdSortBoxedInt) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array arr{test::boxed_int{4}, test::boxed_int{1}, test::boxed_int{3}, test::boxed_int{2}, test::boxed_int{0}, test::boxed_int{8}};
    const entt::std_sort sort;

    sort(arr.begin(), arr.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.value > rhs.value;
    });

    ASSERT_TRUE(std::is_sorted(arr.rbegin(), arr.rend()));
}

TEST(Algorithm, StdSortEmptyContainer) {
    std::vector<int> vec{};
    const entt::std_sort sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}

TEST(Algorithm, InsertionSort) {
    std::array arr{4, 1, 3, 2, 0};
    const entt::insertion_sort sort;

    sort(arr.begin(), arr.end());

    ASSERT_TRUE(std::is_sorted(arr.begin(), arr.end()));
}

TEST(Algorithm, InsertionSortBoxedInt) {
    std::array arr{test::boxed_int{4}, test::boxed_int{1}, test::boxed_int{3}, test::boxed_int{2}, test::boxed_int{0}, test::boxed_int{8}};
    const entt::insertion_sort sort;

    sort(arr.begin(), arr.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.value > rhs.value;
    });

    ASSERT_TRUE(std::is_sorted(arr.rbegin(), arr.rend()));
}

TEST(Algorithm, InsertionSortEmptyContainer) {
    std::vector<int> vec{};
    const entt::insertion_sort sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}

TEST(Algorithm, RadixSort) {
    std::array arr{4u, 1u, 3u, 2u, 0u};
    const entt::radix_sort<8, 32> sort;

    sort(arr.begin(), arr.end(), [](const auto &value) {
        return value;
    });

    ASSERT_TRUE(std::is_sorted(arr.begin(), arr.end()));
}

TEST(Algorithm, RadixSortBoxedInt) {
    std::array arr{test::boxed_int{4}, test::boxed_int{1}, test::boxed_int{3}, test::boxed_int{2}, test::boxed_int{0}, test::boxed_int{8}};
    const entt::radix_sort<2, 6> sort;

    sort(arr.rbegin(), arr.rend(), [](const auto &instance) {
        return instance.value;
    });

    ASSERT_TRUE(std::is_sorted(arr.rbegin(), arr.rend()));
}

TEST(Algorithm, RadixSortEmptyContainer) {
    std::vector<int> vec{};
    const entt::radix_sort<8, 32> sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}
