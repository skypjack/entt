#include <gtest/gtest.h>
#include <entt/stl/cstdint.hpp>

TEST(CStdInt, HasInclude) {
    static_assert(entt::stl::entt_ext_cstdint, "Header not properly included");
}
