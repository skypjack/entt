#ifndef ENTT_ENTITY_COMPONENT_HPP
#define ENTT_ENTITY_COMPONENT_HPP

#include <type_traits>
#include "../config/config.h"

namespace entt {

/*! @brief Commonly used default traits for all types. */
struct basic_component_traits {
    /*! @brief Pointer stability, default is `std::false_type`. */
    using in_place_delete = std::false_type;
    /*! @brief Empty type optimization, default is `ENTT_IGNORE_IF_EMPTY`. */
    using ignore_if_empty = ENTT_IGNORE_IF_EMPTY;
};

/**
 * @brief Common way to access various properties of components.
 * @tparam Type Type of component.
 */
template<typename Type, typename = void>
struct component_traits: basic_component_traits {
    static_assert(std::is_same_v<std::decay_t<Type>, Type>, "Unsupported type");
};

/**
 * @brief Helper variable template.
 * @tparam Type Type of component.
 */
template<class Type>
inline constexpr bool in_place_delete_v = component_traits<Type>::in_place_delete::value;

/**
 * @brief Helper variable template.
 * @tparam Type Type of component.
 */
template<class Type>
inline constexpr bool ignore_as_empty_v = component_traits<Type>::ignore_if_empty::value &&std::is_empty_v<Type>;

} // namespace entt

#endif
