#include <gtest/gtest.h>
#include <entt/stl/tuple.hpp>

TEST(Tuple, HasInclude) {
    static_assert(entt::stl::entt_ext_tuple, "Header not properly included");
}
