#include <gtest/gtest.h>
#include <entt/stl/cassert.hpp>

TEST(CAssert, HasInclude) {
    static_assert(entt::stl::entt_ext_cassert, "Header not properly included");
}
