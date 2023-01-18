#include <entt/core/attribute.h>
#include <entt/signal/dispatcher.hpp>
#include "../common/types.h"

ENTT_API void trigger(entt::dispatcher &dispatcher) {
    dispatcher.trigger<event>();
    dispatcher.trigger(message{42});
}
