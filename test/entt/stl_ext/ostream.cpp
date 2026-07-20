#include <gtest/gtest.h>
#include <entt/stl/ostream.hpp>

TEST(OStream, HasInclude) {
    static_assert(entt::stl::entt_ext_ostream, "Header not properly included");
}
