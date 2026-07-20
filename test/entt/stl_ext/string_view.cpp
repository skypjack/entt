#include <gtest/gtest.h>
#include <entt/stl/string_view.hpp>

TEST(StringView, HasInclude) {
    static_assert(entt::stl::entt_ext_string_view, "Header not properly included");
}
