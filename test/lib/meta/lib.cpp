#include <entt/core/attribute.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "types.h"

position create_position(int x, int y) {
    return position{x, y};
}

ENTT_API void set_up() {
    entt::meta<position>()
            .alias("position"_hs)
            .ctor<&create_position>()
            .data<&position::x>("x"_hs)
            .data<&position::y>("y"_hs);

    entt::meta<velocity>()
            .alias("velocity"_hs)
            .ctor<>()
            .data<&velocity::dx>("dx"_hs)
            .data<&velocity::dy>("dy"_hs);
}

ENTT_API void tear_down() {
    entt::meta<position>().reset();
    entt::meta<velocity>().reset();
}

ENTT_API entt::meta_any wrap_int(int value) {
    return value;
}
