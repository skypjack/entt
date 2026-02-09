#ifndef ENTT_RESOURCE_FWD_HPP
#define ENTT_RESOURCE_FWD_HPP

#include <memory>
#include "../core/concepts.hpp"

namespace entt {

template<typename>
struct resource_loader;

template<typename Type, typename = resource_loader<Type>, allocator_like = std::allocator<Type>>
class resource_cache;

template<typename>
class resource;

} // namespace entt

#endif
