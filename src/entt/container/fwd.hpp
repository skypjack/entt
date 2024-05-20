#ifndef ENTT_CONTAINER_FWD_HPP
#define ENTT_CONTAINER_FWD_HPP

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace entt {

template<
    typename Key,
    typename Type,
    typename = std::hash<Key>,
    typename = std::equal_to<>,
    typename = std::allocator<std::pair<const Key, Type>>>
class dense_map;

template<
    typename Type,
    typename = std::hash<Type>,
    typename = std::equal_to<>,
    typename = std::allocator<Type>>
class dense_set;

template<typename...>
class basic_table;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Type Element types.
 */
template<typename... Type>
using table = basic_table<std::vector<Type>...>;

} // namespace entt

#endif
