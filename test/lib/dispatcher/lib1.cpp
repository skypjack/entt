#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "lib1.hpp"


ENTT_NO_EXPORT
void trigger_lib1_payload_plus_1_hidden_event(
	int payload, entt::dispatcher &dispatcher);

struct lib1_payload_hidden_event {
	int payload;
};

struct ENTT_NO_EXPORT listener {
	template<typename PayloadEvent>
    void on_payload_event(PayloadEvent event) {
		trigger_lib1_payload_plus_1_hidden_event (event.payload+1, *dispatcher);
	}

	template<typename EmptyEvent>
    void on_empty_event(EmptyEvent) {}

    ::entt::dispatcher* dispatcher;
};


void trigger_common_empty_event(entt::dispatcher &dispatcher) {
    dispatcher.trigger<common_empty_event>();
}

void trigger_lib1_empty_event(entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib1_empty_event>();
}

void trigger_lib1_payload_plus_1_event(int payload, entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib1_payload_event>(payload + 1);
}

void trigger_lib1_payload_plus_1_hidden_event(
	int payload, entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib1_payload_event>(payload + 1);
}

void trigger_lib1_payload_plus_3_event(int payload, entt::dispatcher &dispatcher) {
    listener listener{&dispatcher};

    dispatcher
		.sink<lib1_payload_hidden_event>()
		.connect<&listener::on_payload_event<lib1_payload_hidden_event>>(listener);

	dispatcher.trigger<lib1_payload_hidden_event>(payload + 1);
}
