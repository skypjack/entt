#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/core/utility.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/sigh.hpp>
#include "types.h"

ENTT_API void trigger(entt::dispatcher &);

struct listener {
    void on(message msg) {
        value = msg.payload;
    }

    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<message>().connect<entt::overload<void(message)>(&listener::on)>(listener);
    trigger(dispatcher);

    ASSERT_EQ(listener.value, 42);
}
