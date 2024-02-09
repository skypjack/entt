#ifndef ENTT_COMMON_EMITTER_H
#define ENTT_COMMON_EMITTER_H

#include <entt/signal/emitter.hpp>

namespace test {

struct emitter: entt::emitter<emitter> {
    using entt::emitter<emitter>::emitter;
};

} // namespace test

#endif
