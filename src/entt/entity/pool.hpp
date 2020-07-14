#ifndef ENTT_ENTITY_POOL_HPP
#define ENTT_ENTITY_POOL_HPP


#include <type_traits>
#include "storage.hpp"


namespace entt {


/**
 * @brief Applies component-to-pool conversion and defines the resulting type as
 * the member typedef type.
 *
 * Formally:
 *
 * * If the component type is a non-const one, the member typedef type is the
 *   declared storage type.
 * * If the component type is a const one, the member typedef type is the
 *   declared storage type, except it has a const-qualifier added.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = void>
struct pool {
    /*! @brief Resulting type after component-to-pool conversion. */
    using type = storage<Entity, Type>;
};


/*! @copydoc pool */
template<typename Entity, typename Type>
struct pool<Entity, const Type> {
    /*! @brief Resulting type after component-to-pool conversion. */
    using type = std::add_const_t<typename pool<Entity, std::remove_const_t<Type>>::type>;
};


/**
 * @brief Alias declaration to use to make component-to-pool conversions.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
using pool_t = typename pool<Entity, Type>::type;


}


#endif
