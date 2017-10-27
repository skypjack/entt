#include <gtest/gtest.h>
#include <entt/signal/emitter.hpp>

struct TestEmitter: entt::Emitter<TestEmitter> {};

struct FooEvent { int i; char c; };
struct BarEvent {};

TEST(Emitter, Clear) {
    TestEmitter emitter;

    ASSERT_TRUE(emitter.empty());

    emitter.on<FooEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());
    ASSERT_TRUE(emitter.empty<BarEvent>());

    emitter.clear<BarEvent>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());
    ASSERT_TRUE(emitter.empty<BarEvent>());

    emitter.clear<FooEvent>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FooEvent>());
    ASSERT_TRUE(emitter.empty<BarEvent>());

    emitter.on<FooEvent>([](const auto &, const auto &){});
    emitter.on<BarEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());
    ASSERT_FALSE(emitter.empty<BarEvent>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FooEvent>());
    ASSERT_TRUE(emitter.empty<BarEvent>());
}

TEST(Emitter, ClearPublishing) {
    TestEmitter emitter;
    bool invoked = false;

    ASSERT_TRUE(emitter.empty());

    emitter.on<BarEvent>([&invoked](const auto &, auto &em){
        invoked = true;
        em.clear();
    });

    emitter.publish<BarEvent>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(invoked);
}

TEST(Emitter, On) {
    TestEmitter emitter;

    emitter.on<FooEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());

    emitter.publish<FooEvent>(0, 'c');

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());
}

TEST(Emitter, Once) {
    TestEmitter emitter;

    emitter.once<BarEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<BarEvent>());

    emitter.publish<BarEvent>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<BarEvent>());
}

TEST(Emitter, OnceAndErase) {
    TestEmitter emitter;

    auto conn = emitter.once<FooEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FooEvent>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FooEvent>());
}

TEST(Emitter, OnAndErase) {
    TestEmitter emitter;

    auto conn = emitter.on<BarEvent>([](const auto &, const auto &){});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<BarEvent>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<BarEvent>());
}
