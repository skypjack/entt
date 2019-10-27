#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "common.h"

ENTT_EXPORT void trigger_another_event(entt::dispatcher &dispatcher) {
    dispatcher.trigger<another_event>();
}
