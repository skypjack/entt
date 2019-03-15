#include <entt/core/type_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

ENTT_NAMED_STRUCT(position, {
    int x;
    int y;
})

ENTT_NAMED_STRUCT(velocity, {
    int dx;
    int dy;
})

ENTT_NAMED_STRUCT(an_event, {
    int payload;
})

ENTT_NAMED_STRUCT(another_event, {})
