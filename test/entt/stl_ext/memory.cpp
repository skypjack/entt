#include <gtest/gtest.h>
#include <entt/stl/memory.hpp>

TEST(Memory, HasInclude) {
    static_assert(entt::stl::entt_ext_memory, "Header not properly included");
}
