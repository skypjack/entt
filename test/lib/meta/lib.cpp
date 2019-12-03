#define ENTT_API_EXPORT

#include <entt/core/hashed_string.hpp>
#include <entt/lib/attribute.h>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "types.h"


position create_position(int x, int y) {
    return position{x, y};
}

ENTT_EXPORT void meta_set_up() {
    entt::meta<position>()
            .type("position"_hs)
            .ctor<&create_position>()
            .data<&position::x>("x"_hs)
            .data<&position::y>("y"_hs);

    entt::meta<velocity>()
            .ctor<>()
            .data<&velocity::dx>("dx"_hs)
            .data<&velocity::dy>("dy"_hs);
}

ENTT_EXPORT void meta_tear_down() {
    entt::meta<position>().reset();
    entt::meta<velocity>().reset();
}

ENTT_EXPORT entt::meta_any wrap_int(int value) {
    return value;
}
