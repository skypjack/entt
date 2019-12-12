#include <entt/core/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

ENTT_API void trigger(int value, entt::dispatcher &dispatcher) {
    dispatcher.trigger<message>(value);
}
