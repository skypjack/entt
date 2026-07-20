#include <gtest/gtest.h>
#include <entt/stl/vector.hpp>

TEST(Vector, HasInclude) {
    static_assert(entt::stl::entt_ext_vector, "Header not properly included");
}
