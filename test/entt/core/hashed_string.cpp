#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>

TEST(HashedString, Functionalities) {
    using hash_type = entt::HashedString::hash_type;

    const char *bar = "bar";

    auto fooHs = entt::HashedString{"foo"};
    auto barHs = entt::HashedString{bar};

    ASSERT_NE(static_cast<hash_type>(fooHs), static_cast<hash_type>(barHs));
    ASSERT_STREQ(static_cast<const char *>(fooHs), "foo");
    ASSERT_STREQ(static_cast<const char *>(barHs), bar);

    ASSERT_EQ(fooHs, fooHs);
    ASSERT_NE(fooHs, barHs);

    entt::HashedString hs{"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hs), 0x85944171f73967e8);

    ASSERT_EQ(fooHs, "foo"_hs);
    ASSERT_NE(barHs, "foo"_hs);
}

TEST(HashedString, Constexprness) {
    using hash_type = entt::HashedString::hash_type;
    // how would you test a constexpr otherwise?
    (void)std::integral_constant<hash_type, entt::HashedString{"quux"}>{};
    (void)std::integral_constant<hash_type, "quux"_hs>{};
    ASSERT_TRUE(true);
}
