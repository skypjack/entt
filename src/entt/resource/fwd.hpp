#ifndef ENTT_RESOURCE_FWD_HPP
#define ENTT_RESOURCE_FWD_HPP

#include "../config/module.h"

#ifndef ENTT_MODULE
#    include <memory>
#endif // ENTT_MODULE

ENTT_MODULE_EXPORT namespace entt {

template<typename>
struct resource_loader;

template<typename Type, typename = resource_loader<Type>, typename = std::allocator<Type>>
class resource_cache;

template<typename>
class resource;

} // namespace entt

#endif
