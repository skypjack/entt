#ifndef ENTT_COMMON_REGISTRY_H
#define ENTT_COMMON_REGISTRY_H

#include <entt/entity/registry.hpp>

namespace test {

template<typename Entity>
struct basic_custom_registry: entt::basic_registry<Entity> {};

} // namespace test

#endif
