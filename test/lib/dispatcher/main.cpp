#include <gtest/gtest.h>
#include <entt/core/utility.hpp>
#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

ENTT_API void trigger(int, entt::dispatcher &);

struct listener {
    void on(event) {}
    void on(message msg) { value = msg.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher.sink<event>().connect<entt::overload<void(event)>(&listener::on)>(listener);
    dispatcher.sink<message>().connect<entt::overload<void(message)>(&listener::on)>(listener);
    trigger(42, dispatcher);

    ASSERT_EQ(listener.value, 42);
}
