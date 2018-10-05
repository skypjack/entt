#include <gtest/gtest.h>
#include <entt/core/utility.hpp>

struct Functions {
    static void foo(int) {}
    static void foo() {}

    void bar(int) {}
    void bar() {}
};


TEST(Utility, Overload) {
    ASSERT_EQ(entt::overload<void(int)>(&Functions::foo), static_cast<void(*)(int)>(&Functions::foo));
    ASSERT_EQ(entt::overload<void()>(&Functions::foo), static_cast<void(*)()>(&Functions::foo));

    ASSERT_EQ(entt::overload<void(int)>(&Functions::bar), static_cast<void(Functions:: *)(int)>(&Functions::bar));
    ASSERT_EQ(entt::overload<void()>(&Functions::bar), static_cast<void(Functions:: *)()>(&Functions::bar));
}
