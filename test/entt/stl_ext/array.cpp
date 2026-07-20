#include <gtest/gtest.h>
#include <entt/stl/array.hpp>

TEST(Array, HasInclude) {
    static_assert(entt::stl::entt_ext_array, "Header not properly included");
}
