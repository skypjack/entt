#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>
#include "common.h"

extern void trigger_an_event(int, entt::dispatcher &);
extern void trigger_another_event(entt::dispatcher &);

struct listener {
    void on_an_event(an_event event) { value = event.payload; }
    void on_another_event(another_event) {}

    int value;
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher.sink<an_event>().connect<&listener::on_an_event>(listener);
    dispatcher.sink<another_event>().connect<&listener::on_another_event>(listener);

    listener.value = 0;

    trigger_an_event(3, dispatcher);
    trigger_another_event(dispatcher);

    ASSERT_EQ(listener.value, 3);
}
