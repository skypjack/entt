#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/monostate.hpp>

TEST(Monostate, Functionalities) {
    using namespace entt::literals;

    static constexpr entt::id_type tag = "tag"_hs;
    const bool b_pre = entt::monostate<tag>{};
    const int i_pre = entt::monostate<tag>{};

    ASSERT_FALSE(b_pre);
    ASSERT_EQ(i_pre, int{});

    entt::monostate<tag>{} = true;
    entt::monostate_v<tag> = 2;

    const bool &b_post = entt::monostate<tag>{};
    const int &i_post = entt::monostate_v<tag>;

    ASSERT_TRUE(b_post);
    ASSERT_EQ(i_post, 2);
}
