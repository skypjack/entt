#ifndef ENTT_ENTITY_COMPONENT_HPP
#define ENTT_ENTITY_COMPONENT_HPP

#include <cstddef>
#include <type_traits>
#include "../config/config.h"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type, typename = void>
struct in_place_delete: std::bool_constant<!(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>)> {};

template<typename Type>
struct in_place_delete<Type, std::enable_if_t<Type::in_place_delete>>
    : std::true_type {};

template<typename Type, typename = void>
struct page_size: std::integral_constant<std::size_t, !std::is_empty_v<ENTT_ETO_TYPE(Type)> * ENTT_PACKED_PAGE> {};

template<typename Type>
struct page_size<Type, std::enable_if_t<std::is_convertible_v<decltype(Type::page_size), std::size_t>>>
    : std::integral_constant<std::size_t, Type::page_size> {};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Common way to access various properties of components.
 * @tparam Type Type of component.
 */
template<typename Type, typename = void>
struct component_traits {
    static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, Type>, "Unsupported type");

    /*! @brief Component type. */
    using type = Type;

    /*! @brief Pointer stability, default is `false`. */
    static constexpr bool in_place_delete = internal::in_place_delete<Type>::value;
    /*! @brief Page size, default is `ENTT_PACKED_PAGE` for non-empty types. */
    static constexpr std::size_t page_size = internal::page_size<Type>::value;
};

/**
 * @brief Helper variable template.
 * @tparam Type Type of component.
 */
template<class Type>
inline constexpr bool ignore_as_empty_v = (std::is_void_v<Type> || component_traits<Type>::page_size == 0u);

} // namespace entt

#endif
