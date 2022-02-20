#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/core/utility.hpp>

struct functions {
    static void foo(int) {}
    static void foo() {}

    void bar(int) {}
    void bar() {}
};

TEST(Identity, Functionalities) {
    entt::identity identity;
    int value = 42;

    ASSERT_TRUE(entt::is_transparent_v<entt::identity>);
    ASSERT_EQ(identity(value), value);
    ASSERT_EQ(&identity(value), &value);
}

TEST(Overload, Functionalities) {
    ASSERT_EQ(entt::overload<void(int)>(&functions::foo), static_cast<void (*)(int)>(&functions::foo));
    ASSERT_EQ(entt::overload<void()>(&functions::foo), static_cast<void (*)()>(&functions::foo));

    ASSERT_EQ(entt::overload<void(int)>(&functions::bar), static_cast<void (functions::*)(int)>(&functions::bar));
    ASSERT_EQ(entt::overload<void()>(&functions::bar), static_cast<void (functions::*)()>(&functions::bar));

    functions instance;

    ASSERT_NO_FATAL_FAILURE(entt::overload<void(int)>(&functions::foo)(0));
    ASSERT_NO_FATAL_FAILURE(entt::overload<void()>(&functions::foo)());

    ASSERT_NO_FATAL_FAILURE((instance.*entt::overload<void(int)>(&functions::bar))(0));
    ASSERT_NO_FATAL_FAILURE((instance.*entt::overload<void()>(&functions::bar))());
}

TEST(Overloaded, Functionalities) {
    int iv = 0;
    char cv = '\0';

    entt::overloaded func{
        [&iv](int value) { iv = value; },
        [&cv](char value) { cv = value; }};

    func(42);
    func('c');

    ASSERT_EQ(iv, 42);
    ASSERT_EQ(cv, 'c');
}

TEST(YCombinator, Functionalities) {
    entt::y_combinator gauss([](const auto &self, auto value) -> unsigned int {
        return value ? (value + self(value - 1u)) : 0;
    });

    ASSERT_EQ(gauss(3u), 3u * 4u / 2u);
    ASSERT_EQ(std::as_const(gauss)(7u), 7u * 8u / 2u);
}
