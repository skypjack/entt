#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "../core/hashed_string.hpp"


namespace entt {


/**
 * @brief Utility class to disambiguate overloaded functions.
 * @tparam N Number of choices available.
 */
template<std::size_t N>
struct choice_t
        // Unfortunately, doxygen cannot parse such a construct.
        /*! @cond TURN_OFF_DOXYGEN */
        : choice_t<N-1>
        /*! @endcond TURN_OFF_DOXYGEN */
{};


/*! @copybrief choice_t */
template<>
struct choice_t<0> {};


/**
 * @brief Variable template for the choice trick.
 * @tparam N Number of choices available.
 */
template<std::size_t N>
constexpr choice_t<N> choice{};


/*! @brief A class to use to push around lists of types, nothing more. */
template<typename...>
struct type_list {};


/*! @brief Primary template isn't defined on purpose. */
template<typename>
struct type_list_size;


/**
 * @brief Compile-time number of elements in a type list.
 * @tparam Type Types provided by the type list.
 */
template<typename... Type>
struct type_list_size<type_list<Type...>>
        : std::integral_constant<std::size_t, sizeof...(Type)>
{};


/**
 * @brief Helper variable template.
 * @tparam List Type list.
 */
template<class List>
constexpr auto type_list_size_v = type_list_size<List>::value;


/*! @brief Primary template isn't defined on purpose. */
template<typename...>
struct type_list_cat;


/*! @brief Concatenates multiple type lists. */
template<>
struct type_list_cat<> {
    /*! @brief A type list composed by the types of all the type lists. */
    using type = type_list<>;
};


/**
 * @brief Concatenates multiple type lists.
 * @tparam Type Types provided by the first type list.
 * @tparam Other Types provided by the second type list.
 * @tparam List Other type lists, if any.
 */
template<typename... Type, typename... Other, typename... List>
struct type_list_cat<type_list<Type...>, type_list<Other...>, List...> {
    /*! @brief A type list composed by the types of all the type lists. */
    using type = typename type_list_cat<type_list<Type..., Other...>, List...>::type;
};


/**
 * @brief Concatenates multiple type lists.
 * @tparam Type Types provided by the type list.
 */
template<typename... Type>
struct type_list_cat<type_list<Type...>> {
    /*! @brief A type list composed by the types of all the type lists. */
    using type = type_list<Type...>;
};


/**
 * @brief Helper type.
 * @tparam List Type lists to concatenate.
 */
template<typename... List>
using type_list_cat_t = typename type_list_cat<List...>::type;


/*! @brief Primary template isn't defined on purpose. */
template<typename>
struct type_list_unique;


/**
 * @brief Removes duplicates types from a type list.
 * @tparam Type One of the types provided by the given type list.
 * @tparam Other The other types provided by the given type list.
 */
template<typename Type, typename... Other>
struct type_list_unique<type_list<Type, Other...>> {
    /*! @brief A type list without duplicate types. */
    using type = std::conditional_t<
        std::disjunction_v<std::is_same<Type, Other>...>,
        typename type_list_unique<type_list<Other...>>::type,
        type_list_cat_t<type_list<Type>, typename type_list_unique<type_list<Other...>>::type>
    >;
};


/*! @brief Removes duplicates types from a type list. */
template<>
struct type_list_unique<type_list<>> {
    /*! @brief A type list without duplicate types. */
    using type = type_list<>;
};


/**
 * @brief Helper type.
 * @tparam Type A type list.
 */
template<typename Type>
using type_list_unique_t = typename type_list_unique<Type>::type;


/**
 * @brief Provides the member constant `value` to true if a given type is
 * equality comparable, false otherwise.
 * @tparam Type Potentially equality comparable type.
 */
template<typename Type, typename = std::void_t<>>
struct is_equality_comparable: std::false_type {};


/*! @copydoc is_equality_comparable */
template<typename Type>
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


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
 * @brief Helper variable template.
 * @tparam Type Potentially named type.
 */
template<class Type>
constexpr auto named_type_traits_v = named_type_traits<Type>::value;


/**
 * @brief Provides the member constant `value` to true if a given type has a
 * name. In all other cases, `value` is false.
 * @tparam Type Potentially named type.
 */
template<typename Type, typename = std::void_t<>>
struct is_named_type: std::false_type {};


/*! @copydoc is_named_type */
template<typename Type>
struct is_named_type<Type, std::void_t<named_type_traits_t<std::decay_t<Type>>>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially named type.
 */
template<class Type>
constexpr auto is_named_type_v = is_named_type<Type>::value;


/**
 * @brief Defines an enum class to use for opaque identifiers and a dedicate
 * `to_integer` function to convert the identifiers to their underlying type.
 * @param clazz The name to use for the enum class.
 * @param type The underlying type for the enum class.
 */
#define ENTT_OPAQUE_TYPE(clazz, type)\
    enum class clazz: type {};\
    constexpr auto to_integer(const clazz id) ENTT_NOEXCEPT {\
        return std::underlying_type_t<clazz>(id);\
    }\
    static_assert(true)


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
 *
 * The current definition contains a workaround for Clang 6 because it fails to
 * deduce correctly the type to use to specialize the class template.<br/>
 * With a compiler that fully supports C++17 and works fine with deduction
 * guides, the following should be fine instead:
 *
 * @code{.cpp}
 * std::integral_constant<ENTT_ID_TYPE, entt::basic_hashed_string{#type}>
 * @endcode
 *
 * In order to support even sligthly older compilers, I prefer to stick to the
 * implementation below.
 *
 * @param type Type to assign a name to.
 */
#define ENTT_NAMED_TYPE(type)\
    template<>\
    struct entt::named_type_traits<type>\
        : std::integral_constant<ENTT_ID_TYPE, entt::basic_hashed_string<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(#type)>>>>{#type}>\
    {\
        static_assert(std::is_same_v<std::remove_cv_t<type>, type>);\
        static_assert(std::is_object_v<type>);\
    }


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
