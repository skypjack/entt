#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


/*! @brief A class to use to push around lists of types, nothing more. */
template<typename...>
struct type_list {};


/**
 * @brief Concatenates multiple type lists into one.
 * @tparam Type List of types.
 * @return The given type list.
 */
template<typename... Type>
constexpr auto type_list_cat(type_list<Type...> = type_list<>{}) {
    return type_list<Type...>{};
}


/**
 * @brief Concatenates multiple type lists into one.
 * @tparam Type List of types.
 * @tparam Other List of types.
 * @tparam List Type list instances.
 * @return A type list that is the concatenation of the given type lists.
 */
template<typename... Type, typename... Other, typename... List>
constexpr auto type_list_cat(type_list<Type...>, type_list<Other...>, List...) {
    return type_list_cat(type_list<Type..., Other...>{}, List{}...);
}


/*! @brief Traits class used mainly to push things across boundaries. */
template<typename>
struct shared_traits;


/**
 * @brief Makes an already existing type a shared type.
 * @param type Type to make shareable.
 */
#define ENTT_SHARED_TYPE(type)\
    template<>\
    struct shared_traits<type>\
        : std::integral_constant<typename hashed_string::hash_type, hashed_string::to_value(#type)>\
    {}


/**
 * @brief Defines a type as shareable (to use for structs).
 * @param clazz Name of the type to make shareable.
 */
#define ENTT_SHARED_STRUCT(clazz)\
    struct clazz;\
    ENTT_SHARED_TYPE(clazz)\
    struct clazz


/**
 * @brief Defines a type as shareable (to use for classes).
 * @param clazz Name of the type to make shareable.
 */
#define ENTT_SHARED_CLASS(clazz)\
    class clazz;\
    ENTT_SHARED_TYPE(clazz)\
    class clazz


/**
 * @brief Provides the member constant `value` to true if a given type is
 * shared. In all other cases, `value` is false.
 */
template<typename>
struct is_shared: std::false_type {};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * shared. In all other cases, `value` is false.
 * @tparam Type Potentially shared type.
 */
template<typename Type>
struct is_shared<shared_traits<Type>>: std::true_type {};


/**
 * @brief Helper variable template.
 *
 * True if a given type is shared, false otherwise.
 *
 * @tparam Type Potentially shared type.
 */
template<class Type>
constexpr auto is_shared_v = is_shared<Type>::value;


}


#endif // ENTT_CORE_TYPE_TRAITS_HPP
