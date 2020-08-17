#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Using declaration to be used to _repeat_ the same type a number of
 * times equal to the size of a given parameter pack.
 * @tparam Type A type to repeat.
 */
template<typename Type, typename>
using unpack_as_t = Type;


/**
 * @brief Helper variable template to be used to _repeat_ the same value a
 * number of times equal to the size of a given parameter pack.
 * @tparam Value A value to repeat.
 */
template<auto Value, typename>
inline constexpr auto unpack_as_v = Value;


/**
 * @brief Wraps a static constant.
 * @tparam Value A static constant.
 */
template<auto Value>
using integral_constant = std::integral_constant<decltype(Value), Value>;


/**
 * @brief Alias template to ease the creation of named values.
 * @tparam Value A constant value at least convertible to `id_type`.
 */
template<id_type Value>
using tag = integral_constant<Value>;


/**
 * @brief Utility class to disambiguate overloaded functions.
 * @tparam N Number of choices available.
 */
template<std::size_t N>
struct choice_t
        // Unfortunately, doxygen cannot parse such a construct.
        /*! @cond TURN_OFF_DOXYGEN */
        : choice_t<N-1>
        /*! @endcond */
{};


/*! @copybrief choice_t */
template<>
struct choice_t<0> {};


/**
 * @brief Variable template for the choice trick.
 * @tparam N Number of choices available.
 */
template<std::size_t N>
inline constexpr choice_t<N> choice{};


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
inline constexpr auto type_list_size_v = type_list_size<List>::value;


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
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>
        : std::true_type
{};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


/**
 * @brief Provides the member constant `value` to true if a given type is empty
 * and the empty type optimization is enabled, false otherwise.
 * @tparam Type Potential empty type.
 */
template<typename Type, typename = void>
struct is_eto_eligible
    : ENTT_IS_EMPTY(Type)
{};


/**
 * @brief Helper variable template.
 * @tparam Type Potential empty type.
 */
template<typename Type>
inline constexpr auto is_eto_eligible_v = is_eto_eligible<Type>::value;


/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>, "Invalid pointer type to non-static member object or function");

    template<typename Class, typename Ret, typename... Args>
    static Class * clazz(Ret(Class:: *)(Args...));

    template<typename Class, typename Ret, typename... Args>
    static Class * clazz(Ret(Class:: *)(Args...) const);

    template<typename Class, typename Type>
    static Class * clazz(Type Class:: *);

public:
    /*! @brief The class of the given non-static member object or function. */
    using type = std::remove_pointer_t<decltype(clazz(std::declval<Member>()))>;
};


/**
 * @brief Helper type.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
using member_class_t = typename member_class<Member>::type;


}


#endif
