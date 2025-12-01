#include <cmath>
#include <cstddef>
#include <limits>
#include <gtest/gtest.h>
#include <entt/core/bit.hpp>
#include "../../common/config.h"

TEST(Bit, FastMod) {
    // constexpr-ness guaranteed
    constexpr auto fast_mod_of_zero = entt::fast_mod(0u, 8u);

    ASSERT_EQ(fast_mod_of_zero, 0u);
    ASSERT_EQ(entt::fast_mod(7u, 8u), 7u);
    ASSERT_EQ(entt::fast_mod(8u, 8u), 0u);
}
