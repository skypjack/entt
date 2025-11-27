#include <cmath>
#include <cstddef>
#include <limits>
#include <gtest/gtest.h>
#include <entt/core/bit.hpp>
#include "../../common/config.h"

TEST(NextPowerOfTwo, Functionalities) {
    // constexpr-ness guaranteed
    constexpr auto next_power_of_two_of_zero = entt::next_power_of_two(0u);

    ASSERT_EQ(next_power_of_two_of_zero, 1u);
    ASSERT_EQ(entt::next_power_of_two(1u), 1u);
    ASSERT_EQ(entt::next_power_of_two(2u), 2u);
    ASSERT_EQ(entt::next_power_of_two(3u), 4u);
    ASSERT_EQ(entt::next_power_of_two(17u), 32u);
    ASSERT_EQ(entt::next_power_of_two(32u), 32u);
    ASSERT_EQ(entt::next_power_of_two(33u), 64u);
    ASSERT_EQ(entt::next_power_of_two(static_cast<std::size_t>(std::pow(2, 16))), static_cast<std::size_t>(std::pow(2, 16)));
    ASSERT_EQ(entt::next_power_of_two(static_cast<std::size_t>(std::pow(2, 16) + 1u)), static_cast<std::size_t>(std::pow(2, 17)));
}

ENTT_DEBUG_TEST(NextPowerOfTwoDeathTest, Functionalities) {
    ASSERT_DEATH(static_cast<void>(entt::next_power_of_two((std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)) + 1)), "");
}

TEST(FastMod, Functionalities) {
    // constexpr-ness guaranteed
    constexpr auto fast_mod_of_zero = entt::fast_mod(0u, 8u);

    ASSERT_EQ(fast_mod_of_zero, 0u);
    ASSERT_EQ(entt::fast_mod(7u, 8u), 7u);
    ASSERT_EQ(entt::fast_mod(8u, 8u), 0u);
}
