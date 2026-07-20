#include <gtest/gtest.h>
#include <entt/stl/cmath.hpp>

TEST(CMath, HasInclude) {
    static_assert(entt::stl::entt_ext_cmath, "Header not properly included");
}
