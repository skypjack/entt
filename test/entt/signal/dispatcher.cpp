#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/signal/dispatcher.hpp>

struct an_event {};
struct another_event {};

// makes the type non-aggregate
struct one_more_event {
    one_more_event(int) {}
};

struct receiver {
    static void forward(entt::dispatcher &dispatcher, an_event &event) {
        dispatcher.enqueue(event);
    }

    void receive(const an_event &) {
        ++cnt;
    }

    void reset() {
        cnt = 0;
    }

    int cnt{0};
};

TEST(Dispatcher, Functionalities) {
    entt::dispatcher dispatcher;
    entt::dispatcher other;
    receiver receiver;

    ASSERT_NO_FATAL_FAILURE(entt::dispatcher{std::move(dispatcher)});
    ASSERT_NO_FATAL_FAILURE(dispatcher = std::move(other));

    ASSERT_EQ(dispatcher.size<an_event>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);

    dispatcher.trigger(one_more_event{42});
    dispatcher.enqueue<one_more_event>(42);
    dispatcher.update<one_more_event>();

    dispatcher.sink<an_event>().connect<&receiver::receive>(receiver);
    dispatcher.trigger<an_event>();
    dispatcher.enqueue<an_event>();

    ASSERT_EQ(dispatcher.size<one_more_event>(), 0u);
    ASSERT_EQ(dispatcher.size<an_event>(), 1u);
    ASSERT_EQ(dispatcher.size(), 1u);
    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.enqueue(another_event{});
    dispatcher.update<another_event>();

    ASSERT_EQ(dispatcher.size<another_event>(), 0u);
    ASSERT_EQ(dispatcher.size<an_event>(), 1u);
    ASSERT_EQ(dispatcher.size(), 1u);
    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<an_event>();
    dispatcher.trigger<an_event>();

    ASSERT_EQ(dispatcher.size<an_event>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(receiver.cnt, 3);

    dispatcher.enqueue<an_event>();
    dispatcher.clear<an_event>();
    dispatcher.update();

    dispatcher.enqueue(an_event{});
    dispatcher.clear();
    dispatcher.update();

    ASSERT_EQ(dispatcher.size<an_event>(), 0u);
    ASSERT_EQ(dispatcher.size(), 0u);
    ASSERT_EQ(receiver.cnt, 3);

    receiver.reset();

    an_event event{};

    dispatcher.sink<an_event>().disconnect<&receiver::receive>(receiver);
    dispatcher.trigger<an_event>();
    dispatcher.enqueue(event);
    dispatcher.update();
    dispatcher.trigger(std::as_const(event));

    ASSERT_EQ(receiver.cnt, 0);
}

TEST(Dispatcher, Swap) {
    entt::dispatcher dispatcher;
    entt::dispatcher other;
    receiver receiver;

    dispatcher.sink<an_event>().connect<&receiver::receive>(receiver);
    dispatcher.enqueue<an_event>();

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
    entt::dispatcher dispatcher;
    receiver receiver;

    dispatcher.sink<an_event>().connect<&receiver::forward>(dispatcher);
    dispatcher.sink<an_event>().connect<&receiver::receive>(receiver);

    dispatcher.enqueue<an_event>();
    dispatcher.update();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.sink<an_event>().disconnect<&receiver::forward>(dispatcher);
    dispatcher.update();

    ASSERT_EQ(receiver.cnt, 2);
}

TEST(Dispatcher, OpaqueDisconnect) {
    entt::dispatcher dispatcher;
    receiver receiver;

    dispatcher.sink<an_event>().connect<&receiver::receive>(receiver);
    dispatcher.trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.disconnect(receiver);
    dispatcher.trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 1);
}

TEST(Dispatcher, NamedQueue) {
    using namespace entt::literals;

    entt::dispatcher dispatcher;
    receiver receiver;

    dispatcher.sink<an_event>("named"_hs).connect<&receiver::receive>(receiver);
    dispatcher.trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 0);

    dispatcher.trigger("named"_hs, an_event{});

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.enqueue<an_event>();
    dispatcher.enqueue(an_event{});
    dispatcher.enqueue_hint<an_event>("named"_hs);
    dispatcher.enqueue_hint("named"_hs, an_event{});
    dispatcher.update<an_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.clear<an_event>();
    dispatcher.update<an_event>("named"_hs);

    ASSERT_EQ(receiver.cnt, 3);

    dispatcher.enqueue_hint<an_event>("named"_hs);
    dispatcher.clear<an_event>("named"_hs);
    dispatcher.update<an_event>("named"_hs);

    ASSERT_EQ(receiver.cnt, 3);
}

TEST(Dispatcher, CustomAllocator) {
    std::allocator<void> allocator;
    entt::dispatcher dispatcher{allocator};

    ASSERT_EQ(dispatcher.get_allocator(), allocator);
    ASSERT_FALSE(dispatcher.get_allocator() != allocator);

    dispatcher.enqueue<an_event>();
    decltype(dispatcher) other{std::move(dispatcher), allocator};

    ASSERT_EQ(other.size<an_event>(), 1u);
}
