#include <functional>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <common/boxed_type.h>
#include <common/emitter.h>
#include <common/linter.hpp>
#include <entt/signal/emitter.hpp>

struct bar_event {};
struct quux_event {};

TEST(Emitter, Move) {
    test::emitter emitter{};
    emitter.on<test::boxed_int>([](auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());

    test::emitter other{std::move(emitter)};

    test::is_initialized(emitter);

    ASSERT_FALSE(other.empty());
    ASSERT_TRUE(other.contains<test::boxed_int>());
    ASSERT_TRUE(emitter.empty());

    emitter = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());
    ASSERT_TRUE(other.empty());
}

TEST(Emitter, Swap) {
    test::emitter emitter{};
    test::emitter other{};
    int value{};

    emitter.on<test::boxed_int>([&value](auto &event, const auto &) {
        value = event.value;
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(other.empty());

    emitter.swap(other);
    emitter.publish(test::boxed_int{1});

    ASSERT_EQ(value, 0);
    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(other.empty());

    other.publish(test::boxed_int{1});

    ASSERT_EQ(value, 1);
}

TEST(Emitter, Clear) {
    test::emitter emitter{};

    ASSERT_TRUE(emitter.empty());

    emitter.on<test::boxed_int>([](auto &, const auto &) {});
    emitter.on<quux_event>([](const auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.erase<bar_event>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.erase<test::boxed_int>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.contains<test::boxed_int>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_FALSE(emitter.contains<bar_event>());

    emitter.on<test::boxed_int>([](auto &, const auto &) {});
    emitter.on<bar_event>([](const auto &, const auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());
    ASSERT_TRUE(emitter.contains<quux_event>());
    ASSERT_TRUE(emitter.contains<bar_event>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(emitter.contains<test::boxed_int>());
    ASSERT_FALSE(emitter.contains<bar_event>());
}

TEST(Emitter, ClearFromCallback) {
    test::emitter emitter{};

    ASSERT_TRUE(emitter.empty());

    emitter.on<test::boxed_int>([](auto &, auto &owner) {
        owner.template on<test::boxed_int>([](auto &, auto &) {});
        owner.template erase<test::boxed_int>();
    });

    emitter.on<bar_event>([](const auto &, auto &owner) {
        owner.template on<bar_event>([](const auto &, auto &) {});
        owner.template erase<bar_event>();
    });

    ASSERT_FALSE(emitter.empty());

    emitter.publish(test::boxed_int{});
    emitter.publish(bar_event{});

    ASSERT_TRUE(emitter.empty());
}

TEST(Emitter, On) {
    test::emitter emitter{};
    int value{};

    emitter.on<test::boxed_int>([&value](auto &event, const auto &) {
        value = event.value;
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_TRUE(emitter.contains<test::boxed_int>());
    ASSERT_EQ(value, 0);

    emitter.publish(test::boxed_int{1});

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

    emitter.on<test::boxed_int>([](auto &, const auto &) {});
    const decltype(emitter) other{std::move(emitter), allocator};

    test::is_initialized(emitter);

    ASSERT_TRUE(emitter.empty());
    ASSERT_FALSE(other.empty());
}
