#include <entt/core/attribute.h>
#include <entt/signal/emitter.hpp>
#include "types.h"

ENTT_API void emit(int value, test_emitter &emitter) {
    emitter.publish<message>(value);
}
