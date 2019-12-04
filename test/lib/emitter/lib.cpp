#define ENTT_API_EXPORT

#include <entt/lib/attribute.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

template struct entt::family<event, test_emitter::emitter_event_family>;

ENTT_EXPORT void emit(int value, test_emitter &emitter) {
    emitter.publish<message>(value);
}
