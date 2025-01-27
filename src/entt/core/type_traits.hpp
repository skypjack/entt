#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Utility class to disambiguate overloaded functions.
 * @tparam N Number of choices available.
 */
template<std::size_t N>
struct choice_t
    // unfortunately, doxygen cannot parse such a construct
    : /*! @cond TURN_OFF_DOXYGEN */ choice_t<N - 1> /*! @endcond */
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

/**
 * @brief Identity type trait.
 *
 * Useful to establish non-deduced contexts in template argument deduction
 * (waiting for C++20) or to provide types through function arguments.
 *
 * @tparam Type A type.
 */
template<typename Type>
struct type_identity {
    /*! @brief Identity type. */
    using type = Type;
};

/**
 * @brief Helper type.
 * @tparam Type A type.
 */
template<typename Type>
using type_identity_t = typename type_identity<Type>::type;

/**
 * @brief A type-only `sizeof` wrapper that returns 0 where `sizeof` complains.
 * @tparam Type The type of which to return the size.
 */
template<typename Type, typename = void>
struct size_of: std::integral_constant<std::size_t, 0u> {};

/*! @copydoc size_of */
template<typename Type>
struct size_of<Type, std::void_t<decltype(sizeof(Type))>>
    // NOLINTNEXTLINE(bugprone-sizeof-expression)
    : std::integral_constant<std::size_t, sizeof(Type)> {};

/**
 * @brief Helper variable template.
 * @tparam Type The type of which to return the size.
 */
template<typename Type>
inline constexpr std::size_t size_of_v = size_of<Type>::value;

/**
 * @brief Using declaration to be used to _repeat_ the same type a number of
 * times equal to the size of a given parameter pack.
 * @tparam Type A type to repeat.
 */
template<typename Type, typename>
using unpack_as_type = Type;

/**
 * @brief Helper variable template to be used to _repeat_ the same value a
 * number of times equal to the size of a given parameter pack.
 * @tparam Value A value to repeat.
 */
template<auto Value, typename>
inline constexpr auto unpack_as_value = Value;

/**
 * @brief Wraps a static constant.
 * @tparam Value A static constant.
 */
template<auto Value>
using integral_constant = std::integral_constant<decltype(Value), Value>;

/**
 * @brief Alias template to facilitate the creation of named values.
 * @tparam Value A constant value at least convertible to `id_type`.
 */
template<id_type Value>
using tag = integral_constant<Value>;

/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Type list type. */
    using type = type_list;
    /*! @brief Compile-time number of elements in the type list. */
    static constexpr auto size = sizeof...(Type);
};

/*! @brief Primary template isn't defined on purpose. */
template<std::size_t, typename>
struct type_list_element;

/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Index Index of the type to return.
 * @tparam First First type provided by the type list.
 * @tparam Other Other types provided by the type list.
 */
template<std::size_t Index, typename First, typename... Other>
struct type_list_element<Index, type_list<First, Other...>>
    : type_list_element<Index - 1u, type_list<Other...>> {};

/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam First First type provided by the type list.
 * @tparam Other Other types provided by the type list.
 */
template<typename First, typename... Other>
struct type_list_element<0u, type_list<First, Other...>> {
    /*! @brief Searched type. */
    using type = First;
};

/**
 * @brief Helper type.
 * @tparam Index Index of the type to return.
 * @tparam List Type list to search into.
 */
template<std::size_t Index, typename List>
using type_list_element_t = typename type_list_element<Index, List>::type;

/*! @brief Primary template isn't defined on purpose. */
template<typename, typename>
struct type_list_index;

/**
 * @brief Provides compile-time type access to the types of a type list.
 * @tparam Type Type to look for and for which to return the index.
 * @tparam First First type provided by the type list.
 * @tparam Other Other types provided by the type list.
 */
template<typename Type, typename First, typename... Other>
struct type_list_index<Type, type_list<First, Other...>> {
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given type in the sublist. */
    static constexpr value_type value = 1u + type_list_index<Type, type_list<Other...>>::value;
};

/**
 * @brief Provides compile-time type access to the types of a type list.
 * @tparam Type Type to look for and for which to return the index.
 * @tparam Other Other types provided by the type list.
 */
