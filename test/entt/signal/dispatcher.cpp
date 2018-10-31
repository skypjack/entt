#include <memory>
#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>

struct an_event {};
struct another_event {};

struct receiver {
    void receive(const an_event &) { ++cnt; }
    void reset() { cnt = 0; }
    int cnt{0};
};

TEST(Dispatcher, Functionalities) {
    entt::dispatcher dispatcher;
    receiver receiver;

    dispatcher.sink<an_event>().connect(&receiver);
    dispatcher.trigger<an_event>();
    dispatcher.enqueue<an_event>();
    dispatcher.enqueue<another_event>();
    dispatcher.update<another_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<an_event>();
    dispatcher.trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 3);

    receiver.reset();

    an_event event{};
    const an_event &cevent = event;

    dispatcher.sink<an_event>().disconnect(&receiver);
    dispatcher.trigger(an_event{});
    dispatcher.enqueue(event);
    dispatcher.update();
    dispatcher.trigger(cevent);

    ASSERT_EQ(receiver.cnt, 0);
}
