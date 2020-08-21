#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter: entt::emitter<test_emitter> {};

struct foo_event { int i; char c; };
struct bar_event {};
struct quux_event {};

TEST(Emitter, Clear) {
    test_emitter emitter;

    ASSERT_TRUE(emitter.empty());

    emitter.on<foo_event>([](auto &, const auto &){});
    emitter.once<quux_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_FALSE(emitter.empty<quux_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.clear<bar_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_FALSE(emitter.empty<quux_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.clear<foo_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.empty<foo_event>());
    ASSERT_FALSE(emitter.empty<quux_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.on<foo_event>([](auto &, const auto &){});
    emitter.on<bar_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_FALSE(emitter.empty<quux_event>());
    ASSERT_FALSE(emitter.empty<bar_event>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<foo_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());
}

TEST(Emitter, ClearPublishing) {
    test_emitter emitter;

    ASSERT_TRUE(emitter.empty());

    emitter.once<foo_event>([](auto &, auto &em){
        em.template once<foo_event>([](auto &, auto &){});
        em.template clear<foo_event>();
    });

    emitter.on<bar_event>([](const auto &, auto &em){
        em.template once<bar_event>([](const auto &, auto &){});
        em.template clear<bar_event>();
    });

    ASSERT_FALSE(emitter.empty());

    emitter.publish<foo_event>();
    emitter.publish<bar_event>();

    ASSERT_TRUE(emitter.empty());
}

TEST(Emitter, On) {
    test_emitter emitter;

    emitter.on<foo_event>([](auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());

    emitter.publish<foo_event>(0, 'c');

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
}

TEST(Emitter, Once) {
    test_emitter emitter;

    emitter.once<bar_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<bar_event>());

    emitter.publish<bar_event>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<bar_event>());
}

TEST(Emitter, OnceAndErase) {
    test_emitter emitter;

    auto conn = emitter.once<foo_event>([](auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<foo_event>());
}

TEST(Emitter, OnAndErase) {
    test_emitter emitter;

    auto conn = emitter.on<bar_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<bar_event>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<bar_event>());
}
