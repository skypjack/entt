#include <memory>
#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>

struct AnEvent {};
struct AnotherEvent {};

struct Receiver {
    void receive(const AnEvent &) { ++cnt; }
    void reset() { cnt = 0; }
    int cnt{0};
};

TEST(Dispatcher, Functionalities) {
    entt::Dispatcher dispatcher;
    Receiver receiver;

    dispatcher.template sink<AnEvent>().connect(&receiver);
    dispatcher.template trigger<AnEvent>();
    dispatcher.template enqueue<AnEvent>();
    dispatcher.template enqueue<AnotherEvent>();
    dispatcher.update<AnotherEvent>();

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<AnEvent>();
    dispatcher.template trigger<AnEvent>();

    ASSERT_EQ(receiver.cnt, 3);

    receiver.reset();

    dispatcher.template sink<AnEvent>().disconnect(&receiver);
    dispatcher.template trigger<AnEvent>();
    dispatcher.template enqueue<AnEvent>();
    dispatcher.update();
    dispatcher.template trigger<AnEvent>();

    ASSERT_EQ(receiver.cnt, 0);
}
