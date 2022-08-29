#include <entt/core/attribute.h>
#include <entt/locator/locator.hpp>
#include "types.h"

ENTT_API void set_up(const entt::locator<service>::node_type &handle) {
    entt::locator<service>::reset(handle);
}

ENTT_API void use_service(int value) {
    entt::locator<service>::value().value = value;
}
