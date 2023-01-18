#include <entt/core/attribute.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "types.h"

position create_position(int x, int y) {
    return position{x, y};
}

ENTT_API void share(entt::locator<entt::meta_ctx>::node_type handle) {
    entt::locator<entt::meta_ctx>::reset(handle);
}

ENTT_API void set_up() {
    using namespace entt::literals;

    entt::meta<position>()
        .type("position"_hs)
        .ctor<&create_position>()
        .data<&position::x>("x"_hs)
        .data<&position::y>("y"_hs);

    entt::meta<velocity>()
        .type("velocity"_hs)
        .ctor<>()
        .data<&velocity::dx>("dx"_hs)
        .data<&velocity::dy>("dy"_hs);
}

ENTT_API void tear_down() {
    entt::meta_reset<position>();
    entt::meta_reset<velocity>();
}

ENTT_API entt::meta_any wrap_int(int value) {
    return value;
}
