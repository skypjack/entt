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
 * @brief Specialization used to get rid of constness.
 * @tparam Type Shared type.
 */
template<typename Type>
struct shared_traits<const Type>
        : shared_traits<Type>
{};


/**
 * @brief Helper type.
 * @tparam Type Potentially shared type.
 */
template<typename Type>
using shared_traits_t = typename shared_traits<Type>::type;


/**
 * @brief Provides the member constant `value` to true if a given type is
 * shared. In all other cases, `value` is false.
 */
template<typename, typename = std::void_t<>>
struct is_shared: std::false_type {};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * shared. In all other cases, `value` is false.
 * @tparam Type Potentially shared type.
 */
template<typename Type>
struct is_shared<Type, std::void_t<shared_traits_t<std::decay_t<Type>>>>: std::true_type {};


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


/**
 * @brief Utility macro to deal with an issue of MSVC.
 *
 * See _msvc-doesnt-expand-va-args-correctly_ on SO for all the details.
 *
 * @param args Argument to expand.
 */
#define ENTT_EXPAND(args) args


/**
 * @brief Makes an already existing type a shared type.
 * @param type Type to make shareable.
 */
#define ENTT_SHARED_TYPE(type)\
    template<>\
    struct entt::shared_traits<type>\
        : std::integral_constant<typename entt::hashed_string::hash_type, entt::hashed_string::to_value(#type)>\
    {};


/**
 * @brief Defines a type as shareable (to use for structs).
 * @param clazz Name of the type to make shareable.
 * @param body Body of the type to make shareable.
 */
#define ENTT_SHARED_STRUCT_ONLY(clazz, body)\
    struct clazz body;\
    ENTT_SHARED_TYPE(clazz)


/**
 * @brief Defines a type as shareable (to use for structs).
 * @param ns Namespace where to define the type to make shareable.
 * @param clazz Name of the type to make shareable.
 * @param body Body of the type to make shareable.
 */
#define ENTT_SHARED_STRUCT_WITH_NAMESPACE(ns, clazz, body)\
    namespace ns { struct clazz body; }\
    ENTT_SHARED_TYPE(ns::clazz)


/*! @brief Utility function to simulate macro overloading. */
#define ENTT_SHARED_STRUCT_OVERLOAD(_1, _2, _3, FUNC, ...) FUNC
/*! @brief Defines a type as shareable (to use for structs). */
#define ENTT_SHARED_STRUCT(...) ENTT_EXPAND(ENTT_SHARED_STRUCT_OVERLOAD(__VA_ARGS__, ENTT_SHARED_STRUCT_WITH_NAMESPACE, ENTT_SHARED_STRUCT_ONLY,)(__VA_ARGS__))


/**
 * @brief Defines a type as shareable (to use for classes).
 * @param clazz Name of the type to make shareable.
 * @param body Body of the type to make shareable.
 */
#define ENTT_SHARED_CLASS_ONLY(clazz, body)\
    class clazz body;\
    ENTT_SHARED_TYPE(clazz)


/**
 * @brief Defines a type as shareable (to use for classes).
 * @param ns Namespace where to define the type to make shareable.
 * @param clazz Name of the type to make shareable.
 * @param body Body of the type to make shareable.
 */
#define ENTT_SHARED_CLASS_WITH_NAMESPACE(ns, clazz, body)\
    namespace ns { class clazz body; }\
    ENTT_SHARED_TYPE(ns::clazz)


/*! @brief Utility function to simulate macro overloading. */
#define ENTT_SHARED_CLASS_MACRO(_1, _2, _3, FUNC, ...) FUNC
/*! @brief Defines a type as shareable (to use for classes). */
#define ENTT_SHARED_CLASS(...) ENTT_EXPAND(ENTT_SHARED_CLASS_MACRO(__VA_ARGS__, ENTT_SHARED_CLASS_WITH_NAMESPACE, ENTT_SHARED_CLASS_ONLY,)(__VA_ARGS__))


#endif // ENTT_CORE_TYPE_TRAITS_HPP
