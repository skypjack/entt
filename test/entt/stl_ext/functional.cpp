#include <gtest/gtest.h>
#include <entt/stl/functional.hpp>

TEST(Functional, HasInclude) {
    static_assert(entt::stl::entt_ext_functional, "Header not properly included");
}
