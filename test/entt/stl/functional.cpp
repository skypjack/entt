#include <memory>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/stl/functional.hpp>

TEST(Functional, Identity) {
    const entt::stl::identity identity;
    int value = 2;

    ASSERT_TRUE(entt::is_transparent_v<entt::stl::identity>);
    ASSERT_EQ(identity(value), value);
    ASSERT_EQ(&identity(value), &value);
}
