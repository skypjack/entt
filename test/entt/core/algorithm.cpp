#include <array>
#include <gtest/gtest.h>
#include <entt/core/algorithm.hpp>

struct boxed_int {
    int value;
};

TEST(Algorithm, StdSort) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array<int, 5> arr{{4, 1, 3, 2, 0}};
    entt::std_sort sort;

    sort(arr.begin(), arr.end());

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}

TEST(Algorithm, StdSortBoxedInt) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array<boxed_int, 6> arr{{{4}, {1}, {3}, {2}, {0}, {6}}};
    entt::std_sort sort;

    sort(arr.begin(), arr.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.value > rhs.value;
    });

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_GT(arr[i].value, arr[i+1].value);
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

TEST(Algorithm, InsertionSortBoxedInt) {
    std::array<boxed_int, 6> arr{{{4}, {1}, {3}, {2}, {0}, {6}}};
    entt::insertion_sort sort;

    sort(arr.begin(), arr.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.value > rhs.value;
    });

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_GT(arr[i].value, arr[i+1].value);
    }
}

TEST(Algorithm, InsertionSortEmptyContainer) {
    std::vector<int> vec{};
    entt::insertion_sort sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}

TEST(Algorithm, RadixSort) {
    std::array<uint32_t, 5> arr{{4, 1, 3, 2, 0}};
    entt::radix_sort<8, 32> sort;

    sort(arr.begin(), arr.end(), [](const auto &value) {
        return value;
    });

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}

TEST(Algorithm, RadixSortBoxedInt) {
    std::array<boxed_int, 6> arr{{{4}, {1}, {3}, {2}, {0}, {6}}};
    entt::radix_sort<2, 6> sort;

    sort(arr.rbegin(), arr.rend(), [](const auto &instance) {
        return instance.value;
    });

    for(typename decltype(arr)::size_type i = 0; i < (arr.size() - 1); ++i) {
        ASSERT_GT(arr[i].value, arr[i+1].value);
    }
}

TEST(Algorithm, RadixSortEmptyContainer) {
    std::vector<int> vec{};
    entt::radix_sort<8, 32> sort;
    // this should crash with asan enabled if we break the constraint
    sort(vec.begin(), vec.end());
}
