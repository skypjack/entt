#include <gtest/gtest.h>
#include <entt/stl/cstddef.hpp>

TEST(CStdDef, HasInclude) {
    static_assert(entt::stl::entt_ext_cstddef, "Header not properly included");
}