template<typename Type, typename... Other>
struct type_list_index<Type, type_list<Type, Other...>> {
    static_assert(type_list_index<Type, type_list<Other...>>::value == sizeof...(Other), "Non-unique type");
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given type in the sublist. */
    static constexpr value_type value = 0u;
};

/**
 * @brief Provides compile-time type access to the types of a type list.
 * @tparam Type Type to look for and for which to return the index.
 */
template<typename Type>
struct type_list_index<Type, type_list<>> {
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given type in the sublist. */
    static constexpr value_type value = 0u;
};

/**
 * @brief Helper variable template.
 * @tparam List Type list.
 * @tparam Type Type to look for and for which to return the index.
 */
template<typename Type, typename List>
inline constexpr std::size_t type_list_index_v = type_list_index<Type, List>::value;

/**
 * @brief Concatenates multiple type lists.
 * @tparam Type Types provided by the first type list.
 * @tparam Other Types provided by the second type list.
 * @return A type list composed by the types of both the type lists.
 */
template<typename... Type, typename... Other>
constexpr type_list<Type..., Other...> operator+(type_list<Type...>, type_list<Other...>) {
    return {};
}

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

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename...>
struct type_list_unique;

template<typename First, typename... Other, typename... Type>
struct type_list_unique<type_list<First, Other...>, Type...>
    : std::conditional_t<(std::is_same_v<First, Type> || ...), type_list_unique<type_list<Other...>, Type...>, type_list_unique<type_list<Other...>, Type..., First>> {};

template<typename... Type>
struct type_list_unique<type_list<>, Type...> {
    using type = type_list<Type...>;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Removes duplicates types from a type list.
 * @tparam List Type list.
 */
template<typename List>
struct type_list_unique {
    /*! @brief A type list without duplicate types. */
    using type = typename internal::type_list_unique<List>::type;
};

/**
 * @brief Helper type.
 * @tparam List Type list.
 */
template<typename List>
using type_list_unique_t = typename type_list_unique<List>::type;

/**
 * @brief Provides the member constant `value` to true if a type list contains a
 * given type, false otherwise.
 * @tparam List Type list.
 * @tparam Type Type to look for.
 */
template<typename List, typename Type>
struct type_list_contains;

/**
 * @copybrief type_list_contains
 * @tparam Type Types provided by the type list.
 * @tparam Other Type to look for.
 */
template<typename... Type, typename Other>
struct type_list_contains<type_list<Type...>, Other>
    : std::bool_constant<(std::is_same_v<Type, Other> || ...)> {};

/**
 * @brief Helper variable template.
 * @tparam List Type list.
 * @tparam Type Type to look for.
 */
template<typename List, typename Type>
inline constexpr bool type_list_contains_v = type_list_contains<List, Type>::value;

/*! @brief Primary template isn't defined on purpose. */
template<typename...>
struct type_list_diff;

/**
 * @brief Computes the difference between two type lists.
 * @tparam Type Types provided by the first type list.
 * @tparam Other Types provided by the second type list.
 */
template<typename... Type, typename... Other>
struct type_list_diff<type_list<Type...>, type_list<Other...>> {
    /*! @brief A type list that is the difference between the two type lists. */
    using type = type_list_cat_t<std::conditional_t<type_list_contains_v<type_list<Other...>, Type>, type_list<>, type_list<Type>>...>;
};

/**
 * @brief Helper type.
 * @tparam List Type lists between which to compute the difference.
 */
template<typename... List>
using type_list_diff_t = typename type_list_diff<List...>::type;

/*! @brief Primary template isn't defined on purpose. */
template<typename, template<typename...> class>
struct type_list_transform;

/**
 * @brief Applies a given _function_ to a type list and generate a new list.
 * @tparam Type Types provided by the type list.
 * @tparam Op Unary operation as template class with a type member named `type`.
 */
template<typename... Type, template<typename...> class Op>
struct type_list_transform<type_list<Type...>, Op> {
    /*! @brief Resulting type list after applying the transform function. */
    // NOLINTNEXTLINE(modernize-type-traits)
    using type = type_list<typename Op<Type>::type...>;
};

/**
 * @brief Helper type.
 * @tparam List Type list.
 * @tparam Op Unary operation as template class with a type member named `type`.
 */
template<typename List, template<typename...> class Op>
using type_list_transform_t = typename type_list_transform<List, Op>::type;

/**
 * @brief A class to use to push around lists of constant values, nothing more.
 * @tparam Value Values provided by the value list.
 */
template<auto... Value>
struct value_list {
    /*! @brief Value list type. */
    using type = value_list;
    /*! @brief Compile-time number of elements in the value list. */
    static constexpr auto size = sizeof...(Value);
};

/*! @brief Primary template isn't defined on purpose. */
template<std::size_t, typename>
struct value_list_element;

/**
 * @brief Provides compile-time indexed access to the values of a value list.
 * @tparam Index Index of the value to return.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<std::size_t Index, auto Value, auto... Other>
struct value_list_element<Index, value_list<Value, Other...>>
    : value_list_element<Index - 1u, value_list<Other...>> {};

/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<auto Value, auto... Other>
struct value_list_element<0u, value_list<Value, Other...>> {
    /*! @brief Searched type. */
    using type = decltype(Value);
    /*! @brief Searched value. */
    static constexpr auto value = Value;
};

