#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


#include <memory>
#include "../core/fwd.hpp"


namespace entt {


template<typename Entity, typename = std::allocator<Entity>>
class basic_sparse_set;


template<typename, typename Type, typename = std::allocator<Type>>
struct basic_storage;


template<typename>
class basic_registry;


template<typename...>
struct basic_view;


template<typename>
class basic_runtime_view;


template<typename...>
class basic_group;


template<typename>
class basic_observer;


template<typename>
class basic_organizer;


template<typename, typename...>
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
using sparse_set = basic_sparse_set<entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using storage = basic_storage<entity, Args...>;


/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;


/*! @brief Alias declaration for the most common use case. */
using observer = basic_observer<entity>;


/*! @brief Alias declaration for the most common use case. */
using organizer = basic_organizer<entity>;


/*! @brief Alias declaration for the most common use case. */
using handle = basic_handle<entity>;


/*! @brief Alias declaration for the most common use case. */
using const_handle = basic_handle<const entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using handle_view = basic_handle<entity, Args...>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using const_handle_view = basic_handle<const entity, Args...>;


/*! @brief Alias declaration for the most common use case. */
using snapshot = basic_snapshot<entity>;


/*! @brief Alias declaration for the most common use case. */
using snapshot_loader = basic_snapshot_loader<entity>;


/*! @brief Alias declaration for the most common use case. */
using continuous_loader = basic_continuous_loader<entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using view = basic_view<entity, Args...>;


/*! @brief Alias declaration for the most common use case. */
using runtime_view = basic_runtime_view<entity>;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using group = basic_group<entity, Args...>;


}


#endif
