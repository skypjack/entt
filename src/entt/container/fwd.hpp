#ifndef ENTT_CONTAINER_FWD_HPP
#define ENTT_CONTAINER_FWD_HPP

#include "../stl/functional.hpp"
#include "../stl/memory.hpp"
#include "../stl/utility.hpp"
#include "../stl/vector.hpp"

namespace entt {

template<
    typename Key,
    typename Type,
    typename = stl::hash<Key>,
    typename = stl::equal_to<>,
    typename = stl::allocator<stl::pair<const Key, Type>>>
class dense_map;

template<
    typename Type,
    typename = stl::hash<Type>,
    typename = stl::equal_to<>,
    typename = stl::allocator<Type>>
class dense_set;

template<typename...>
class basic_table;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Type Element types.
 */
template<typename... Type>
using table = basic_table<stl::vector<Type>...>;

} // namespace entt

#endif
