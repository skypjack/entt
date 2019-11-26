#include <gtest/gtest.h>
#include <entt/meta/factory.hpp>
#include "types.h"

extern void bind_ctx(entt::meta_ctx);
extern void meta_init();
extern void meta_deinit();

TEST(Lib, Meta) {
    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_FALSE(entt::resolve("int"_hs));
    ASSERT_FALSE(entt::resolve<int>().data("0"_hs));
    ASSERT_FALSE(entt::resolve<char>().data("c"_hs));

    bind_ctx(entt::meta_ctx{});
    entt::meta<double>().type("double"_hs).conv<int>();
    meta_init();

    ASSERT_TRUE(entt::resolve("double"_hs));
    ASSERT_TRUE(entt::resolve("char"_hs));
    ASSERT_TRUE(entt::resolve("int"_hs));
    ASSERT_TRUE(entt::resolve<int>().data("0"_hs));
    ASSERT_TRUE(entt::resolve<char>().data("c"_hs));

    meta_deinit();
    entt::meta<double>().reset();

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_FALSE(entt::resolve("int"_hs));
    ASSERT_FALSE(entt::resolve<int>().data("0"_hs));
    ASSERT_FALSE(entt::resolve<char>().data("c"_hs));
}
