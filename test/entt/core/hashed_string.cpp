#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>

TEST(HashedString, Functionalities) {
    using hash_type = entt::hashed_string::hash_type;

    const char *bar = "bar";

    auto foo_hs = entt::hashed_string{"foo"};
    auto bar_hs = entt::hashed_string{bar};

    ASSERT_NE(static_cast<hash_type>(foo_hs), static_cast<hash_type>(bar_hs));
    ASSERT_STREQ(static_cast<const char *>(foo_hs), "foo");
    ASSERT_STREQ(static_cast<const char *>(bar_hs), bar);
    ASSERT_STREQ(foo_hs.data(), "foo");
    ASSERT_STREQ(bar_hs.data(), bar);

    ASSERT_EQ(foo_hs, foo_hs);
    ASSERT_NE(foo_hs, bar_hs);

    entt::hashed_string hs{"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hs), 0xbf9cf968);
    ASSERT_EQ(hs.value(), 0xbf9cf968);

    ASSERT_EQ(foo_hs, "foo"_hs);
    ASSERT_NE(bar_hs, "foo"_hs);
}

TEST(HashedString, Empty) {
    using hash_type = entt::hashed_string::hash_type;

    entt::hashed_string hs{};

    ASSERT_EQ(static_cast<hash_type>(hs), hash_type{});
    ASSERT_EQ(static_cast<const char *>(hs), nullptr);
}

TEST(HashedString, Constexprness) {
    using hash_type = entt::hashed_string::hash_type;
    // how would you test a constexpr otherwise?
    (void)std::integral_constant<hash_type, entt::hashed_string{"quux"}>{};
    (void)std::integral_constant<hash_type, "quux"_hs>{};
    ASSERT_TRUE(true);
}

TEST(HashedString, ToValue) {
    using hash_type = entt::hashed_string::hash_type;

    const char *foobar = "foobar";

    ASSERT_EQ(entt::hashed_string::to_value(foobar), 0xbf9cf968);
    // how would you test a constexpr otherwise?
    (void)std::integral_constant<hash_type, entt::hashed_string::to_value("quux")>{};
}
