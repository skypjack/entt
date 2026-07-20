#include <gtest/gtest.h>
#include <entt/stl/concepts.hpp>

TEST(Concepts, HasInclude) {
    static_assert(entt::stl::entt_ext_concepts, "Header not properly included");
}
