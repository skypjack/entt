#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "common.h"

ENTT_EXPORT void trigger_an_event(int payload, entt::dispatcher &dispatcher) {
    dispatcher.trigger<an_event>(payload);
}
