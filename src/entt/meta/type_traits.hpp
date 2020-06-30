#ifndef ENTT_META_TYPE_TRAITS_HPP
#define ENTT_META_TYPE_TRAITS_HPP


#include <type_traits>


namespace entt {


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
 * @brief Provides the member constant `value` to true if support for meta
 * sequence containers is enabled for the given type, false otherwise.
 * @tparam Type Potentially sequence container type.
 */
template<typename Type, typename = void>
struct has_meta_sequence_container_traits: std::false_type {};


/*! @copydoc has_meta_sequence_container_traits */
template<typename Type>
struct has_meta_sequence_container_traits<Type, std::void_t<typename meta_sequence_container_traits<Type>::value_type>>
        : std::true_type
{};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially sequence container type.
 */
template<typename Type>
inline constexpr auto has_meta_sequence_container_traits_v = has_meta_sequence_container_traits<Type>::value;


/**
 * @brief Provides the member constant `value` to true if support for meta
 * associative containers is enabled for the given type, false otherwise.
 * @tparam Type Potentially associative container type.
 */
template<typename, typename = void>
struct has_meta_associative_container_traits: std::false_type {};


/*! @copydoc has_meta_associative_container_traits */
template<typename Type>
struct has_meta_associative_container_traits<Type, std::void_t<typename meta_associative_container_traits<Type>::key_type>>
        : std::true_type
{};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially associative container type.
 */
template<typename Type>
inline constexpr auto has_meta_associative_container_traits_v = has_meta_associative_container_traits<Type>::value;


/**
 * @brief Provides the member constant `value` to true if a meta associative
 * container claims to wrap a key-only type, false otherwise.
 * @tparam Type Potentially key-only meta associative container type.
 */
template<typename, typename = void>
struct is_key_only_meta_associative_container: std::true_type {};


/*! @copydoc is_key_only_meta_associative_container */
template<typename Type>
struct is_key_only_meta_associative_container<Type, std::void_t<typename meta_associative_container_traits<Type>::mapped_type>>
        : std::false_type
{};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially key-only meta associative container type.
 */
template<typename Type>
inline constexpr auto is_key_only_meta_associative_container_v = is_key_only_meta_associative_container<Type>::value;


/**
 * @brief Provides the member constant `value` to true if a given type is a
 * pointer-like type from the point of view of the meta system, false otherwise.
 * @tparam Type Potentially pointer-like type.
 */
template<typename>
struct is_meta_pointer_like: std::false_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially pointer-like type.
 */
template<typename Type>
inline constexpr auto is_meta_pointer_like_v = is_meta_pointer_like<Type>::value;


}


#endif
