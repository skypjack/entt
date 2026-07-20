#include <gtest/gtest.h>
#include <entt/stl/algorithm.hpp>

TEST(Algorithm, HasInclude) {
    static_assert(entt::stl::entt_ext_algorithm, "Header not properly included");
}
