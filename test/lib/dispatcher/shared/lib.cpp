#include <entt/config/config.h>
#include <entt/signal/dispatcher.hpp>
#include "../../../common/value_type.h"
#include "lib.h"

ENTT_API void trigger(entt::dispatcher &dispatcher) {
    dispatcher.trigger(test::empty{});
    dispatcher.trigger(test::boxed_int{2});
}
