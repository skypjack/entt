#include <gtest/gtest.h>
#include <entt/stl/type_traits.hpp>

TEST(TypeTraits, HasInclude) {
    static_assert(entt::stl::entt_ext_type_traits, "Header not properly included");
}
