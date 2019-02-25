#include <entt/core/type_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

ENTT_SHARED_STRUCT(position, {
    int x;
    int y;
})

ENTT_SHARED_STRUCT(velocity, {
    int dx;
    int dy;
})

ENTT_SHARED_STRUCT(an_event, {
    int payload;
})

ENTT_SHARED_STRUCT(another_event, {})
