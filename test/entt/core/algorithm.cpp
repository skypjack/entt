#include <array>
#include <gtest/gtest.h>
#include <entt/core/algorithm.hpp>

TEST(Algorithm, StdSort) {
    // well, I'm pretty sure it works, it's std::sort!!
    std::array<int, 5> arr{{4, 1, 3, 2, 0}};
    entt::StdSort sort;

    sort(arr.begin(), arr.end());

    for(auto i = 0; i < 4; ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}

TEST(Algorithm, InsertionSort) {
    std::array<int, 5> arr{{4, 1, 3, 2, 0}};
    entt::InsertionSort sort;

    sort(arr.begin(), arr.end());

    for(auto i = 0; i < 4; ++i) {
        ASSERT_LT(arr[i], arr[i+1]);
    }
}
