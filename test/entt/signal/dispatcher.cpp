#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/signal/dispatcher.hpp>
#include "../../common/empty.h"

// makes the type non-aggregate
struct non_aggregate {
    non_aggregate(int) {}
};

struct receiver {
    static void forward(entt::dispatcher &dispatcher, test::empty &event) {
        dispatcher.enqueue(event);
    }

    void receive(const test::empty &) {
        ++cnt;
    }

    void reset() {
        cnt = 0;
    }

    int cnt{0};
};

TEST(Dispatcher, Functionalities) {
    entt::dispatcher dispatcher{};
    entt::dispatcher other{std::move(dispatcher)};

    dispatcher = std::move(other);

    receiver receiver{};

    ASSERT_EQ(dispatcher.size<test::empty>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);

    dispatcher.trigger(non_aggregate{1});
    dispatcher.enqueue<non_aggregate>(2);
    dispatcher.update<non_aggregate>();

    dispatcher.sink<test::empty>().connect<&receiver::receive>(receiver);
    dispatcher.trigger<test::empty>();
    dispatcher.enqueue<test::empty>();

    ASSERT_EQ(dispatcher.size<non_aggregate>(), 0u);
    ASSERT_EQ(dispatcher.size<test::empty>(), 1u);
    ASSERT_EQ(dispatcher.size(), 1u);
    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.enqueue(test::other_empty{});
    dispatcher.update<test::other_empty>();

    ASSERT_EQ(dispatcher.size<test::other_empty>(), 0u);
    ASSERT_EQ(dispatcher.size<test::empty>(), 1u);
    ASSERT_EQ(dispatcher.size(), 1u);
    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<test::empty>();
    dispatcher.trigger<test::empty>();

    ASSERT_EQ(dispatcher.size<test::empty>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(receiver.cnt, 3);

    dispatcher.enqueue<test::empty>();
    dispatcher.clear<test::empty>();
    dispatcher.update();

    dispatcher.enqueue(test::empty{});
    dispatcher.clear();
    dispatcher.update();

    ASSERT_EQ(dispatcher.size<test::empty>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(receiver.cnt, 3);

    receiver.reset();

    test::empty event{};

    dispatcher.sink<test::empty>().disconnect<&receiver::receive>(receiver);
    dispatcher.trigger<test::empty>();
    dispatcher.enqueue(event);
    dispatcher.update();
    dispatcher.trigger(std::as_const(event));

    ASSERT_EQ(receiver.cnt, 0);
}

TEST(Dispatcher, Swap) {
    entt::dispatcher dispatcher{};
    entt::dispatcher other{};
    receiver receiver{};

    dispatcher.sink<test::empty>().connect<&receiver::receive>(receiver);
    dispatcher.enqueue<test::empty>();

    ASSERT_EQ(dispatcher.size(), 1u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(receiver.cnt, 0);

    dispatcher.swap(other);
    dispatcher.update();

    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(receiver.cnt, 0);

    other.update();

    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(receiver.cnt, 1);
}

TEST(Dispatcher, StopAndGo) {
    entt::dispatcher dispatcher{};
    receiver receiver{};

    dispatcher.sink<test::empty>().connect<&receiver::forward>(dispatcher);
    dispatcher.sink<test::empty>().connect<&receiver::receive>(receiver);

    dispatcher.enqueue<test::empty>();
    dispatcher.update();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.sink<test::empty>().disconnect<&receiver::forward>(dispatcher);
    dispatcher.update();

    ASSERT_EQ(receiver.cnt, 2);
}

TEST(Dispatcher, OpaqueDisconnect) {
    entt::dispatcher dispatcher{};
    receiver receiver{};

    dispatcher.sink<test::empty>().connect<&receiver::receive>(receiver);
    dispatcher.trigger<test::empty>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.disconnect(receiver);
    dispatcher.trigger<test::empty>();

    ASSERT_EQ(receiver.cnt, 1);
}

TEST(Dispatcher, NamedQueue) {
    using namespace entt::literals;

    entt::dispatcher dispatcher{};
    receiver receiver{};

    dispatcher.sink<test::empty>("named"_hs).connect<&receiver::receive>(receiver);
    dispatcher.trigger<test::empty>();

    ASSERT_EQ(receiver.cnt, 0);

    dispatcher.trigger("named"_hs, test::empty{});

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.enqueue<test::empty>();
    dispatcher.enqueue(test::empty{});
    dispatcher.enqueue_hint<test::empty>("named"_hs);
    dispatcher.enqueue_hint("named"_hs, test::empty{});
    dispatcher.update<test::empty>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.clear<test::empty>();
    dispatcher.update<test::empty>("named"_hs);

    ASSERT_EQ(receiver.cnt, 3);

    dispatcher.enqueue_hint<test::empty>("named"_hs);
    dispatcher.clear<test::empty>("named"_hs);
    dispatcher.update<test::empty>("named"_hs);

    ASSERT_EQ(receiver.cnt, 3);
}

TEST(Dispatcher, CustomAllocator) {
    const std::allocator<void> allocator{};
    entt::dispatcher dispatcher{allocator};

    ASSERT_EQ(dispatcher.get_allocator(), allocator);
    ASSERT_FALSE(dispatcher.get_allocator() != allocator);

    dispatcher.enqueue<test::empty>();
    const decltype(dispatcher) other{std::move(dispatcher), allocator};

    ASSERT_EQ(other.size<test::empty>(), 1u);
}
