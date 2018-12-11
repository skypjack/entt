#include <array>
#include <gtest/gtest.h>
#include <entt/core/algorithm.hpp>

TEST(Algorithm, StdSort) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array<int, 5> arr{{4, 1, 3, 2, 0}};
    entt::std_sort sort;

    sort(arr.begin(), arr.end());

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}

TEST(Algorithm, InsertionSort) {
    std::array<int, 5> arr{{4, 1, 3, 2, 0}};
    entt::insertion_sort sort;

    sort(arr.begin(), arr.end());

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}

TEST(Algorithm, InsertionSortEmptyContainer) {
    std::vector<int> vec{};
    entt::insertion_sort sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}
