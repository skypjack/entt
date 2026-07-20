#include <gtest/gtest.h>
#include <entt/stl/sstream.hpp>

TEST(SStream, HasInclude) {
    static_assert(entt::stl::entt_ext_sstream, "Header not properly included");
}
