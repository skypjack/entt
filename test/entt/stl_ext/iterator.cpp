#include <gtest/gtest.h>
#include <entt/stl/iterator.hpp>

TEST(Iterator, HasInclude) {
    static_assert(entt::stl::entt_ext_iterator, "Header not properly included");
}
