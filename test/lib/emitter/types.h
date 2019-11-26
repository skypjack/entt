#include <entt/core/type_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

ENTT_NAMED_STRUCT(event, {
    int payload;
});
