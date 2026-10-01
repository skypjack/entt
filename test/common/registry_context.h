#ifndef ENTT_COMMON_REGISTRY_CONTEXT_H
#define ENTT_COMMON_REGISTRY_CONTEXT_H

#include <cstdint>
#include <entt/entity/registry.hpp>

namespace test {

template<typename Type>
struct registry_context_value {
    registry_context_value();
    registry_context_value *insert(entt::registry &);
};

using registry_context_type = registry_context_value<std::uint32_t>;

registry_context_type *registry_context_insert(entt::registry &);

} // namespace test

#endif