/**
 * @brief Helper type.
 * @tparam Index Index of the type to return.
 * @tparam List Value list to search into.
 */
template<std::size_t Index, typename List>
using value_list_element_t = typename value_list_element<Index, List>::type;

/**
 * @brief Helper type.
 * @tparam Index Index of the value to return.
 * @tparam List Value list to search into.
 */
template<std::size_t Index, typename List>
inline constexpr auto value_list_element_v = value_list_element<Index, List>::value;

/*! @brief Primary template isn't defined on purpose. */
template<auto, typename>
struct value_list_index;

/**
 * @brief Provides compile-time type access to the values of a value list.
 * @tparam Value Value to look for and for which to return the index.
 * @tparam First First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<auto Value, auto First, auto... Other>
struct value_list_index<Value, value_list<First, Other...>> {
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given value in the sublist. */
    static constexpr value_type value = 1u + value_list_index<Value, value_list<Other...>>::value;
};

/**
 * @brief Provides compile-time type access to the values of a value list.
 * @tparam Value Value to look for and for which to return the index.
 * @tparam Other Other values provided by the value list.
 */
template<auto Value, auto... Other>
struct value_list_index<Value, value_list<Value, Other...>> {
    static_assert(value_list_index<Value, value_list<Other...>>::value == sizeof...(Other), "Non-unique type");
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given value in the sublist. */
    static constexpr value_type value = 0u;
};

/**
 * @brief Provides compile-time type access to the values of a value list.
 * @tparam Value Value to look for and for which to return the index.
 */
template<auto Value>
struct value_list_index<Value, value_list<>> {
    /*! @brief Unsigned integer type. */
    using value_type = std::size_t;
    /*! @brief Compile-time position of the given type in the sublist. */
    static constexpr value_type value = 0u;
};

/**
 * @brief Helper variable template.
 * @tparam List Value list.
 * @tparam Value Value to look for and for which to return the index.
 */
template<auto Value, typename List>
inline constexpr std::size_t value_list_index_v = value_list_index<Value, List>::value;

/**
 * @brief Concatenates multiple value lists.
 * @tparam Value Values provided by the first value list.
 * @tparam Other Values provided by the second value list.
 * @return A value list composed by the values of both the value lists.
 */
template<auto... Value, auto... Other>
constexpr value_list<Value..., Other...> operator+(value_list<Value...>, value_list<Other...>) {
    return {};
}

/*! @brief Primary template isn't defined on purpose. */
template<typename...>
struct value_list_cat;

/*! @brief Concatenates multiple value lists. */
template<>
struct value_list_cat<> {
    /*! @brief A value list composed by the values of all the value lists. */
    using type = value_list<>;
};

/**
 * @brief Concatenates multiple value lists.
 * @tparam Value Values provided by the first value list.
 * @tparam Other Values provided by the second value list.
 * @tparam List Other value lists, if any.
 */
