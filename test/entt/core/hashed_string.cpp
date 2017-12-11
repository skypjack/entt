#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>

constexpr bool check(const char *str) {
    using hash_type = entt::HashedString::hash_type;

    return (static_cast<hash_type>(entt::HashedString{str}) == entt::HashedString{str}
            && static_cast<const char *>(entt::HashedString{str}) == str
            && entt::HashedString{str} == entt::HashedString{str}
            && !(entt::HashedString{str} != entt::HashedString{str}));
}

TEST(HashedString, Constexprness) {
    // how would you test a constepxr otherwise?
    static_assert(check("foobar"), "!");
    ASSERT_TRUE(true);
}

TEST(HashedString, Functionalities) {
    using hash_type = entt::HashedString::hash_type;

    const char *bar = "bar";

    auto fooHs = entt::HashedString{"foo"};
    auto barHs = entt::HashedString{bar};

    ASSERT_NE(static_cast<hash_type>(fooHs), static_cast<hash_type>(barHs));
    ASSERT_EQ(static_cast<const char *>(fooHs), "foo");
    ASSERT_EQ(static_cast<const char *>(barHs), bar);

    ASSERT_TRUE(fooHs == fooHs);
    ASSERT_FALSE(fooHs == barHs);

    entt::HashedString hs{"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hs), 0x85944171f73967e8);
}
