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

    dispatcher.template sink<an_event>().connect(&receiver);
    dispatcher.template trigger<an_event>();
    dispatcher.template enqueue<an_event>();
    dispatcher.template enqueue<another_event>();
    dispatcher.update<another_event>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<an_event>();
    dispatcher.template trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 3);

    receiver.reset();

    dispatcher.template sink<an_event>().disconnect(&receiver);
    dispatcher.template trigger<an_event>();
    dispatcher.template enqueue<an_event>();
    dispatcher.update();
    dispatcher.template trigger<an_event>();

    ASSERT_EQ(receiver.cnt, 0);
}
