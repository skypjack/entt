#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "common.h"

#include "lib2.hpp"

void trigger_common_payload_event(int payload, entt::dispatcher &dispatcher) {
    dispatcher.trigger<common_payload_event>(payload);
}