template<auto... Value, auto... Other, typename... List>
struct value_list_cat<value_list<Value...>, value_list<Other...>, List...> {
    /*! @brief A value list composed by the values of all the value lists. */
    using type = typename value_list_cat<value_list<Value..., Other...>, List...>::type;
};

/**
 * @brief Concatenates multiple value lists.
 * @tparam Value Values provided by the value list.
 */
template<auto... Value>
struct value_list_cat<value_list<Value...>> {
    /*! @brief A value list composed by the values of all the value lists. */
    using type = value_list<Value...>;
};

/**
 * @brief Helper type.
 * @tparam List Value lists to concatenate.
 */
template<typename... List>
using value_list_cat_t = typename value_list_cat<List...>::type;

/*! @brief Primary template isn't defined on purpose. */
template<typename>
struct value_list_unique;

/**
 * @brief Removes duplicates values from a value list.
 * @tparam Value One of the values provided by the given value list.
 * @tparam Other The other values provided by the given value list.
 */
template<auto Value, auto... Other>
struct value_list_unique<value_list<Value, Other...>> {
    /*! @brief A value list without duplicate types. */
    using type = std::conditional_t<
        ((Value == Other) || ...),
        typename value_list_unique<value_list<Other...>>::type,
        value_list_cat_t<value_list<Value>, typename value_list_unique<value_list<Other...>>::type>>;
};

/*! @brief Removes duplicates values from a value list. */
template<>
struct value_list_unique<value_list<>> {
    /*! @brief A value list without duplicate types. */
    using type = value_list<>;
};

/**
 * @brief Helper type.
 * @tparam Type A value list.
 */
template<typename Type>
using value_list_unique_t = typename value_list_unique<Type>::type;

/**
 * @brief Provides the member constant `value` to true if a value list contains
 * a given value, false otherwise.
 * @tparam List Value list.
 * @tparam Value Value to look for.
 */
template<typename List, auto Value>
struct value_list_contains;

/**
 * @copybrief value_list_contains
 * @tparam Value Values provided by the value list.
 * @tparam Other Value to look for.
 */
template<auto... Value, auto Other>
struct value_list_contains<value_list<Value...>, Other>
    : std::bool_constant<((Value == Other) || ...)> {};

/**
 * @brief Helper variable template.
 * @tparam List Value list.
 * @tparam Value Value to look for.
 */
template<typename List, auto Value>
inline constexpr bool value_list_contains_v = value_list_contains<List, Value>::value;

/*! @brief Primary template isn't defined on purpose. */
template<typename...>
struct value_list_diff;

/**
 * @brief Computes the difference between two value lists.
 * @tparam Value Values provided by the first value list.
 * @tparam Other Values provided by the second value list.
 */
template<auto... Value, auto... Other>
struct value_list_diff<value_list<Value...>, value_list<Other...>> {
    /*! @brief A value list that is the difference between the two value lists. */
    using type = value_list_cat_t<std::conditional_t<value_list_contains_v<value_list<Other...>, Value>, value_list<>, value_list<Value>>...>;
};

/**
 * @brief Helper type.
 * @tparam List Value lists between which to compute the difference.
 */
template<typename... List>
using value_list_diff_t = typename value_list_diff<List...>::type;

/*! @brief Same as std::is_invocable, but with tuples. */
template<typename, typename>
struct is_applicable: std::false_type {};

/**
 * @copybrief is_applicable
 * @tparam Func A valid function type.
 * @tparam Tuple Tuple-like type.
 * @tparam Args The list of arguments to use to probe the function type.
 */
template<typename Func, template<typename...> class Tuple, typename... Args>
struct is_applicable<Func, Tuple<Args...>>: std::is_invocable<Func, Args...> {};

/**
 * @copybrief is_applicable
 * @tparam Func A valid function type.
 * @tparam Tuple Tuple-like type.
 * @tparam Args The list of arguments to use to probe the function type.
 */
template<typename Func, template<typename...> class Tuple, typename... Args>
struct is_applicable<Func, const Tuple<Args...>>: std::is_invocable<Func, Args...> {};

