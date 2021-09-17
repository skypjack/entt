#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
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
    receiver receiver;

    dispatcher.trigger<one_more_event>(42);
    dispatcher.enqueue<one_more_event>(42);
    dispatcher.update<one_more_event>();

    dispatcher.sink<an_event>().connect<&receiver::receive>(receiver);
    dispatcher.trigger<an_event>();
    dispatcher.enqueue<an_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.enqueue<another_event>();
    dispatcher.update<another_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<an_event>();
    dispatcher.trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 3);

    dispatcher.enqueue<an_event>();
    dispatcher.clear<an_event>();
    dispatcher.update();

    dispatcher.enqueue<an_event>();
    dispatcher.clear();
    dispatcher.update();

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
