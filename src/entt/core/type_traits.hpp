#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


/*! @brief A class to use to push around lists of types, nothing more. */
template<typename... Type>
struct type_list {};


/*! @brief Traits class used mainly to push things across boundaries. */
template<typename>
struct named_type_traits;


/**
 * @brief Specialization used to get rid of constness.
 * @tparam Type Named type.
 */
template<typename Type>
struct named_type_traits<const Type>
        : named_type_traits<Type>
{};


/**
 * @brief Helper type.
 * @tparam Type Potentially named type.
 */
template<typename Type>
using named_type_traits_t = typename named_type_traits<Type>::type;


/**
 * @brief Provides the member constant `value` to true if a given type has a
 * name. In all other cases, `value` is false.
 */
template<typename, typename = std::void_t<>>
struct is_named_type: std::false_type {};


/**
 * @brief Provides the member constant `value` to true if a given type has a
 * name. In all other cases, `value` is false.
 * @tparam Type Potentially named type.
 */
template<typename Type>
struct is_named_type<Type, std::void_t<named_type_traits_t<std::decay_t<Type>>>>: std::true_type {};


/**
 * @brief Helper variable template.
 *
 * True if a given type has a name, false otherwise.
 *
 * @tparam Type Potentially named type.
 */
template<class Type>
constexpr auto is_named_type_v = is_named_type<Type>::value;


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
 * @brief Makes an already existing type a named type.
 * @param type Type to assign a name to.
 */
#define ENTT_NAMED_TYPE(type)\
    template<>\
    struct entt::named_type_traits<type>\
        : std::integral_constant<typename entt::hashed_string::hash_type, entt::hashed_string::to_value(#type)>\
    {\
        static_assert(std::is_same_v<std::decay_t<type>, type>);\
    };


/**
 * @brief Defines a named type (to use for structs).
 * @param clazz Name of the type to define.
 * @param body Body of the type to define.
 */
#define ENTT_NAMED_STRUCT_ONLY(clazz, body)\
    struct clazz body;\
    ENTT_NAMED_TYPE(clazz)


/**
 * @brief Defines a named type (to use for structs).
 * @param ns Namespace where to define the named type.
 * @param clazz Name of the type to define.
 * @param body Body of the type to define.
 */
#define ENTT_NAMED_STRUCT_WITH_NAMESPACE(ns, clazz, body)\
    namespace ns { struct clazz body; }\
    ENTT_NAMED_TYPE(ns::clazz)


/*! @brief Utility function to simulate macro overloading. */
#define ENTT_NAMED_STRUCT_OVERLOAD(_1, _2, _3, FUNC, ...) FUNC
/*! @brief Defines a named type (to use for structs). */
#define ENTT_NAMED_STRUCT(...) ENTT_EXPAND(ENTT_NAMED_STRUCT_OVERLOAD(__VA_ARGS__, ENTT_NAMED_STRUCT_WITH_NAMESPACE, ENTT_NAMED_STRUCT_ONLY,)(__VA_ARGS__))


/**
 * @brief Defines a named type (to use for classes).
 * @param clazz Name of the type to define.
 * @param body Body of the type to define.
 */
#define ENTT_NAMED_CLASS_ONLY(clazz, body)\
    class clazz body;\
    ENTT_NAMED_TYPE(clazz)


/**
 * @brief Defines a named type (to use for classes).
 * @param ns Namespace where to define the named type.
 * @param clazz Name of the type to define.
 * @param body Body of the type to define.
 */
#define ENTT_NAMED_CLASS_WITH_NAMESPACE(ns, clazz, body)\
    namespace ns { class clazz body; }\
    ENTT_NAMED_TYPE(ns::clazz)


/*! @brief Utility function to simulate macro overloading. */
#define ENTT_NAMED_CLASS_MACRO(_1, _2, _3, FUNC, ...) FUNC
/*! @brief Defines a named type (to use for classes). */
#define ENTT_NAMED_CLASS(...) ENTT_EXPAND(ENTT_NAMED_CLASS_MACRO(__VA_ARGS__, ENTT_NAMED_CLASS_WITH_NAMESPACE, ENTT_NAMED_CLASS_ONLY,)(__VA_ARGS__))


#endif // ENTT_CORE_TYPE_TRAITS_HPP
