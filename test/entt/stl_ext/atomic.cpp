#include <gtest/gtest.h>
#include <entt/stl/atomic.hpp>

TEST(Atomic, HasInclude) {
    static_assert(entt::stl::entt_ext_atomic, "Header not properly included");
}
