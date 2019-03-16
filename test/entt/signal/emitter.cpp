#include <gtest/gtest.h>
#include <entt/entity/entt_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter: entt::emitter<test_emitter> {};

struct foo_event { int i; char c; };
struct bar_event {};
struct quux_event {};

ENTT_SHARED_TYPE(foo_event)

TEST(Emitter, Clear) {
    test_emitter emitter;

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<quux_event>());

    emitter.on<foo_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.clear<bar_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.clear<foo_event>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<foo_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());

    emitter.on<foo_event>([](const auto &, const auto &){});
    emitter.on<bar_event>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<foo_event>());
    ASSERT_FALSE(emitter.empty<bar_event>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<foo_event>());
    ASSERT_TRUE(emitter.empty<bar_event>());
}

TEST(Emitter, ClearPublishing) {
    test_emitter emitter;
    bool invoked = false;

    ASSERT_TRUE(emitter.empty());

    emitter.on<bar_event>([&invoked](const auto &, auto &em){
        invoked = true;
        em.clear();
    });

    emitter.publish<bar_event>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(invoked);
}

TEST(Emitter, On) {
    test_emitter emitter;

    emitter.on<foo_event>([](const auto &, const auto &){});

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

    auto conn = emitter.once<foo_event>([](const auto &, const auto &){});

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
