#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "lib1.hpp"


void trigger_common_empty_event(entt::dispatcher &dispatcher) {
    dispatcher.trigger<common_empty_event>();
}

void trigger_lib1_empty_event(entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib1_empty_event>();
}

void trigger_lib1_payload_plus_1_event(int payload, entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib1_payload_event>(payload + 1);
}
