#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>

#include "common.h"

#include "lib1.hpp"
#include "lib2.hpp"


struct listener {
	template<typename PayloadEvent>
    void on_payload_event(PayloadEvent event) { value = event.payload; }

	template<typename EmptyEvent>
    void on_empty_event(EmptyEvent) {}

    int value;
};

TEST(Lib, CommonEventTypes) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher
		.sink<common_payload_event>()
		.connect<&listener::on_payload_event<common_payload_event>>(listener);
    dispatcher
		.sink<common_empty_event>()
		.connect<&listener::on_empty_event<common_empty_event>>(listener);

    listener.value = 0;

    trigger_common_payload_event(3, dispatcher);
    trigger_common_empty_event(dispatcher);

    ASSERT_EQ(listener.value, 3);
}
