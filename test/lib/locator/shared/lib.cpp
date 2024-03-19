#include <entt/core/attribute.h>
#include <entt/locator/locator.hpp>
#include "../../../common/boxed_type.h"

ENTT_API void set_up(const entt::locator<test::boxed_int>::node_type &handle) {
    entt::locator<test::boxed_int>::reset(handle);
}

ENTT_API void use_service(int value) {
    entt::locator<test::boxed_int>::value().value = value;
}
