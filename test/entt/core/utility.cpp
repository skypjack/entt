#include <utility>
#include <gtest/gtest.h>
#include <entt/core/utility.hpp>

struct Functions {
    static void foo(int) {}
    static void foo() {}

    void bar(int) {}
    void bar() {}
};

TEST(Utility, Identity) {
    entt::identity identity;
    int value = 42;

    ASSERT_EQ(identity(value), value);
    ASSERT_EQ(&identity(value), &value);
}

TEST(Utility, Overload) {
    ASSERT_EQ(entt::overload<void(int)>(&Functions::foo), static_cast<void(*)(int)>(&Functions::foo));
    ASSERT_EQ(entt::overload<void()>(&Functions::foo), static_cast<void(*)()>(&Functions::foo));

    ASSERT_EQ(entt::overload<void(int)>(&Functions::bar), static_cast<void(Functions:: *)(int)>(&Functions::bar));
    ASSERT_EQ(entt::overload<void()>(&Functions::bar), static_cast<void(Functions:: *)()>(&Functions::bar));

    Functions instance;

    ASSERT_NO_THROW(entt::overload<void(int)>(&Functions::foo)(0));
    ASSERT_NO_THROW(entt::overload<void()>(&Functions::foo)());

    ASSERT_NO_THROW((instance.*entt::overload<void(int)>(&Functions::bar))(0));
    ASSERT_NO_THROW((instance.*entt::overload<void()>(&Functions::bar))());
}

TEST(Utility, Overloaded) {
    int iv = 0;
    char cv = '\0';

    entt::overloaded func{
        [&iv](int value) { iv = value; },
        [&cv](char value) { cv = value; }
    };

    func(42);
    func('c');

    ASSERT_EQ(iv, 42);
    ASSERT_EQ(cv, 'c');
}

TEST(Utility, YCombinator) {
    entt::y_combinator gauss([](auto &&self, unsigned int value) -> unsigned int {
        return value ? (value + self(value-1u)) : 0;
    });

    ASSERT_EQ(gauss(3u), 3u*4u/2u);
    ASSERT_EQ(std::as_const(gauss)(7u), 7u*8u/2u);
}
