#include <functional>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <common/emitter.h>
#include <common/linter.hpp>
#include <entt/signal/emitter.hpp>

struct foo_event {
    int i;
};

struct bar_event {};
struct quux_event {};

TEST(Emitter, Move) {
    test::emitter emitter{};
    emitter.on<foo_event>([](auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());

    test::emitter other{std::move(emitter)};

    test::is_initialized(emitter);

    ASSERT_FALSE(other.empty());
    ASSERT_TRUE(other.contains<foo_event>());
    ASSERT_TRUE(emitter.empty());

    emitter = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());
    ASSERT_TRUE(other.empty());
}

TEST(Emitter, Swap) {
    test::emitter emitter{};
    test::emitter other{};
    int value{};

    emitter.on<foo_event>([&value](auto &event, const auto &) {
        value = event.i;
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(other.empty());

    emitter.swap(other);
    emitter.publish(foo_event{1});

    ASSERT_EQ(value, 0);
    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(other.empty());

    other.publish(foo_event{1});

    ASSERT_EQ(value, 1);
}

TEST(Emitter, Clear) {
    test::emitter emitter{};

    ASSERT_TRUE(emitter.empty());

    emitter.on<foo_event>([](auto &, const auto &) {});
    emitter.on<quux_event>([](const auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.erase<bar_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.erase<foo_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.contains<foo_event>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.on<foo_event>([](auto &, const auto &) {});
    emitter.on<bar_event>([](const auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_TRUE(emitter.contains<bar_event>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(emitter.contains<foo_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());
}

TEST(Emitter, ClearFromCallback) {
    test::emitter emitter{};

    ASSERT_TRUE(emitter.empty());

    emitter.on<foo_event>([](auto &, auto &owner) {
        owner.template on<foo_event>([](auto &, auto &) {});
        owner.template erase<foo_event>();
    });

    emitter.on<bar_event>([](const auto &, auto &owner) {
        owner.template on<bar_event>([](const auto &, auto &) {});
        owner.template erase<bar_event>();
    });

    ASSERT_FALSE(emitter.empty());

    emitter.publish(foo_event{});
    emitter.publish(bar_event{});

    ASSERT_TRUE(emitter.empty());
}

TEST(Emitter, On) {
    test::emitter emitter{};
    int value{};

    emitter.on<foo_event>([&value](auto &event, const auto &) {
        value = event.i;
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<foo_event>());
    ASSERT_EQ(value, 0);

    emitter.publish(foo_event{1});

    ASSERT_EQ(value, 1);
}

TEST(Emitter, OnAndErase) {
    test::emitter emitter{};
    const std::function<void(bar_event &, test::emitter &)> func{};

    emitter.on(func);

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<bar_event>());

    emitter.erase<bar_event>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(emitter.contains<bar_event>());
}

TEST(Emitter, CustomAllocator) {
    const std::allocator<void> allocator{};
    test::emitter emitter{allocator};

    ASSERT_EQ(emitter.get_allocator(), allocator);
    ASSERT_FALSE(emitter.get_allocator() != allocator);

    emitter.on<foo_event>([](auto &, const auto &) {});
    const decltype(emitter) other{std::move(emitter), allocator};

    test::is_initialized(emitter);

    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(other.empty());
}
