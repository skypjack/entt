#include <entt/core/attribute.h>
#include "../common/types.h"

ENTT_API void emit(test_emitter &emitter) {
    emitter.publish(event{});
    emitter.publish(message{2});
    emitter.publish(message{3});
}
