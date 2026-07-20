#ifndef ENTT_RESOURCE_FWD_HPP
#define ENTT_RESOURCE_FWD_HPP

#include "../stl/memory.hpp"

namespace entt {

template<typename>
struct resource_loader;

template<typename Type, typename = resource_loader<Type>, typename = stl::allocator<Type>>
class resource_cache;

template<typename>
class resource;

} // namespace entt

#endif
