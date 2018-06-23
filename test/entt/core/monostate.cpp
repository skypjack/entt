#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/monostate.hpp>

TEST(Monostate, Functionalities) {
    const bool bPre = entt::Monostate<entt::HashedString{"foobar"}>{};
    const int iPre = entt::Monostate<"foobar"_hs>{};

    ASSERT_FALSE(bPre);
    ASSERT_EQ(iPre, int{});

    entt::Monostate<"foobar"_hs>{} = true;
    entt::Monostate<"foobar"_hs>{} = 42;

    const bool &bPost = entt::Monostate<"foobar"_hs>{};
    const int &iPost = entt::Monostate<entt::HashedString{"foobar"}>{};

    ASSERT_TRUE(bPost);
    ASSERT_EQ(iPost, 42);
}
