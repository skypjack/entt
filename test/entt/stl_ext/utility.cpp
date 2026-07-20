#include <gtest/gtest.h>
#include <entt/stl/utility.hpp>

TEST(Utility, HasInclude) {
    static_assert(entt::stl::entt_ext_utility, "Header not properly included");
}
