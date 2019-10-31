#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "common.h"

#include "lib1.hpp"


void trigger_common_empty_event(entt::dispatcher &dispatcher){
    dispatcher.trigger<common_empty_event>();
}
