#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


#include "../core/fwd.hpp"


namespace entt {


template <typename>
class basic_registry;


template<typename...>
class basic_view;


template<typename>
class basic_runtime_view;


template<typename...>
class basic_group;


template<typename>
class basic_observer;


template <typename>
struct basic_actor;


template<typename>
struct basic_handle;


template<typename>
class basic_snapshot;


template<typename>
class basic_snapshot_loader;


template<typename>
class basic_continuous_loader;


/*! @brief Default entity identifier. */
enum class entity: id_type {};


/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;


/*! @brief Alias declaration for the most common use case. */
using observer = basic_observer<entity>;


/*! @brief Alias declaration for the most common use case. */
using actor [[deprecated("Consider using the handle class instead")]] = basic_actor<entity>;


/*! @brief Alias declaration for the most common use case. */
using handle = basic_handle<entity>;


/*! @brief Alias declaration for the most common use case. */
using const_handle = basic_handle<const entity>;


/*! @brief Alias declaration for the most common use case. */
using snapshot = basic_snapshot<entity>;


/*! @brief Alias declaration for the most common use case. */
using snapshot_loader = basic_snapshot_loader<entity>;


/*! @brief Alias declaration for the most common use case. */
using continuous_loader = basic_continuous_loader<entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Types Types of components iterated by the view.
 */
template<typename... Types>
using view = basic_view<entity, Types...>;


/*! @brief Alias declaration for the most common use case. */
using runtime_view = basic_runtime_view<entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Types Types of components iterated by the group.
 */
template<typename... Types>
using group = basic_group<entity, Types...>;


}


#endif
