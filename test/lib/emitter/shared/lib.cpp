#include <entt/core/attribute.h>
#include "types.h"

ENTT_API void emit(test_emitter &emitter) {
    emitter.publish(event{});
    emitter.publish(message{42});
    emitter.publish(message{3});
}
