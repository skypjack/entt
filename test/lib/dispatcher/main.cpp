#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

extern void trigger_event(int, entt::dispatcher &);

struct listener {
    void on_int(int) { FAIL(); }
    void on_event(event ev) { value = ev.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher.sink<int>().connect<&listener::on_int>(listener);
    dispatcher.sink<event>().connect<&listener::on_event>(listener);
    trigger_event(42, dispatcher);

    ASSERT_EQ(listener.value, 42);
}
