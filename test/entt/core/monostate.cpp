#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/monostate.hpp>

TEST(Monostate, Functionalities) {
    using namespace entt::literals;

    const bool b_pre = entt::monostate<entt::hashed_string{"foobar"}>{};
    const int i_pre = entt::monostate<"foobar"_hs>{};

    ASSERT_FALSE(b_pre);
    ASSERT_EQ(i_pre, int{});

    entt::monostate<"foobar"_hs>{} = true;
    entt::monostate_v<"foobar"_hs> = 42;

    const bool &b_post = entt::monostate<"foobar"_hs>{};
    const int &i_post = entt::monostate_v<entt::hashed_string{"foobar"}>;

    ASSERT_TRUE(b_post);
    ASSERT_EQ(i_post, 42);
}
