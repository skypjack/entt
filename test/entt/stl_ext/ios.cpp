#include <gtest/gtest.h>
#include <entt/stl/ios.hpp>

TEST(IOS, HasInclude) {
    static_assert(entt::stl::entt_ext_ios, "Header not properly included");
}
