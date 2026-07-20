#include <gtest/gtest.h>
#include <entt/stl/bit.hpp>

TEST(Bit, HasInclude) {
    static_assert(entt::stl::entt_ext_bit, "Header not properly included");
}
