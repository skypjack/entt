#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/core/utility.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/sigh.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/listener.h"

ENTT_API void trigger(entt::dispatcher &);

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    test::listener<test::boxed_int> listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<test::boxed_int>().connect<entt::overload<void(test::boxed_int)>(&test::listener<test::boxed_int>::on)>(listener);
    trigger(dispatcher);

    ASSERT_EQ(listener.value, 2);
}
