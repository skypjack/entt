#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


#include "../config/config.h"


namespace entt {


/*! @brief Forward declaration of the registry class. */
template <typename>
class basic_registry;

/*! @brief Forward declaration of the view class. */
template<typename, typename...>
class basic_view;

/*! @brief Forward declaration of the runtime view class. */
template<typename>
class basic_runtime_view;

/*! @brief Forward declaration of the group class. */
template<typename...>
class basic_group;

/*! @brief Forward declaration of the actor class. */
template <typename>
struct basic_actor;

/*! @brief Forward declaration of the prototype class. */
template<typename>
class basic_prototype;

/*! @brief Forward declaration of the snapshot class. */
template<typename>
class basic_snapshot;

/*! @brief Forward declaration of the snapshot loader class. */
template<typename>
class basic_snapshot_loader;

/*! @brief Forward declaration of the continuous loader class. */
template<typename>
class basic_continuous_loader;

/*! @brief Alias declaration for the most common use case. */
using entity = ENTT_ENTITY_TYPE;

/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;

/*! @brief Alias declaration for the most common use case. */
using actor = basic_actor<entity>;

/*! @brief Alias declaration for the most common use case. */
using prototype = basic_prototype<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot = basic_snapshot<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot_loader = basic_snapshot_loader<entity>;

/*! @brief Alias declaration for the most common use case. */
using continuous_loader = basic_continuous_loader<entity>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Component Types of components iterated by the view.
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


#endif // ENTT_ENTITY_FWD_HPP