/**
 * @brief Helper variable template.
 * @tparam Func A valid function type.
 * @tparam Args The list of arguments to use to probe the function type.
 */
template<typename Func, typename Args>
inline constexpr bool is_applicable_v = is_applicable<Func, Args>::value;

/*! @brief Same as std::is_invocable_r, but with tuples for arguments. */
template<typename, typename, typename>
struct is_applicable_r: std::false_type {};

/**
 * @copybrief is_applicable_r
 * @tparam Ret The type to which the return type of the function should be
 * convertible.
 * @tparam Func A valid function type.
 * @tparam Args The list of arguments to use to probe the function type.
 */
template<typename Ret, typename Func, typename... Args>
struct is_applicable_r<Ret, Func, std::tuple<Args...>>: std::is_invocable_r<Ret, Func, Args...> {};

/**
 * @brief Helper variable template.
 * @tparam Ret The type to which the return type of the function should be
 * convertible.
 * @tparam Func A valid function type.
 * @tparam Args The list of arguments to use to probe the function type.
 */
template<typename Ret, typename Func, typename Args>
inline constexpr bool is_applicable_r_v = is_applicable_r<Ret, Func, Args>::value;

/**
 * @brief Provides the member constant `value` to true if a given type is
 * complete, false otherwise.
 * @tparam Type The type to test.
 */
template<typename Type, typename = void>
struct is_complete: std::false_type {};

/*! @copydoc is_complete */
template<typename Type>
struct is_complete<Type, std::void_t<decltype(sizeof(Type))>>: std::true_type {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_complete_v = is_complete<Type>::value;

/**
 * @brief Provides the member constant `value` to true if a given type is an
 * iterator, false otherwise.
 * @tparam Type The type to test.
 */
template<typename Type, typename = void>
struct is_iterator: std::false_type {};

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename = void>
struct has_iterator_category: std::false_type {};

template<typename Type>
struct has_iterator_category<Type, std::void_t<typename std::iterator_traits<Type>::iterator_category>>: std::true_type {};

} // namespace internal
/*! @endcond */

