#ifndef ENTT_META_TYPE_TRAITS_HPP
#define ENTT_META_TYPE_TRAITS_HPP

#include <type_traits>
#include <utility>

namespace entt {

/**
 * @brief Traits class template to be specialized to enable support for meta
 * template information.
 */
template<typename>
struct meta_template_traits;

/**
 * @brief Traits class template to be specialized to enable support for meta
 * sequence containers.
 */
template<typename>
struct meta_sequence_container_traits;

/**
 * @brief Traits class template to be specialized to enable support for meta
 * associative containers.
 */
template<typename>
struct meta_associative_container_traits;

/**
 * @brief Provides the member constant `value` to true if a given type is a
 * pointer-like type from the point of view of the meta system, false otherwise.
 * @tparam Type Potentially pointer-like type.
 */
template<typename>
struct is_meta_pointer_like: std::false_type {};

/**
 * @brief Partial specialization to ensure that const pointer-like types are
 * also accepted.
 * @tparam Type Potentially pointer-like type.
 */
template<typename Type>
struct is_meta_pointer_like<const Type>: is_meta_pointer_like<Type> {};

/**
 * @brief Helper variable template.
 * @tparam Type Potentially pointer-like type.
 */
template<typename Type>
inline constexpr auto is_meta_pointer_like_v = is_meta_pointer_like<Type>::value;

} // namespace entt

#endif
