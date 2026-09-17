#include <gtest/gtest.h>
#include <entt/stl/compare.hpp>

TEST(Compare, HasInclude) {
    static_assert(entt::stl::entt_ext_compare, "Header not properly included");
}