/*! @copydoc is_iterator */
template<typename Type>
struct is_iterator<Type, std::enable_if_t<!std::is_void_v<std::remove_cv_t<std::remove_pointer_t<Type>>>>>
    : internal::has_iterator_category<Type> {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_iterator_v = is_iterator<Type>::value;

/**
 * @brief Provides the member constant `value` to true if a given type is both
 * an empty and non-final class, false otherwise.
 * @tparam Type The type to test
 */
template<typename Type>
struct is_ebco_eligible
    : std::bool_constant<std::is_empty_v<Type> && !std::is_final_v<Type>> {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_ebco_eligible_v = is_ebco_eligible<Type>::value;

/**
 * @brief Provides the member constant `value` to true if `Type::is_transparent`
 * is valid and denotes a type, false otherwise.
 * @tparam Type The type to test.
 */
template<typename Type, typename = void>
struct is_transparent: std::false_type {};

/*! @copydoc is_transparent */
template<typename Type>
struct is_transparent<Type, std::void_t<typename Type::is_transparent>>: std::true_type {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_transparent_v = is_transparent<Type>::value;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename = void>
struct has_tuple_size_value: std::false_type {};

template<typename Type>
struct has_tuple_size_value<Type, std::void_t<decltype(std::tuple_size<const Type>::value)>>: std::true_type {};

template<typename, typename = void>
struct has_value_type: std::false_type {};

template<typename Type>
struct has_value_type<Type, std::void_t<typename Type::value_type>>: std::true_type {};

template<typename>
[[nodiscard]] constexpr bool dispatch_is_equality_comparable();

template<typename Type, std::size_t... Index>
[[nodiscard]] constexpr bool unpack_maybe_equality_comparable(std::index_sequence<Index...>) {
    return (dispatch_is_equality_comparable<std::tuple_element_t<Index, Type>>() && ...);
}

template<typename>
[[nodiscard]] constexpr bool maybe_equality_comparable(char) {
    return false;
}

template<typename Type>
[[nodiscard]] constexpr auto maybe_equality_comparable(int) -> decltype(std::declval<Type>() == std::declval<Type>()) {
    return true;
}

template<typename Type>
[[nodiscard]] constexpr bool dispatch_is_equality_comparable() {
    // NOLINTBEGIN(modernize-use-transparent-functors)
    if constexpr(std::is_array_v<Type>) {
        return false;
    } else if constexpr(is_complete_v<std::tuple_size<std::remove_cv_t<Type>>>) {
        if constexpr(has_tuple_size_value<Type>::value) {
            return maybe_equality_comparable<Type>(0) && unpack_maybe_equality_comparable<Type>(std::make_index_sequence<std::tuple_size<Type>::value>{});
        } else {
            return maybe_equality_comparable<Type>(0);
        }
    } else if constexpr(has_value_type<Type>::value) {
        if constexpr(is_iterator_v<Type> || std::is_same_v<typename Type::value_type, Type> || dispatch_is_equality_comparable<typename Type::value_type>()) {
            return maybe_equality_comparable<Type>(0);
        } else {
            return false;
        }
    } else {
        return maybe_equality_comparable<Type>(0);
    }
    // NOLINTEND(modernize-use-transparent-functors)
}

} // namespace internal
/*! @endcond */

/**
 * @brief Provides the member constant `value` to true if a given type is
 * equality comparable, false otherwise.
 * @tparam Type The type to test.
 */
template<typename Type>
struct is_equality_comparable: std::bool_constant<internal::dispatch_is_equality_comparable<Type>()> {};

/*! @copydoc is_equality_comparable */
template<typename Type>
struct is_equality_comparable<const Type>: is_equality_comparable<Type> {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<Type>::value;

/**
 * @brief Transcribes the constness of a type to another type.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
struct constness_as {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = std::remove_const_t<To>;
};

/*! @copydoc constness_as */
template<typename To, typename From>
struct constness_as<To, const From> {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = const To;
};

/**
 * @brief Alias template to facilitate the transcription of the constness.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
using constness_as_t = typename constness_as<To, From>::type;

/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>, "Invalid pointer type to non-static member object or function");

    template<typename Class, typename Ret, typename... Args>
    static Class *clazz(Ret (Class::*)(Args...));

    template<typename Class, typename Ret, typename... Args>
    static Class *clazz(Ret (Class::*)(Args...) const);

    template<typename Class, typename Type>
    static Class *clazz(Type Class::*);

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

/**
 * @brief Extracts the n-th argument of a _callable_ type.
 * @tparam Index The index of the argument to extract.
 * @tparam Candidate A valid _callable_ type.
 */
template<std::size_t Index, typename Candidate>
class nth_argument {
    template<typename Ret, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (*)(Args...));

    template<typename Ret, typename Class, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (Class ::*)(Args...));

    template<typename Ret, typename Class, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (Class ::*)(Args...) const);

    template<typename Type, typename Class>
    static constexpr type_list<Type> pick_up(Type Class ::*);

    template<typename Type>
    static constexpr decltype(pick_up(&Type::operator())) pick_up(Type &&);

public:
    /*! @brief N-th argument of the _callable_ type. */
    using type = type_list_element_t<Index, decltype(pick_up(std::declval<Candidate>()))>;
};

/**
 * @brief Helper type.
 * @tparam Index The index of the argument to extract.
 * @tparam Candidate A valid function, member function or data member type.
 */
template<std::size_t Index, typename Candidate>
using nth_argument_t = typename nth_argument<Index, Candidate>::type;

} // namespace entt

template<typename... Type>
struct std::tuple_size<entt::type_list<Type...>>: std::integral_constant<std::size_t, entt::type_list<Type...>::size> {};

template<std::size_t Index, typename... Type>
struct std::tuple_element<Index, entt::type_list<Type...>>: entt::type_list_element<Index, entt::type_list<Type...>> {};

template<auto... Value>
struct std::tuple_size<entt::value_list<Value...>>: std::integral_constant<std::size_t, entt::value_list<Value...>::size> {};

template<std::size_t Index, auto... Value>
struct std::tuple_element<Index, entt::value_list<Value...>>: entt::value_list_element<Index, entt::value_list<Value...>> {};

#endif
