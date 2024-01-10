#include <entt/core/attribute.h>
#include "../common/types.h"

ENTT_API void emit(test_emitter &emitter) {
    emitter.publish(event{});
    emitter.publish(message{42}); // NOLINT
    emitter.publish(message{3});  // NOLINT
}
