#include <entt/core/attribute.h>
#include "../../../common/boxed_type.h"
#include "../../../common/emitter.h"
#include "../../../common/empty.h"

ENTT_API void emit(test::emitter &emitter) {
    emitter.publish(test::empty{});
    emitter.publish(test::boxed_int{2});
    emitter.publish(test::boxed_int{3});
}
