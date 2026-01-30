#ifndef ENTT_ENTITY_COMPONENT_HPP
#define ENTT_ENTITY_COMPONENT_HPP

#include <concepts>
#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "../core/concepts.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

template<typename Type>
struct in_place_delete: std::bool_constant<!(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>)> {};

template<>
struct in_place_delete<void>: std::false_type {};

template<typename Type>
requires Type::in_place_delete
struct in_place_delete<Type>: std::true_type {};

template<typename Type>
struct page_size: std::integral_constant<std::size_t, !std::is_empty_v<ENTT_ETO_TYPE(Type)> * ENTT_PACKED_PAGE> {};

template<>
struct page_size<void>: std::integral_constant<std::size_t, 0u> {};

template<typename Type>
requires std::is_convertible_v<decltype(Type::page_size), std::size_t>
struct page_size<Type>: std::integral_constant<std::size_t, Type::page_size> {};

} // namespace internal
/*! @endcond */

/**
 * @brief Common way to access various properties of components.
 * @tparam Type Element type.
 * @tparam Entity A valid entity type.
 */
template<cvref_unqualified Type, entity_like Entity, typename>
struct component_traits {
    /*! @brief Element type. */
    using element_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /*! @brief Pointer stability, default is `false`. */
    static constexpr bool in_place_delete = internal::in_place_delete<Type>::value;
    /*! @brief Page size, default is `ENTT_PACKED_PAGE` for non-empty types. */
    static constexpr std::size_t page_size = internal::page_size<Type>::value;
};

} // namespace entt

#endif
