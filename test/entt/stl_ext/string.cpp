#include <gtest/gtest.h>
#include <entt/stl/string.hpp>

TEST(String, HasInclude) {
    static_assert(entt::stl::entt_ext_string, "Header not properly included");
}
