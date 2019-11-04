#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "lib2.hpp"

ENTT_DISPATCHER_TYPE_IMPL(lib2_empty_event);
ENTT_DISPATCHER_TYPE_IMPL(lib2_payload_event);

void trigger_common_payload_event(int payload, entt::dispatcher &dispatcher) {
    dispatcher.trigger<common_payload_event>(payload);
}

void trigger_common_empty_event(entt::dispatcher &dispatcher) {
    dispatcher.trigger<common_empty_event>();
}

void trigger_lib2_empty_event(entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib2_empty_event>();
}

void trigger_lib2_payload_plus_2_event(int payload, entt::dispatcher &dispatcher) {
	dispatcher.trigger<lib2_payload_event>(payload + 2);
}
