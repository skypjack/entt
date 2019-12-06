#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

template struct entt::family<event, entt::dispatcher::dispatcher_event_family>;

ENTT_API void trigger(int value, entt::dispatcher &dispatcher) {
    dispatcher.trigger<message>(value);
}
