#include <entt/config/config.h>
#include "../../../common/emitter.h"
#include "../../../common/value_type.h"
#include "lib.h"

ENTT_API void emit(test::emitter &emitter) {
    emitter.publish(test::empty{});
    emitter.publish(test::boxed_int{2});
    emitter.publish(test::boxed_int{3});
}
