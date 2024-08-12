#ifndef ENTT_ENTITY_COMPONENT_HPP
#define ENTT_ENTITY_COMPONENT_HPP

#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "fwd.hpp"

namespace entt {

namespace internal {

    // Concept to check for the presence of a static member `in_place_delete`
    template<typename Type, typename = void>
    concept HasInPlaceDelete = requires {
        { Type::in_place_delete } -> std::convertible_to<bool>;
    };

    // Concept to check for the presence of a static member `page_size`
    template<typename Type, typename = void>
    concept HasPageSize = requires {
        { Type::page_size } -> std::convertible_to<std::size_t>;
    };

    // In-place delete trait with advanced type checks
    template<typename Type, typename = void>
    struct in_place_delete
        : std::bool_constant<!std::is_move_constructible_v<Type> || !std::is_move_assignable_v<Type>> {};

    template<>
    struct in_place_delete<void> : std::false_type {};

    template<typename Type>
    struct in_place_delete<Type, std::enable_if_t<HasInPlaceDelete<Type>>>
        : std::integral_constant<bool, Type::in_place_delete> {};

    // Page size trait with advanced checks and defaulting
    template<typename Type, typename = void>
    struct page_size
        : std::integral_constant<std::size_t, (std::is_empty_v<ENTT_ETO_TYPE(Type)> ? ENTT_PACKED_PAGE : 0)> {};

    template<>
    struct page_size<void> : std::integral_constant<std::size_t, 0u> {};

    template<typename Type>
    struct page_size<Type, std::void_t<decltype(Type::page_size)>>
        : std::integral_constant<std::size_t, Type::page_size> {};

} // namespace internal

/**
 * @brief Traits for accessing various properties of components with advanced type introspection.
 * @tparam Type Component type.
 */
template<typename Type, typename = void>
struct component_traits {
    static_assert(std::is_same_v<std::decay_t<Type>, Type>, "Type must be non-reference and non-const/volatile.");

    /*! @brief Component type. */
    using type = Type;

    /*! @brief Indicates if the component supports in-place deletion, default is `false`. */
    static constexpr bool in_place_delete = internal::in_place_delete<Type>::value;

    /*! @brief Size of the page used for storing components. Defaults to `ENTT_PACKED_PAGE` if unspecified. */
    static constexpr std::size_t page_size = internal::page_size<Type>::value;
};

} // namespace entt

#endif
