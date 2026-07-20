#include <gtest/gtest.h>
#include <entt/stl/limits.hpp>

TEST(Limits, HasInclude) {
    static_assert(entt::stl::entt_ext_limits, "Header not properly included");
}
