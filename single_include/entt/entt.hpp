// #include "core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <functional>
#include <algorithm>
#include <utility>


namespace entt {


/**
 * @brief Function object to wrap `std::sort` in a class type.
 *
 * Unfortunately, `std::sort` cannot be passed as template argument to a class
 * template or a function template.<br/>
 * This class fills the gap by wrapping some flavors of `std::sort` in a
 * function object.
 */
struct std_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @tparam Args Types of arguments to forward to the sort function.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param args Arguments to forward to the sort function, if any.
     */
    template<typename It, typename Compare = std::less<>, typename... Args>
    void operator()(It first, It last, Compare compare = Compare{}, Args &&... args) const {
        std::sort(std::forward<Args>(args)..., std::move(first), std::move(last), std::move(compare));
    }
};


/*! @brief Function object for performing insertion sort. */
struct insertion_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     */
    template<typename It, typename Compare = std::less<>>
    void operator()(It first, It last, Compare compare = Compare{}) const {
        if(first != last) {
            auto it = first + 1;

            while(it != last) {
                auto pre = it++;
                auto value = *pre;

                while(pre-- != first && compare(value, *pre)) {
                    *(pre+1) = *pre;
                }

                *(pre+1) = value;
            }
        }
    }
};


}


#endif // ENTT_CORE_ALGORITHM_HPP

// #include "core/family.hpp"
#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @brief Dynamic identifier generator.
 *
 * Utility class template that can be used to assign unique identifiers to types
 * at runtime. Use different specializations to create separate sets of
 * identifiers.
 */
template<typename...>
class family {
    inline static maybe_atomic_t<ENTT_ID_TYPE> identifier;

    template<typename...>
    inline static const auto inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used in place of family_type here
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

// #include "core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP

// #include "core/ident.hpp"
#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP


#include <tuple>
#include <utility>
#include <type_traits>
// #include "../config/config.h"



namespace entt {


/**
 * @brief Types identifiers.
 *
 * Variable template used to generate identifiers at compile-time for the given
 * types. Use the `get` member function to know what's the identifier associated
 * to the specific type.
 *
 * @note
 * Identifiers are constant expression and can be used in any context where such
 * an expression is required. As an example:
 * @code{.cpp}
 * using id = entt::identifier<a_type, another_type>;
 *
 * switch(a_type_identifier) {
 * case id::type<a_type>:
 *     // ...
 *     break;
 * case id::type<another_type>:
 *     // ...
 *     break;
 * default:
 *     // ...
 * }
 * @endcode
 *
 * @tparam Types List of types for which to generate identifiers.
 */
template<typename... Types>
class identifier {
    using tuple_type = std::tuple<std::decay_t<Types>...>;

    template<typename Type, std::size_t... Indexes>
    static constexpr ENTT_ID_TYPE get(std::index_sequence<Indexes...>) ENTT_NOEXCEPT {
        static_assert(std::disjunction_v<std::is_same<Type, Types>...>);
        return (0 + ... + (std::is_same_v<Type, std::tuple_element_t<Indexes, tuple_type>> ? ENTT_ID_TYPE(Indexes) : ENTT_ID_TYPE{}));
    }

public:
    /*! @brief Unsigned integer type. */
    using identifier_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename Type>
    static constexpr identifier_type type = get<std::decay_t<Type>>(std::make_index_sequence<sizeof...(Types)>{});
};


}


#endif // ENTT_CORE_IDENT_HPP

// #include "core/monostate.hpp"
#ifndef ENTT_CORE_MONOSTATE_HPP
#define ENTT_CORE_MONOSTATE_HPP


#include <cassert>
// #include "../config/config.h"

// #include "hashed_string.hpp"



namespace entt {


/**
 * @brief Minimal implementation of the monostate pattern.
 *
 * A minimal, yet complete configuration system built on top of the monostate
 * pattern. Thread safe by design, it works only with basic types like `int`s or
 * `bool`s.<br/>
 * Multiple types and therefore more than one value can be associated with a
 * single key. Because of this, users must pay attention to use the same type
 * both during an assignment and when they try to read back their data.
 * Otherwise, they can incur in unexpected results.
 */
template<hashed_string::hash_type>
struct monostate {
    /**
     * @brief Assigns a value of a specific type to a given key.
     * @tparam Type Type of the value to assign.
     * @param val User data to assign to the given key.
     */
    template<typename Type>
    void operator=(Type val) const ENTT_NOEXCEPT {
        value<Type> = val;
    }

    /**
     * @brief Gets a value of a specific type for a given key.
     * @tparam Type Type of the value to get.
     * @return Stored value, if any.
     */
    template<typename Type>
    operator Type() const ENTT_NOEXCEPT {
        return value<Type>;
    }

private:
    template<typename Type>
    inline static maybe_atomic_t<Type> value{};
};


/**
 * @brief Helper variable template.
 * @tparam Value Value used to differentiate between different variables.
 */
template<hashed_string::hash_type Value>
inline monostate<Value> monostate_v = {};


}


#endif // ENTT_CORE_MONOSTATE_HPP

// #include "core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the given type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Unsigned integer type. */
    static constexpr auto size = sizeof...(Type);
};


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

// #include "core/utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


namespace entt {


/**
 * @brief Constant utility to disambiguate overloaded member functions.
 * @tparam Type Function type of the desired overload.
 * @tparam Class Type of class to which the member functions belong.
 * @param member A valid pointer to a member function.
 * @return Pointer to the member function.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Type Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Type>
constexpr auto overload(Type *func) { return func; }


}


#endif // ENTT_CORE_UTILITY_HPP

// #include "entity/actor.hpp"
#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <cassert>
#include <utility>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H

// #include "registry.hpp"
#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/family.hpp"
#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @brief Dynamic identifier generator.
 *
 * Utility class template that can be used to assign unique identifiers to types
 * at runtime. Use different specializations to create separate sets of
 * identifiers.
 */
template<typename...>
class family {
    inline static maybe_atomic_t<ENTT_ID_TYPE> identifier;

    template<typename...>
    inline static const auto inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used in place of family_type here
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

// #include "../core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <functional>
#include <algorithm>
#include <utility>


namespace entt {


/**
 * @brief Function object to wrap `std::sort` in a class type.
 *
 * Unfortunately, `std::sort` cannot be passed as template argument to a class
 * template or a function template.<br/>
 * This class fills the gap by wrapping some flavors of `std::sort` in a
 * function object.
 */
struct std_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @tparam Args Types of arguments to forward to the sort function.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param args Arguments to forward to the sort function, if any.
     */
    template<typename It, typename Compare = std::less<>, typename... Args>
    void operator()(It first, It last, Compare compare = Compare{}, Args &&... args) const {
        std::sort(std::forward<Args>(args)..., std::move(first), std::move(last), std::move(compare));
    }
};


/*! @brief Function object for performing insertion sort. */
struct insertion_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     */
    template<typename It, typename Compare = std::less<>>
    void operator()(It first, It last, Compare compare = Compare{}) const {
        if(first != last) {
            auto it = first + 1;

            while(it != last) {
                auto pre = it++;
                auto value = *pre;

                while(pre-- != first && compare(value, *pre)) {
                    *(pre+1) = *pre;
                }

                *(pre+1) = value;
            }
        }
    }
};


}


#endif // ENTT_CORE_ALGORITHM_HPP

// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the given type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Unsigned integer type. */
    static constexpr auto size = sizeof...(Type);
};


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

// #include "../signal/sigh.hpp"
#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>
#include <functional>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H

// #include "delegate.hpp"
#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <cstring>
#include <algorithm>
#include <functional>
#include <type_traits>
// #include "../config/config.h"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Ret, typename... Args>
auto to_function_pointer(Ret(*)(Args...)) -> Ret(*)(Args...);


template<typename Ret, typename... Args, typename Type>
auto to_function_pointer(Ret(*)(Type *, Args...), Type *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), Class *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, Class *) -> Ret(*)(Args...);


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Used to wrap a function or a member of a specified type. */
template<auto>
struct connect_arg_t {};


/*! @brief Constant of type connect_arg_t used to disambiguate calls. */
template<auto Func>
constexpr connect_arg_t<Func> connect_arg{};


/**
 * @brief Basic delegate implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class delegate;


/**
 * @brief Utility class to use to send around functions and members.
 *
 * Unmanaged delegate for function pointers and members. Users of this class are
 * in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions (with or without payload) and members provided along with an
 * instance on which to invoke them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using proto_fn_type = Ret(const void *, Args...);

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : fn{nullptr}, data{nullptr}
    {}

    /**
     * @brief Constructs a delegate and connects a free function to it.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Function>();
    }

    /**
     * @brief Constructs a delegate and connects a member for a given instance
     * or a free function with payload.
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type *value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(value_or_instance);
    }

    /**
     * @brief Connects a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), Args...>);
        data = nullptr;

        fn = [](const void *, Args... args) -> Ret {
            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Function, args...));
        };
    }

    /**
     * @brief Connects a member function for a given instance or a free function
     * with payload to a delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>);
        data = value_or_instance;

        fn = [](const void *payload, Args... args) -> Ret {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(payload);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(payload));
            }

            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Candidate, curr, args...));
        };
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate cannot be invoked anymore.
     */
    void reset() ENTT_NOEXCEPT {
        fn = nullptr;
        data = nullptr;
    }

    /**
     * @brief Returns the instance linked to a delegate, if any.
     * @return An opaque pointer to the instance linked to the delegate, if any.
     */
    const void * instance() const ENTT_NOEXCEPT {
        return data;
    }

    /**
     * @brief Triggers a delegate.
     *
     * The delegate invokes the underlying function and returns the result.
     *
     * @warning
     * Attempting to trigger an invalid delegate results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * delegate has not yet been set.
     *
     * @param args Arguments to use to invoke the underlying function.
     * @return The value returned by the underlying function.
     */
    Ret operator()(Args... args) const {
        ENTT_ASSERT(fn);
        return fn(data, args...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also data
        return fn;
    }

    /**
     * @brief Checks if the connected functions differ.
     *
     * Instances connected to delegates are ignored by this operator. Use the
     * `instance` member function instead.
     *
     * @param other Delegate with which to compare.
     * @return False if the connected functions differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Checks if the connected functions differ.
 *
 * Instances connected to delegates are ignored by this operator. Use the
 * `instance` member function instead.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the connected functions differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a
 * function provided to the constructor.
 *
 * @tparam Function A valid free function pointer.
 */
template<auto Function>
delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Function))>>;


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a member
 * or a free function with payload provided to the constructor.
 *
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type *) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Candidate, std::declval<Type *>()))>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP

// #include "fwd.hpp"
#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @class delegate */
template<typename>
class delegate;

/*! @class sink */
template<typename>
class sink;

/*! @class sigh */
template<typename, typename>
struct sigh;


}


#endif // ENTT_SIGNAL_FWD_HPP



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename, typename>
struct invoker;


template<typename Ret, typename... Args, typename Collector>
struct invoker<Ret(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &collector, const delegate<Ret(Args...)> &delegate, Args... args) const {
        return collector(delegate(args...));
    }
};


template<typename... Args, typename Collector>
struct invoker<void(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &, const delegate<void(Args...)> &delegate, Args... args) const {
        return (delegate(args...), true);
    }
};


template<typename Ret>
struct null_collector {
    using result_type = Ret;
    bool operator()(result_type) const ENTT_NOEXCEPT { return true; }
};


template<>
struct null_collector<void> {
    using result_type = void;
    bool operator()() const ENTT_NOEXCEPT { return true; }
};


template<typename>
struct default_collector;


template<typename Ret, typename... Args>
struct default_collector<Ret(Args...)> {
    using collector_type = null_collector<Ret>;
};


template<typename Function>
using default_collector_type = typename default_collector<Function>::collector_type;


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Sink implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler declaration.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Function, typename Collector = internal::default_collector_type<Function>>
struct sigh;


/**
 * @brief Sink implementation.
 *
 * A sink is an opaque object used to connect listeners to signals.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of a class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    /*! @brief A signal is allowed to create sinks. */
    template<typename, typename>
    friend struct sigh;

    template<typename Type>
    Type * payload_type(Ret(*)(Type *, Args...));

    sink(std::vector<delegate<Ret(Args...)>> *ref) ENTT_NOEXCEPT
        : calls{ref}
    {}

public:
    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return calls->empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() {
        disconnect<Function>();
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls->emplace_back(std::move(delegate));
    }

    /**
     * @brief Connects a member function or a free function with payload to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) {
        if constexpr(std::is_member_function_pointer_v<decltype(Candidate)>) {
            disconnect<Candidate>(value_or_instance);
        } else {
            disconnect<Candidate>();
        }

        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls->emplace_back(std::move(delegate));
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        delegate<Ret(Args...)> delegate{};

        if constexpr(std::is_invocable_r_v<Ret, decltype(Function), Args...>) {
            delegate.template connect<Function>();
        } else {
            decltype(payload_type(Function)) payload = nullptr;
            delegate.template connect<Function>(payload);
        }

        calls->erase(std::remove(calls->begin(), calls->end(), std::move(delegate)), calls->end());
    }

    /**
     * @brief Disconnects a given member function from a signal.
     * @tparam Member Member function to disconnect from the signal.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class>
    void disconnect(Class *instance) {
        static_assert(std::is_member_function_pointer_v<decltype(Member)>);
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Member>(instance);
        calls->erase(std::remove_if(calls->begin(), calls->end(), [&delegate](const auto &other) {
            return other == delegate && other.instance() == delegate.instance();
        }), calls->end());
    }

    /**
     * @brief Disconnects all the listeners from a signal.
     */
    void disconnect() {
        calls->clear();
    }

private:
    std::vector<delegate<Ret(Args...)>> *calls;
};


/**
 * @brief Unmanaged signal handler definition.
 *
 * Unmanaged signal handler. It works directly with naked pointers to classes
 * and pointers to member functions as well as pointers to free functions. Users
 * of this class are in charge of disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals used later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * The default collector does nothing. To properly collect data, define and use
 * a class that has a call operator the signature of which is `bool(Param)` and:
 *
 * * `Param` is a type to which `Ret` can be converted.
 * * The return type is true if the handler must stop collecting data, false
 * otherwise.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Ret, typename... Args, typename Collector>
struct sigh<Ret(Args...), Collector>: private internal::invoker<Ret(Args...), Collector> {
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
    /*! @brief Collector type. */
    using collector_type = Collector;
    /*! @brief Sink type. */
    using sink_type = entt::sink<Ret(Args...)>;

    /**
     * @brief Instance type when it comes to connecting member functions.
     * @tparam Class Type of class to which the member function belongs.
     */
    template<typename Class>
    using instance_type = Class *;

    /**
     * @brief Number of listeners connected to the signal.
     * @return Number of listeners currently connected.
     */
    size_type size() const ENTT_NOEXCEPT {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return calls.empty();
    }

    /**
     * @brief Returns a sink object for the given signal.
     *
     * A sink is an opaque object used to connect listeners to signals.<br/>
     * The function type for a listener is the one of the signal to which it
     * belongs. The order of invocation of the listeners isn't guaranteed.
     *
     * @return A temporary sink object.
     */
    sink_type sink() ENTT_NOEXCEPT {
        return { &calls };
    }

    /**
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            auto &call = calls[pos-1];
            call(args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     * @param args Arguments to use to invoke listeners.
     * @return An instance of the collector filled with collected data.
     */
    collector_type collect(Args... args) const {
        collector_type collector;

        for(auto &&call: calls) {
            if(!this->invoke(collector, call, args...)) {
                break;
            }
        }

        return collector;
    }

    /**
     * @brief Swaps listeners between the two signals.
     * @param lhs A valid signal object.
     * @param rhs A valid signal object.
     */
    friend void swap(sigh &lhs, sigh &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


}


#endif // ENTT_SIGNAL_SIGH_HPP

// #include "runtime_view.hpp"
#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP


#include <iterator>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
// #include "../config/config.h"

// #include "sparse_set.hpp"
#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>
// #include "../config/config.h"

// #include "entity.hpp"
#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


// #include "../config/config.h"



namespace entt {


/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
template<typename>
struct entt_traits;


/**
 * @brief Entity traits for a 16 bits entity identifier.
 *
 * A 16 bits entity identifier guarantees:
 *
 * * 12 bits for the entity number (up to 4k entities).
 * * 4 bit for the version (resets in [0-15]).
 */
template<>
struct entt_traits<std::uint16_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint16_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint8_t;
    /*! @brief Difference type. */
    using difference_type = std::int32_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr std::uint16_t entity_mask = 0xFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr std::uint16_t version_mask = 0xF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto entity_shift = 12;
};


/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier guarantees:
 *
 * * 20 bits for the entity number (suitable for almost all the games).
 * * 12 bit for the version (resets in [0-4095]).
 */
template<>
struct entt_traits<std::uint32_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint32_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint16_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr std::uint32_t entity_mask = 0xFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr std::uint32_t version_mask = 0xFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto entity_shift = 20;
};


/**
 * @brief Entity traits for a 64 bits entity identifier.
 *
 * A 64 bits entity identifier guarantees:
 *
 * * 32 bits for the entity number (an indecently large number).
 * * 32 bit for the version (an indecently large number).
 */
template<>
struct entt_traits<std::uint64_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint64_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint32_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr std::uint64_t entity_mask = 0xFFFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr std::uint64_t version_mask = 0xFFFFFFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto entity_shift = 32;
};


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct null {
    template<typename Entity>
    constexpr operator Entity() const ENTT_NOEXCEPT {
        using traits_type = entt_traits<Entity>;
        return traits_type::entity_mask | (traits_type::version_mask << traits_type::entity_shift);
    }

    constexpr bool operator==(null) const ENTT_NOEXCEPT {
        return true;
    }

    constexpr bool operator!=(null) const ENTT_NOEXCEPT {
        return false;
    }

    template<typename Entity>
    constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return entity == static_cast<Entity>(*this);
    }

    template<typename Entity>
    constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return entity != static_cast<Entity>(*this);
    }
};


template<typename Entity>
constexpr bool operator==(const Entity entity, null other) ENTT_NOEXCEPT {
    return other == entity;
}


template<typename Entity>
constexpr bool operator!=(const Entity entity, null other) ENTT_NOEXCEPT {
    return other != entity;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Null entity.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
constexpr auto null = internal::null{};


}


#endif // ENTT_ENTITY_ENTITY_HPP



namespace entt {


/**
 * @brief Basic sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This is largely used by the registry to offer users the fastest access ever
 * to the components. Views and groups in general are almost entirely designed
 * around sparse sets.
 *
 * This type of data structure is widely documented in the literature and on the
 * web. This is nothing more than a customized implementation suitable for the
 * purpose of the framework.
 *
 * @note
 * There are no guarantees that entities are returned in the insertion order
 * when iterate a sparse set. Do not make assumption on the order in any case.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `data` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class sparse_set {
    using traits_type = entt_traits<Entity>;

    static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0));
    static constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(typename entt_traits<Entity>::entity_type);

    class iterator {
        friend class sparse_set<Entity>;

        using direct_type = const std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        iterator(direct_type *ref, const index_type idx) ENTT_NOEXCEPT
            : direct{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return --(*this), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{direct, index-value};
        }

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*direct)[pos];
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*direct)[pos];
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        direct_type *direct;
        index_type index;
    };

    void assure(const std::size_t page) {
        if(!(page < reverse.size())) {
            reverse.resize(page+1);
        }

        if(!reverse[page].first) {
            reverse[page].first = std::make_unique<entity_type[]>(entt_per_page);
            // null is safe in all cases for our purposes
            std::fill_n(reverse[page].first.get(), entt_per_page, null);
        }
    }

    auto index(const Entity entt) const ENTT_NOEXCEPT {
        const auto identifier = entt & traits_type::entity_mask;
        const auto page = size_type(identifier / entt_per_page);
        const auto offset = size_type(identifier & (entt_per_page - 1));
        return std::make_pair(page, offset);
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator;

    /*! @brief Default constructor. */
    sparse_set() = default;

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    sparse_set(const sparse_set &other)
        : reverse{},
          direct{other.direct}
    {
        for(size_type pos{}, last = other.reverse.size(); pos < last; ++pos) {
            if(other.reverse[pos].first) {
                assure(pos);
                std::copy_n(other.reverse[pos].first.get(), entt_per_page, reverse[pos].first.get());
                reverse[pos].second = other.reverse[pos].second;
            }
        }
    }

    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() ENTT_NOEXCEPT = default;

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This sparse set.
     */
    sparse_set & operator=(const sparse_set &other) {
        if(&other != this) {
            auto tmp{other};
            *this = std::move(tmp);
        }

        return *this;
    }

    /*! @brief Default move assignment operator. @return This sparse set. */
    sparse_set & operator=(sparse_set &&) = default;

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    virtual void reserve(const size_type cap) {
        direct.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return direct.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    virtual void shrink_to_fit() {
        while(!reverse.empty() && !reverse.back().second) {
            reverse.pop_back();
        }

        for(auto &&data: reverse) {
            if(!data.second) {
                data.first.reset();
            }
        }

        reverse.shrink_to_fit();
        direct.shrink_to_fit();
    }

    /**
     * @brief Returns the extent of a sparse set.
     *
     * The extent of a sparse set is also the size of the internal sparse array.
     * There is no guarantee that the internal packed array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Extent of the sparse set.
     */
    size_type extent() const ENTT_NOEXCEPT {
        return reverse.size() * entt_per_page;
    }

    /**
     * @brief Returns the number of elements in a sparse set.
     *
     * The number of elements is also the size of the internal packed array.
     * There is no guarantee that the internal sparse array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Number of elements.
     */
    size_type size() const ENTT_NOEXCEPT {
        return direct.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return direct.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though `respect` has been
     * previously invoked. Internal data structures arrange elements to maximize
     * performance. Accessing them directly gives a performance boost but less
     * guarantees. Use `begin` and `end` if you want to iterate the sparse set
     * in the expected order.
     *
     * @return A pointer to the internal packed array.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return direct.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first entity of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to `respect`.
     *
     * @return An iterator to the first entity of the internal packed array.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = direct.size();
        return iterator_type{&direct, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to `respect`.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return iterator_type{&direct, {}};
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        return has(entt) ? --(end() - get(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entt) const ENTT_NOEXCEPT {
        auto [page, offset] = index(entt);
        // testing against null permits to avoid accessing the direct vector
        return (page < reverse.size() && reverse[page].second && reverse[page].first[offset] != null);
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    size_type get(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(has(entt));
        auto [page, offset] = index(entt);
        return size_type(reverse[page].first[offset]);
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void construct(const entity_type entt) {
        ENTT_ASSERT(!has(entt));
        auto [page, offset] = index(entt);
        assure(page);
        reverse[page].first[offset] = entity_type(direct.size());
        reverse[page].second++;
        direct.push_back(entt);
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void batch(It first, It last) {
        std::for_each(first, last, [next = entity_type(direct.size()), this](const auto entt) mutable {
            ENTT_ASSERT(!has(entt));
            auto [page, offset] = index(entt);
            assure(page);
            reverse[page].first[offset] = next++;
            reverse[page].second++;
        });

        direct.insert(direct.end(), first, last);
    }

    /**
     * @brief Removes an entity from a sparse set.
     *
     * @warning
     * Attempting to remove an entity that doesn't belong to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     */
    virtual void destroy(const entity_type entt) {
        ENTT_ASSERT(has(entt));
        auto [from_page, from_offset] = index(entt);
        auto [to_page, to_offset] = index(direct.back());
        direct[size_type(reverse[from_page].first[from_offset])] = direct.back();
        reverse[to_page].first[to_offset] = reverse[from_page].first[from_offset];
        reverse[from_page].first[from_offset] = null;
        reverse[from_page].second--;
        direct.pop_back();
    }

    /**
     * @brief Swaps the position of two entities in the internal packed array.
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entities.
     *
     * @param lhs A valid position within the sparse set.
     * @param rhs A valid position within the sparse set.
     */
    void swap(const size_type lhs, const size_type rhs) ENTT_NOEXCEPT {
        ENTT_ASSERT(lhs < direct.size());
        ENTT_ASSERT(rhs < direct.size());
        auto [src_page, src_offset] = index(direct[lhs]);
        auto [dst_page, dst_offset] = index(direct[rhs]);
        std::swap(reverse[src_page].first[src_offset], reverse[dst_page].first[dst_offset]);
        std::swap(direct[lhs], direct[rhs]);
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @note
     * Attempting to iterate elements using the raw pointer returned by `data`
     * gives no guarantees on the order, even though `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    virtual void respect(const sparse_set &other) ENTT_NOEXCEPT {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = direct.size() - 1;

        while(pos && from != to) {
            if(has(*from)) {
                if(*from != direct[pos]) {
                    swap(pos, get(*from));
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Resets a sparse set.
     */
    virtual void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<std::pair<std::unique_ptr<entity_type[]>, size_type>> reverse;
    std::vector<entity_type> direct;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP

// #include "entity.hpp"

// #include "fwd.hpp"
#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


#include <cstdint>
// #include "../config/config.h"



namespace entt {

/*! @class basic_registry */
template <typename>
class basic_registry;

/*! @class basic_view */
template<typename, typename...>
class basic_view;

/*! @class basic_runtime_view */
template<typename>
class basic_runtime_view;

/*! @class basic_group */
template<typename...>
class basic_group;

/*! @class basic_actor */
template <typename>
struct basic_actor;

/*! @class basic_prototype */
template<typename>
class basic_prototype;

/*! @class basic_snapshot */
template<typename>
class basic_snapshot;

/*! @class basic_snapshot_loader */
template<typename>
class basic_snapshot_loader;

/*! @class basic_continuous_loader */
template<typename>
class basic_continuous_loader;

/*! @brief Alias declaration for the most common use case. */
using entity = std::uint32_t;

/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;

/*! @brief Alias declaration for the most common use case. */
using actor = basic_actor<entity>;

/*! @brief Alias declaration for the most common use case. */
using prototype = basic_prototype<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot = basic_snapshot<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot_loader = basic_snapshot_loader<entity>;

/*! @brief Alias declaration for the most common use case. */
using continuous_loader = basic_continuous_loader<entity>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Component Types of components iterated by the view.
 */
template<typename... Types>
using view = basic_view<entity, Types...>;

/*! @brief Alias declaration for the most common use case. */
using runtime_view = basic_runtime_view<entity>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Types Types of components iterated by the group.
 */
template<typename... Types>
using group = basic_group<entity, Types...>;


}


#endif // ENTT_ENTITY_FWD_HPP



namespace entt {


/**
 * @brief Runtime view.
 *
 * Runtime views iterate over those entities that have at least all the given
 * components in their bags. During initialization, a runtime view looks at the
 * number of entities available for each component and picks up a reference to
 * the smallest set of candidate entities in order to get a performance boost
 * when iterate.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structures. See sparse_set and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by the views, unless
 * a pool was missing when the view was built (in this case, the view won't
 * have a valid reference and won't be updated accordingly).
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_runtime_view {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using underlying_iterator_type = typename sparse_set<Entity>::iterator_type;
    using extent_type = typename sparse_set<Entity>::size_type;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class basic_runtime_view<Entity>;

        iterator(underlying_iterator_type first, underlying_iterator_type last, const sparse_set<Entity> * const *others, const sparse_set<Entity> * const *length, extent_type ext) ENTT_NOEXCEPT
            : begin{first},
              end{last},
              from{others},
              to{length},
              extent{ext}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        bool valid() const ENTT_NOEXCEPT {
            const auto entt = *begin;
            const auto sz = size_type(entt & traits_type::entity_mask);

            return sz < extent && std::all_of(from, to, [entt](const auto *view) {
                return view->has(entt);
            });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::forward_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        underlying_iterator_type begin;
        underlying_iterator_type end;
        const sparse_set<Entity> * const *from;
        const sparse_set<Entity> * const *to;
        extent_type extent;
    };

    basic_runtime_view(std::vector<const sparse_set<Entity> *> others) ENTT_NOEXCEPT
        : pools{std::move(others)}
    {
        const auto it = std::min_element(pools.begin(), pools.end(), [](const auto *lhs, const auto *rhs) {
            return (!lhs && rhs) || (lhs && rhs && lhs->size() < rhs->size());
        });

        // brings the best candidate (if any) on front of the vector
        std::rotate(pools.begin(), it, pools.end());
    }

    extent_type min() const ENTT_NOEXCEPT {
        extent_type extent{};

        if(valid()) {
            const auto it = std::min_element(pools.cbegin(), pools.cend(), [](const auto *lhs, const auto *rhs) {
                return lhs->extent() < rhs->extent();
            });

            extent = (*it)->extent();
        }

        return extent;
    }

    inline bool valid() const ENTT_NOEXCEPT {
        return !pools.empty() && pools.front();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return valid() ? pools.front()->size() : size_type{};
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return !valid() || pools.front()->empty();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            const auto * const *data = pools.data();
            it = { pool.begin(), pool.end(), data + 1, data + pools.size(), min() };
        }

        return it;
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            it = { pool.end(), pool.end(), nullptr, nullptr, min() };
        }

        return it;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return valid() && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *view) {
            return view->has(entt) && view->data()[view->get(entt)] == entt;
        });
    }

    /**
     * @brief Iterates entities and applies the given function object to them.
     *
     * The function object is invoked for each entity. It is provided only with
     * the entity itself. To get the components, users can use the registry with
     * which the view was built.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const entity_type);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        std::for_each(begin(), end(), func);
    }

private:
    std::vector<const sparse_set<Entity> *> pools;
};


}


#endif // ENTT_ENTITY_RUNTIME_VIEW_HPP

// #include "sparse_set.hpp"

// #include "snapshot.hpp"
#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <array>
#include <cstddef>
#include <utility>
#include <iterator>
#include <type_traits>
#include <unordered_map>
// #include "../config/config.h"

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Utility class to create snapshots from a registry.
 *
 * A _snapshot_ can be either a dump of the entire registry or a narrower
 * selection of components of interest.<br/>
 * This type can be used in both cases if provided with a correctly configured
 * output archive.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_snapshot {
    /*! @brief A registry is allowed to create snapshots. */
    friend class basic_registry<Entity>;

    using follow_fn_type = Entity(const basic_registry<Entity> &, const Entity);

    basic_snapshot(const basic_registry<Entity> *source, Entity init, follow_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          seed{init},
          follow{fn}
    {}

    template<typename Component, typename Archive, typename It>
    void get(Archive &archive, std::size_t sz, It first, It last) const {
        archive(static_cast<Entity>(sz));

        while(first != last) {
            const auto entt = *(first++);

            if(reg->template has<Component>(entt)) {
                if constexpr(std::is_empty_v<Component>) {
                    archive(entt);
                } else {
                    archive(entt, reg->template get<Component>(entt));
                }
            }
        }
    }

    template<typename... Component, typename Archive, typename It, std::size_t... Indexes>
    void component(Archive &archive, It first, It last, std::index_sequence<Indexes...>) const {
        std::array<std::size_t, sizeof...(Indexes)> size{};
        auto begin = first;

        while(begin != last) {
            const auto entt = *(begin++);
            ((reg->template has<Component>(entt) ? ++size[Indexes] : size[Indexes]), ...);
        }

        (get<Component>(archive, size[Indexes], first, last), ...);
    }

public:
    /*! @brief Default move constructor. */
    basic_snapshot(basic_snapshot &&) = default;

    /*! @brief Default move assignment operator. @return This snapshot. */
    basic_snapshot & operator=(basic_snapshot &&) = default;

    /**
     * @brief Puts aside all the entities that are still in use.
     *
     * Entities are serialized along with their versions. Destroyed entities are
     * not taken in consideration by this function.
     *
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Archive>
    const basic_snapshot & entities(Archive &archive) const {
        archive(static_cast<Entity>(reg->alive()));
        reg->each([&archive](const auto entt) { archive(entt); });
        return *this;
    }

    /**
     * @brief Puts aside destroyed entities.
     *
     * Entities are serialized along with their versions. Entities that are
     * still in use are not taken in consideration by this function.
     *
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Archive>
    const basic_snapshot & destroyed(Archive &archive) const {
        auto size = reg->size() - reg->alive();
        archive(static_cast<Entity>(size));

        if(size) {
            auto curr = seed;
            archive(curr);

            for(--size; size; --size) {
                curr = follow(*reg, curr);
                archive(curr);
            }
        }

        return *this;
    }

    /**
     * @brief Puts aside the given components.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Component Types of components to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename... Component, typename Archive>
    const basic_snapshot & component(Archive &archive) const {
        if constexpr(sizeof...(Component) == 1) {
            const auto sz = reg->template size<Component...>();
            const auto *entities = reg->template data<Component...>();

            archive(static_cast<Entity>(sz));

            for(std::remove_const_t<decltype(sz)> pos{}; pos < sz; ++pos) {
                const auto entt = entities[pos];

                if constexpr(std::is_empty_v<Component...>) {
                    archive(entt);
                } else {
                    archive(entt, reg->template get<Component...>(entt));
                }
            };
        } else {
            (component<Component>(archive), ...);
        }

        return *this;
    }

    /**
     * @brief Puts aside the given components for the entities in a range.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Component Types of components to serialize.
     * @tparam Archive Type of output archive.
     * @tparam It Type of input iterator.
     * @param archive A valid reference to an output archive.
     * @param first An iterator to the first element of the range to serialize.
     * @param last An iterator past the last element of the range to serialize.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename... Component, typename Archive, typename It>
    const basic_snapshot & component(Archive &archive, It first, It last) const {
        component<Component...>(archive, first, last, std::make_index_sequence<sizeof...(Component)>{});
        return *this;
    }

private:
    const basic_registry<Entity> *reg;
    const Entity seed;
    follow_fn_type *follow;
};


/**
 * @brief Utility class to restore a snapshot as a whole.
 *
 * A snapshot loader requires that the destination registry be empty and loads
 * all the data at once while keeping intact the identifiers that the entities
 * originally had.<br/>
 * An example of use is the implementation of a save/restore utility.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_snapshot_loader {
    /*! @brief A registry is allowed to create snapshot loaders. */
    friend class basic_registry<Entity>;

    using force_fn_type = void(basic_registry<Entity> &, const Entity, const bool);

    basic_snapshot_loader(basic_registry<Entity> *source, force_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          force{fn}
    {
        // to restore a snapshot as a whole requires a clean registry
        ENTT_ASSERT(reg->empty());
    }

    template<typename Archive>
    void assure(Archive &archive, bool destroyed) const {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entt{};
            archive(entt);
            force(*reg, entt, destroyed);
        }
    }

    template<typename Type, typename Archive, typename... Args>
    void assign(Archive &archive, Args... args) const {
        Entity length{};
        archive(length);

        while(length--) {
            static constexpr auto destroyed = false;
            Entity entt{};

            if constexpr(std::is_empty_v<Type>) {
                archive(entt);
                force(*reg, entt, destroyed);
                reg->template assign<Type>(args..., entt);
            } else {
                Type instance{};
                archive(entt, instance);
                force(*reg, entt, destroyed);
                reg->template assign<Type>(args..., entt, std::as_const(instance));
            }
        }
    }

public:
    /*! @brief Default move constructor. */
    basic_snapshot_loader(basic_snapshot_loader &&) = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_snapshot_loader & operator=(basic_snapshot_loader &&) = default;

    /**
     * @brief Restores entities that were in use during serialization.
     *
     * This function restores the entities that were in use during serialization
     * and gives them the versions they originally had.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename Archive>
    const basic_snapshot_loader & entities(Archive &archive) const {
        static constexpr auto destroyed = false;
        assure(archive, destroyed);
        return *this;
    }

    /**
     * @brief Restores entities that were destroyed during serialization.
     *
     * This function restores the entities that were destroyed during
     * serialization and gives them the versions they originally had.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename Archive>
    const basic_snapshot_loader & destroyed(Archive &archive) const {
        static constexpr auto destroyed = true;
        assure(archive, destroyed);
        return *this;
    }

    /**
     * @brief Restores components and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the component is
     * assigned doesn't exist yet, the loader will take care to create it with
     * the version it originally had.
     *
     * @tparam Component Types of components to restore.
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename... Component, typename Archive>
    const basic_snapshot_loader & component(Archive &archive) const {
        (assign<Component>(archive), ...);
        return *this;
    }

    /**
     * @brief Destroys those entities that have no components.
     *
     * In case all the entities were serialized but only part of the components
     * was saved, it could happen that some of the entities have no components
     * once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A valid loader to continue restoring data.
     */
    const basic_snapshot_loader & orphans() const {
        reg->orphans([this](const auto entt) {
            reg->destroy(entt);
        });

        return *this;
    }

private:
    basic_registry<Entity> *reg;
    force_fn_type *force;
};


/**
 * @brief Utility class for _continuous loading_.
 *
 * A _continuous loader_ is designed to load data from a source registry to a
 * (possibly) non-empty destination. The loader can accomodate in a registry
 * more than one snapshot in a sort of _continuous loading_ that updates the
 * destination one step at a time.<br/>
 * Identifiers that entities originally had are not transferred to the target.
 * Instead, the loader maps remote identifiers to local ones while restoring a
 * snapshot.<br/>
 * An example of use is the implementation of a client-server applications with
 * the requirement of transferring somehow parts of the representation side to
 * side.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_continuous_loader {
    using traits_type = entt_traits<Entity>;

    void destroy(Entity entt) {
        const auto it = remloc.find(entt);

        if(it == remloc.cend()) {
            const auto local = reg->create();
            remloc.emplace(entt, std::make_pair(local, true));
            reg->destroy(local);
        }
    }

    void restore(Entity entt) {
        const auto it = remloc.find(entt);

        if(it == remloc.cend()) {
            const auto local = reg->create();
            remloc.emplace(entt, std::make_pair(local, true));
        } else {
            remloc[entt].first = reg->valid(remloc[entt].first) ? remloc[entt].first : reg->create();
            // set the dirty flag
            remloc[entt].second = true;
        }
    }

    template<typename Other, typename Type, typename Member>
    void update(Other &instance, Member Type:: *member) {
        if constexpr(!std::is_same_v<Other, Type>) {
            return;
        } else if constexpr(std::is_same_v<Member, Entity>) {
            instance.*member = map(instance.*member);
        } else {
            // maybe a container? let's try...
            for(auto &entt: instance.*member) {
                entt = map(entt);
            }
        }
    }

    template<typename Archive>
    void assure(Archive &archive, void(basic_continuous_loader:: *member)(Entity)) {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entt{};
            archive(entt);
            (this->*member)(entt);
        }
    }

    template<typename Component>
    void reset() {
        for(auto &&ref: remloc) {
            const auto local = ref.second.first;

            if(reg->valid(local)) {
                reg->template reset<Component>(local);
            }
        }
    }

    template<typename Other, typename Archive, typename... Type, typename... Member>
    void assign(Archive &archive, [[maybe_unused]] Member Type:: *... member) {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entt{};

            if constexpr(std::is_empty_v<Other>) {
                archive(entt);
                restore(entt);
                reg->template assign_or_replace<Other>(map(entt));
            } else {
                Other instance{};
                archive(entt, instance);
                (update(instance, member), ...);
                restore(entt);
                reg->template assign_or_replace<Other>(map(entt), std::as_const(instance));
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs a loader that is bound to a given registry.
     * @param source A valid reference to a registry.
     */
    basic_continuous_loader(basic_registry<entity_type> &source) ENTT_NOEXCEPT
        : reg{&source}
    {}

    /*! @brief Default move constructor. */
    basic_continuous_loader(basic_continuous_loader &&) = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_continuous_loader & operator=(basic_continuous_loader &&) = default;

    /**
     * @brief Restores entities that were in use during serialization.
     *
     * This function restores the entities that were in use during serialization
     * and creates local counterparts for them if required.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A non-const reference to this loader.
     */
    template<typename Archive>
    basic_continuous_loader & entities(Archive &archive) {
        assure(archive, &basic_continuous_loader::restore);
        return *this;
    }

    /**
     * @brief Restores entities that were destroyed during serialization.
     *
     * This function restores the entities that were destroyed during
     * serialization and creates local counterparts for them if required.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A non-const reference to this loader.
     */
    template<typename Archive>
    basic_continuous_loader & destroyed(Archive &archive) {
        assure(archive, &basic_continuous_loader::destroy);
        return *this;
    }

    /**
     * @brief Restores components and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the component is
     * assigned doesn't exist yet, the loader will take care to create a local
     * counterpart for it.<br/>
     * Members can be either data members of type entity_type or containers of
     * entities. In both cases, the loader will visit them and update the
     * entities by replacing each one with its local counterpart.
     *
     * @tparam Component Type of component to restore.
     * @tparam Archive Type of input archive.
     * @tparam Type Types of components to update with local counterparts.
     * @tparam Member Types of members to update with their local counterparts.
     * @param archive A valid reference to an input archive.
     * @param member Members to update with their local counterparts.
     * @return A non-const reference to this loader.
     */
    template<typename... Component, typename Archive, typename... Type, typename... Member>
    basic_continuous_loader & component(Archive &archive, Member Type:: *... member) {
        (reset<Component>(), ...);
        (assign<Component>(archive, member...), ...);
        return *this;
    }

    /**
     * @brief Helps to purge entities that no longer have a conterpart.
     *
     * Users should invoke this member function after restoring each snapshot,
     * unless they know exactly what they are doing.
     *
     * @return A non-const reference to this loader.
     */
    basic_continuous_loader & shrink() {
        auto it = remloc.begin();

        while(it != remloc.cend()) {
            const auto local = it->second.first;
            bool &dirty = it->second.second;

            if(dirty) {
                dirty = false;
                ++it;
            } else {
                if(reg->valid(local)) {
                    reg->destroy(local);
                }

                it = remloc.erase(it);
            }
        }

        return *this;
    }

    /**
     * @brief Destroys those entities that have no components.
     *
     * In case all the entities were serialized but only part of the components
     * was saved, it could happen that some of the entities have no components
     * once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A non-const reference to this loader.
     */
    basic_continuous_loader & orphans() {
        reg->orphans([this](const auto entt) {
            reg->destroy(entt);
        });

        return *this;
    }

    /**
     * @brief Tests if a loader knows about a given entity.
     * @param entt An entity identifier.
     * @return True if `entity` is managed by the loader, false otherwise.
     */
    bool has(entity_type entt) const ENTT_NOEXCEPT {
        return (remloc.find(entt) != remloc.cend());
    }

    /**
     * @brief Returns the identifier to which an entity refers.
     * @param entt An entity identifier.
     * @return The local identifier if any, the null entity otherwise.
     */
    entity_type map(entity_type entt) const ENTT_NOEXCEPT {
        const auto it = remloc.find(entt);
        entity_type other = null;

        if(it != remloc.cend()) {
            other = it->second.first;
        }

        return other;
    }

private:
    std::unordered_map<Entity, std::pair<Entity, bool>> remloc;
    basic_registry<Entity> *reg;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP

// #include "storage.hpp"
#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP


#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>
#include <cstddef>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/algorithm.hpp"

// #include "sparse_set.hpp"

// #include "entity.hpp"



namespace entt {


/**
 * @brief Basic storage implementation.
 *
 * This class is a refinement of a sparse set that associates an object to an
 * entity. The main purpose of this class is to extend sparse sets to store
 * components in a registry. It guarantees fast access both to the elements and
 * to the entities.
 *
 * @note
 * Entities and objects have the same order. It's guaranteed both in case of raw
 * access (either to entities or objects) and when using input iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `raw` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Temporary objects are returned in
 * place of the instances of the components and raw access isn't available for
 * them.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = std::void_t<>>
class basic_storage: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    template<bool Const>
    class iterator {
        friend class basic_storage<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        iterator(instance_type *ref, const index_type idx) ENTT_NOEXCEPT
            : instances{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = std::conditional_t<Const, const value_type *, value_type *>;
        using reference = std::conditional_t<Const, const value_type &, value_type &>;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return --(*this), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{instances, index-value};
        }

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*instances)[pos];
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*instances)[pos];
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        instance_type *instances;
        index_type index;
    };

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator<false>;
    /*! @brief Constant random access iterator type. */
    using const_iterator_type = iterator<true>;

    /**
     * @brief Increases the capacity of a storage.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) override {
        underlying_type::reserve(cap);
        instances.reserve(cap);
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
        underlying_type::shrink_to_fit();
        instances.shrink_to_fit();
    }

    /**
     * @brief Direct access to the array of objects.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though either `sort` or
     * `respect` has been previously invoked. Internal data structures arrange
     * elements to maximize performance. Accessing them directly gives a
     * performance boost but less guarantees. Use `begin` and `end` if you want
     * to iterate the storage in the expected order.
     *
     * @return A pointer to the array of objects.
     */
    const object_type * raw() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc raw */
    object_type * raw() ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the given type. If
     * the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator_type{&instances, pos};
    }

    /*! @copydoc cbegin */
    inline const_iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    iterator_type begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator_type{&instances, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return const_iterator_type{&instances, {}};
    }

    /*! @copydoc cend */
    inline const_iterator_type end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    iterator_type end() ENTT_NOEXCEPT {
        return iterator_type{&instances, {}};
    }

    /**
     * @brief Returns the object associated with an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    const object_type & get(const entity_type entt) const ENTT_NOEXCEPT {
        return instances[underlying_type::get(entt)];
    }

    /*! @copydoc get */
    inline object_type & get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<object_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entt A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entt) const ENTT_NOEXCEPT {
        return underlying_type::has(entt) ? (instances.data() + underlying_type::get(entt)) : nullptr;
    }

    /*! @copydoc try_get */
    inline object_type * try_get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).try_get(entt));
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * This version accept both types that can be constructed in place directly
     * and types like aggregates that do not work well with a placement new as
     * performed usually under the hood during an _emplace back_.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return The object associated with the entity.
     */
    template<typename... Args>
    object_type & construct(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<object_type>) {
            instances.emplace_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::construct(entt);
        return instances.back();
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects.
     *
     * The object type must be at least default constructible.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return A pointer to the array of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    object_type * batch(It first, It last) {
        static_assert(std::is_default_constructible_v<object_type>);
        const auto skip = instances.size();
        instances.insert(instances.end(), last-first, {});
        // entity goes after component in case constructor throws
        underlying_type::batch(first, last);
        return instances.data() + skip;
    }

    /**
     * @brief Removes an entity from a storage and destroys its object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void destroy(const entity_type entt) override {
        std::swap(instances[underlying_type::get(entt)], instances.back());
        instances.pop_back();
        underlying_type::destroy(entt);
    }

    /**
     * @brief Sort instances according to the given comparison function.
     *
     * Sort the elements so that iterating the storage with a couple of
     * iterators returns them in the expected order. See `begin` and `end` for
     * more details.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * bool(const Type &, const Type &);
     * @endcode
     *
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * The comparison function object received by the sort function object
     * hasn't necessarily the type of the one passed along with the other
     * parameters to this member function.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        std::vector<size_type> copy(instances.size());
        std::iota(copy.begin(), copy.end(), 0);

        if constexpr(std::is_invocable_v<Compare, const object_type &, const object_type &>) {
            static_assert(!std::is_empty_v<object_type>);

            algo(copy.rbegin(), copy.rend(), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[lhs]), std::as_const(instances[rhs]));
            }, std::forward<Args>(args)...);
        } else {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), data = underlying_type::data()](const auto lhs, const auto rhs) {
                return compare(data[lhs], data[rhs]);
            }, std::forward<Args>(args)...);
        }

        for(size_type pos{}, last = copy.size(); pos < last; ++pos) {
            auto curr = pos;
            auto next = copy[curr];

            while(curr != next) {
                const auto lhs = copy[curr];
                const auto rhs = copy[next];
                std::swap(instances[lhs], instances[rhs]);
                underlying_type::swap(lhs, rhs);
                copy[curr] = curr;
                curr = next;
                next = copy[curr];
            }
        }
    }

    /**
     * @brief Sort instances according to the order of the entities in another
     * sparse set.
     *
     * Entities that are part of both the storage are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.
     * Instances are sorted according to the entities to which they belong.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the storage with a couple of iterators returns elements in the
     * expected order after a call to `respect`. See `begin` and `end` for more
     * details.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set<Entity> &other) ENTT_NOEXCEPT override {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = underlying_type::size() - 1;
        const auto *local = underlying_type::data();

        while(pos && from != to) {
            const auto curr = *from;

            if(underlying_type::has(curr)) {
                if(curr != *(local + pos)) {
                    auto candidate = underlying_type::get(curr);
                    std::swap(instances[pos], instances[candidate]);
                    underlying_type::swap(pos, candidate);
                }

                --pos;
            }

            ++from;
        }
    }

    /*! @brief Resets a storage. */
    void reset() override {
        underlying_type::reset();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


/*! @copydoc basic_storage */
template<typename Entity, typename Type>
class basic_storage<Entity, Type, std::enable_if_t<std::is_empty_v<Type>>>: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class basic_storage<Entity, Type>;

        using index_type = typename traits_type::difference_type;

        iterator(const index_type idx) ENTT_NOEXCEPT
            : index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = const value_type *;
        using reference = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return --(*this), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{index-value};
        }

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type) const ENTT_NOEXCEPT {
            return {};
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return nullptr;
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return {};
        }

    private:
        index_type index;
    };

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the given type. If
     * the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator_type{pos};
    }

    /*! @copydoc cbegin */
    inline iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    iterator_type cend() const ENTT_NOEXCEPT {
        return iterator_type{};
    }

    /*! @copydoc cend */
    inline iterator_type end() const ENTT_NOEXCEPT {
        return cend();
    }

    /**
     * @brief Returns the object associated with an entity.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, this function
     * always returns a temporary object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    object_type get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(underlying_type::has(entt));
        return {};
    }
};

/*! @copydoc basic_storage */
template<typename Entity, typename Type>
struct storage: basic_storage<Entity, Type> {};


}


#endif // ENTT_ENTITY_STORAGE_HPP

// #include "entity.hpp"

// #include "group.hpp"
#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP


#include <tuple>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "sparse_set.hpp"

// #include "storage.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Alias for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
struct get_t: type_list<Type...> {};


/**
 * @brief Variable template for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
constexpr get_t<Type...> get{};


/**
 * @brief Group.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_group;


/**
 * @brief Non-owning group.
 *
 * A non-owning group returns all the entities and only the entities that have
 * at least the given components. Moreover, it's guaranteed that the entity list
 * is tightly packed in memory for fast iterations.<br/>
 * In general, non-owning groups don't stay true to the order of any set of
 * components unless users explicitly sort them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.<br/>
 * Moreover, sorting a non-owning group affects all the instance of the same
 * group (it means that users don't have to call `sort` on each instance to sort
 * all of them because they share the set of entities).
 *
 * @warning
 * Lifetime of a group must overcome the one of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Get Types of components observed by the group.
 */
template<typename Entity, typename... Get>
class basic_group<Entity, get_t<Get...>> {
    static_assert(sizeof...(Get) > 0);

    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    // we could use pool_type<Get> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_group(sparse_set<Entity> *ref, storage<Entity, std::remove_const_t<Get>> *... get) ENTT_NOEXCEPT
        : handler{ref},
          pools{get...}
    {}

    template<typename Func, typename... Weak>
    inline void traverse(Func func, type_list<Weak...>) const {
        for(const auto entt: *handler) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Weak>({}))...>) {
                func(std::get<pool_type<Weak> *>(pools)->get(entt)...);
            } else {
                func(entt, std::get<pool_type<Weak> *>(pools)->get(entt)...);
            }
        };
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->size();
    }

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return handler->size();
    }

    /**
     * @brief Returns the number of elements that a group has currently
     * allocated space for.
     * @return Capacity of the group.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return handler->capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        handler->shrink_to_fit();
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->empty();
    }

    /**
     * @brief Checks whether the group is empty.
     * @return True if the group is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return handler->empty();
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Component>
    Component * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->data();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return handler->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the group is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return handler->begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return handler->end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = handler->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * group doesn't contain the given entity.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>(entt))...>{get<Component>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Get &...);
     * void(Get &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        traverse(std::move(func), type_list<Get...>{});
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void less(Func func) const {
        using non_empty_get = type_list_cat_t<std::conditional_t<std::is_empty_v<Get>, type_list<>, type_list<Get>>...>;
        traverse(std::move(func), non_empty_get{});
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Non-owning groups of the same type share with the registry a pool of
     * entities with  its own order that doesn't depend on the order of any pool
     * of components. Users can order the underlying data structure so that it
     * respects the order of the pool of the given component.
     *
     * @note
     * The shared pool of entities and thus its order is affected by the changes
     * to each and every pool that it tracks. Therefore changes to those pools
     * can quickly ruin the order imposed to the pool of entities shared between
     * the non-owning groups.
     *
     * @tparam Component Type of component to use to impose the order.
     */
    template<typename Component>
    void sort() const {
        handler->respect(*std::get<pool_type<Component> *>(pools));
    }

private:
    sparse_set<entity_type> *handler;
    const std::tuple<pool_type<Get> *...> pools;
};


/**
 * @brief Owning group.
 *
 * Owning groups return all the entities and only the entities that have at
 * least the given components. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that the lists of owned components are tightly packed in
 *   memory for even faster iterations and to allow direct access.
 * * They stay true to the order of the owned components and all the owned
 *   components have the same order in memory.
 *
 * The more types of components are owned by a group, the faster it is to
 * iterate them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.
 * Moreover, sorting an owning group affects all the instance of the same group
 * (it means that users don't have to call `sort` on each instance to sort all
 * of them because they share the underlying data structure).
 *
 * @warning
 * Lifetime of a group must overcome the one of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Get Types of components observed by the group.
 * @tparam Owned Types of components owned by the group.
 */
template<typename Entity, typename... Get, typename... Owned>
class basic_group<Entity, get_t<Get...>, Owned...> {
    static_assert(sizeof...(Get) + sizeof...(Owned) > 0);

    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    template<typename Component>
    using component_iterator_type = decltype(std::declval<pool_type<Component>>().begin());

    // we could use pool_type<Type> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_group(const typename basic_registry<Entity>::size_type *sz, storage<Entity, std::remove_const_t<Owned>> *... owned, storage<Entity, std::remove_const_t<Get>> *... get) ENTT_NOEXCEPT
        : length{sz},
          pools{owned..., get...}
    {}

    template<typename Component>
    decltype(auto) from_index(const typename sparse_set<Entity>::size_type index) {
        static_assert(!std::is_empty_v<Component>);

        if constexpr(std::disjunction_v<std::is_same<Component, Owned>...>) {
            return std::as_const(*std::get<pool_type<Component> *>(pools)).raw()[index];
        } else {
            return std::as_const(*std::get<pool_type<Component> *>(pools)).get(data()[index]);
        }
    }

    template<typename Component>
    inline auto swap(int, const std::size_t lhs, const std::size_t rhs)
    -> decltype(std::declval<pool_type<Component>>().raw(), void()) {
        auto *cpool = std::get<pool_type<Component> *>(pools);
        std::swap(cpool->raw()[lhs], cpool->raw()[rhs]);
        cpool->swap(lhs, rhs);
    }

    template<typename Component>
    inline void swap(char, const std::size_t lhs, const std::size_t rhs) {
        std::get<pool_type<Component> *>(pools)->swap(lhs, rhs);
    }

    template<typename Func, typename... Strong, typename... Weak>
    inline void traverse(Func func, type_list<Strong...>, type_list<Weak...>) const {
        auto raw = std::make_tuple((std::get<pool_type<Strong> *>(pools)->end() - *length)...);
        [[maybe_unused]] auto data = std::get<0>(pools)->sparse_set<entity_type>::end() - *length;

        for(auto next = *length; next; --next) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Strong>({}))..., decltype(get<Weak>({}))...>) {
                if constexpr(sizeof...(Weak) == 0) {
                    func(*(std::get<component_iterator_type<Strong>>(raw)++)...);
                } else {
                    const auto entt = *(data++);
                    func(*(std::get<component_iterator_type<Strong>>(raw)++)..., std::get<pool_type<Weak> *>(pools)->get(entt)...);
                }
            } else {
                const auto entt = *(data++);
                func(entt, *(std::get<component_iterator_type<Strong>>(raw)++)..., std::get<pool_type<Weak> *>(pools)->get(entt)...);
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->size();
    }

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return *length;
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->empty();
    }

    /**
     * @brief Checks whether the group is empty.
     * @return True if the group is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return !*length;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.<br/>
     * Moreover, in case the group owns the given component, the range
     * `[raw<Component>(), raw<Component>() + size()]` is such that it contains
     * the instances that are part of the group itself.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Component>
    Component * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.<br/>
     * Moreover, in case the group owns the given component, the range
     * `[data<Component>(), data<Component>() + size()]` is such that it
     * contains the entities that are part of the group itself.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->data();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the group is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->sparse_set<entity_type>::end() - *length;
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->sparse_set<entity_type>::end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = std::get<0>(pools)->find(entt);
        return it != end() && it >= begin() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * group doesn't contain the given entity.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>(entt))...>{get<Component>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Owned &..., Get &...);
     * void(Owned &..., Get &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        traverse(std::move(func), type_list<Owned...>{}, type_list<Get...>{});
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void less(Func func) const {
        using non_empty_owned = type_list_cat_t<std::conditional_t<std::is_empty_v<Owned>, type_list<>, type_list<Owned>>...>;
        using non_empty_get = type_list_cat_t<std::conditional_t<std::is_empty_v<Get>, type_list<>, type_list<Get>>...>;
        traverse(std::move(func), non_empty_owned{}, non_empty_get{});
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * Sort the group so that iterating it with a couple of iterators returns
     * entities and components in the expected order. See `begin` and `end` for
     * more details.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Component &..., const Component &...);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Component` are either owned types or not but still such that they
     * are iterated by the group.<br/>
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * The comparison function object received by the sort function object
     * hasn't necessarily the type of the one passed along with the other
     * parameters to this member function.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @tparam Component Optional types of components to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename... Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        std::vector<size_type> copy(*length);
        std::iota(copy.begin(), copy.end(), 0);

        if constexpr(sizeof...(Component) == 0) {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), data = data()](const auto lhs, const auto rhs) {
                return compare(data[lhs], data[rhs]);
            }, std::forward<Args>(args)...);
        } else {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), this](const auto lhs, const auto rhs) {
                // useless this-> used to suppress a warning with clang
                return compare(this->from_index<Component>(lhs)..., this->from_index<Component>(rhs)...);
            }, std::forward<Args>(args)...);
        }

        for(size_type pos{}, last = copy.size(); pos < last; ++pos) {
            auto curr = pos;
            auto next = copy[curr];

            while(curr != next) {
                const auto lhs = copy[curr];
                const auto rhs = copy[next];
                (swap<Owned>(0, lhs, rhs), ...);
                copy[curr] = curr;
                curr = next;
                next = copy[curr];
            }
        }
    }

private:
    const typename basic_registry<Entity>::size_type *length;
    const std::tuple<pool_type<Owned> *..., pool_type<Get> *...> pools;
};


}


#endif // ENTT_ENTITY_GROUP_HPP

// #include "view.hpp"
#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <iterator>
#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "sparse_set.hpp"

// #include "storage.hpp"

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and picks up a
 * reference to the smallest set of candidate entities in order to get a
 * performance boost when iterate.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structures. See sparse_set and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Component>
class basic_view {
    static_assert(sizeof...(Component) > 1);

    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    template<typename Comp>
    using pool_type = std::conditional_t<std::is_const_v<Comp>, const storage<Entity, std::remove_const_t<Comp>>, storage<Entity, Comp>>;

    template<typename Comp>
    using component_iterator_type = decltype(std::declval<pool_type<Comp>>().begin());

    using underlying_iterator_type = typename sparse_set<Entity>::iterator_type;
    using unchecked_type = std::array<const sparse_set<Entity> *, (sizeof...(Component) - 1)>;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class basic_view<Entity, Component...>;

        using extent_type = typename sparse_set<Entity>::size_type;

        iterator(unchecked_type other, underlying_iterator_type first, underlying_iterator_type last) ENTT_NOEXCEPT
            : unchecked{other},
              begin{first},
              end{last},
              extent{min(std::make_index_sequence<other.size()>{})}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        template<auto... Indexes>
        extent_type min(std::index_sequence<Indexes...>) const ENTT_NOEXCEPT {
            return std::min({ std::get<Indexes>(unchecked)->extent()... });
        }

        bool valid() const ENTT_NOEXCEPT {
            const auto entt = *begin;
            const auto sz = size_type(entt& traits_type::entity_mask);

            return sz < extent && std::all_of(unchecked.cbegin(), unchecked.cend(), [entt](const sparse_set<Entity> *view) {
                return view->has(entt);
            });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::forward_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        unchecked_type unchecked;
        underlying_iterator_type begin;
        underlying_iterator_type end;
        extent_type extent;
    };

    // we could use pool_type<Component> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_view(storage<Entity, std::remove_const_t<Component>> *... ref) ENTT_NOEXCEPT
        : pools{ref...}
    {}

    const sparse_set<Entity> * candidate() const ENTT_NOEXCEPT {
        return std::min({ static_cast<const sparse_set<Entity> *>(std::get<pool_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    unchecked_type unchecked(const sparse_set<Entity> *view) const ENTT_NOEXCEPT {
        unchecked_type other{};
        typename unchecked_type::size_type pos{};
        ((std::get<pool_type<Component> *>(pools) == view ? nullptr : (other[pos++] = std::get<pool_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename Other>
    inline decltype(auto) get([[maybe_unused]] component_iterator_type<Comp> it, [[maybe_unused]] pool_type<Other> *cpool, [[maybe_unused]] const Entity entt) const ENTT_NOEXCEPT {
        if constexpr(std::is_same_v<Comp, Other>) {
            return *it;
        } else {
            return cpool->get(entt);
        }
    }

    template<typename Comp, typename Func, typename... Other, typename... Type>
    void traverse(Func func, type_list<Other...>, type_list<Type...>) const {
        const auto end = std::get<pool_type<Comp> *>(pools)->sparse_set<Entity>::end();
        auto begin = std::get<pool_type<Comp> *>(pools)->sparse_set<Entity>::begin();

        if constexpr(std::disjunction_v<std::is_same<Comp, Type>...>) {
            std::for_each(begin, end, [raw = std::get<pool_type<Comp> *>(pools)->begin(), &func, this](const auto entity) mutable {
                auto curr = raw++;

                if((std::get<pool_type<Other> *>(pools)->has(entity) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entity)...);
                    } else {
                        func(entity, get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entity)...);
                    }
                }
            });
        } else {
            std::for_each(begin, end, [&func, this](const auto entity) mutable {
                if((std::get<pool_type<Other> *>(pools)->has(entity) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(std::get<pool_type<Type> *>(pools)->get(entity)...);
                    } else {
                        func(entity, std::get<pool_type<Type> *>(pools)->get(entity)...);
                    }
                }
            });
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Comp Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Comp>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->size();
    }

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return std::min({ std::get<pool_type<Component> *>(pools)->size()... });
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Comp Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Comp>
    bool empty() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->empty();
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return (std::get<pool_type<Component> *>(pools)->empty() || ...);
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Comp>(), raw<Comp>() + size<Comp>()]` is always a valid range, even
     * if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Comp>
    Comp * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Comp>(), data<Comp>() + size<Comp>()]` is always a valid range,
     * even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Comp>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        const auto *view = candidate();
        return iterator_type{unchecked(view), view->begin(), view->end()};
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        const auto *view = candidate();
        return iterator_type{unchecked(view), view->end(), view->end()};
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto *view = candidate();
        iterator_type it{unchecked(view), view->find(entt), view->end()};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 1) {
            return (std::get<pool_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Comp>(entt))...>{get<Comp>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &...);
     * void(Component &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        const auto *view = candidate();
        ((std::get<pool_type<Component> *>(pools) == view ? each<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &...);
     * void(Component &...);
     * @endcode
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    inline void each(Func func) const {
        using other_type = type_list_cat_t<std::conditional_t<std::is_same_v<Comp, Component>, type_list<>, type_list<Component>>...>;
        traverse<Comp>(std::move(func), other_type{}, type_list<Component...>{});
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void less(Func func) const {
        const auto *view = candidate();
        ((std::get<pool_type<Component> *>(pools) == view ? less<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    inline void less(Func func) const {
        using other_type = type_list_cat_t<std::conditional_t<std::is_same_v<Comp, Component>, type_list<>, type_list<Component>>...>;
        using non_empty_type = type_list_cat_t<std::conditional_t<std::is_empty_v<Component>, type_list<>, type_list<Component>>...>;
        traverse<Comp>(std::move(func), other_type{}, non_empty_type{});
    }

private:
    const std::tuple<pool_type<Component> *...> pools;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See sparse_set and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pool of the given component in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class basic_view<Entity, Component> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    basic_view(pool_type *ref) ENTT_NOEXCEPT
        : pool{ref}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = Component;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of components.
     */
    raw_type * raw() const ENTT_NOEXCEPT {
        return pool->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return pool->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * component.
     *
     * The returned iterator points to the first entity that has the given
     * component. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given component.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given component.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given component. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given component.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = pool->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    decltype(auto) get(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));
        return pool->get(entt);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to its component. The _constness_ of the
     * component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        if constexpr(std::is_invocable_v<Func, decltype(get({}))>) {
            std::for_each(pool->begin(), pool->end(), std::move(func));
        } else {
            std::for_each(pool->sparse_set<Entity>::begin(), pool->sparse_set<Entity>::end(), [&func, raw = pool->begin()](const auto entt) mutable {
                func(entt, *(raw++));
            });
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to its component if it's a non-empty one.
     * The _constness_ of the component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms in case the component isn't an empty one:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * In case the component is an empty one instead, the following forms are
     * accepted:
     *
     * @code{.cpp}
     * void(const entity_type);
     * void();
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void less(Func func) const {
        if constexpr(std::is_empty_v<Component>) {
            if constexpr(std::is_invocable_v<Func>) {
                for(auto pos = pool->size(); pos; --pos) {
                    func();
                }
            } else {
                std::for_each(pool->sparse_set<Entity>::begin(), pool->sparse_set<Entity>::end(), std::move(func));
            }
        } else {
            each(std::move(func));
        }
    }

private:
    pool_type *pool;
};


}


#endif // ENTT_ENTITY_VIEW_HPP

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Alias for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
struct exclude_t: type_list<Type...> {};


/**
 * @brief Variable template for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
constexpr exclude_t<Type...> exclude{};


/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components and thus
 * create views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_registry {
    using context_family = family<struct internal_registry_context_family>;
    using component_family = family<struct internal_registry_component_family>;
    using traits_type = entt_traits<Entity>;

    template<typename Component>
    struct pool_handler: storage<Entity, Component> {
        using reference_type = std::conditional_t<std::is_empty_v<Component>, const Component &, Component &>;

        sigh<void(basic_registry &, const Entity, reference_type)> on_construct;
        sigh<void(basic_registry &, const Entity, reference_type)> on_replace;
        sigh<void(basic_registry &, const Entity)> on_destroy;
        void *group{};

        pool_handler() ENTT_NOEXCEPT = default;

        pool_handler(const storage<Entity, Component> &other)
            : storage<Entity, Component>{other}
        {}

        template<typename... Args>
        decltype(auto) assign(basic_registry &registry, const Entity entt, Args &&... args) {
            if constexpr(std::is_empty_v<Component>) {
                storage<Entity, Component>::construct(entt);
                on_construct.publish(registry, entt, Component{});
                return Component{std::forward<Args>(args)...};
            } else {
                auto &component = storage<Entity, Component>::construct(entt, std::forward<Args>(args)...);
                on_construct.publish(registry, entt, component);
                return component;
            }
        }

        template<typename It>
        Component * batch(basic_registry &registry, It first, It last) {
            Component *component = nullptr;

            if constexpr(std::is_empty_v<Component>) {
                storage<Entity, Component>::batch(first, last);

                if(!on_construct.empty()) {
                    std::for_each(first, last, [&registry, this](const auto entt) {
                        on_construct.publish(registry, entt, Component{});
                    });
                }
            } else {
                component = storage<Entity, Component>::batch(first, last);

                if(!on_construct.empty()) {
                    std::for_each(first, last, [&registry, component, this](const auto entt) mutable {
                        on_construct.publish(registry, entt, *(component++));
                    });
                }
            }

            return component;
        }

        void remove(basic_registry &registry, const Entity entt) {
            on_destroy.publish(registry, entt);
            storage<Entity, Component>::destroy(entt);
        }

        template<typename... Args>
        decltype(auto) replace(basic_registry &registry, const Entity entt, Args &&... args) {
            if constexpr(std::is_empty_v<Component>) {
                ENTT_ASSERT((storage<Entity, Component>::has(entt)));
                on_replace.publish(registry, entt, Component{});
                return Component{std::forward<Args>(args)...};
            } else {
                Component component{std::forward<Args>(args)...};
                on_replace.publish(registry, entt, component);
                return (storage<Entity, Component>::get(entt) = std::move(component));
            }
        }
    };

    template<typename Component>
    using pool_type = pool_handler<std::decay_t<Component>>;

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get>
    struct group_handler<type_list<Exclude...>, type_list<Get...>>: sparse_set<Entity> {
        template<typename Component, typename... Args>
        void maybe_valid_if(basic_registry &reg, const Entity entt, const Args &...) {
            if constexpr(std::disjunction_v<std::is_same<Get, Component>...>) {
                if(((std::is_same_v<Component, Get> || reg.pool<Get>()->has(entt)) && ...) && !(reg.pool<Exclude>()->has(entt) || ...)) {
                    this->construct(entt);
                }
            } else if constexpr(std::disjunction_v<std::is_same<Exclude, Component>...>) {
                if((reg.pool<Get>()->has(entt) && ...) && ((std::is_same_v<Exclude, Component> || !reg.pool<Exclude>()->has(entt)) && ...)) {
                    this->construct(entt);
                }
            }
        }

        template<typename... Args>
        void discard_if(basic_registry &, const Entity entt, const Args &...) {
            if(this->has(entt)) {
                this->destroy(entt);
            }
        }
    };

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<type_list<Exclude...>, type_list<Get...>, Owned...>: sparse_set<Entity> {
        std::size_t owned{};

        template<typename Component, typename... Args>
        void maybe_valid_if(basic_registry &reg, const Entity entt, const Args &...) {
            const auto cpools = std::make_tuple(reg.pool<Owned>()...);

            if constexpr(std::disjunction_v<std::is_same<Owned, Component>..., std::is_same<Get, Component>...>) {
                if(((std::is_same_v<Component, Owned> || std::get<pool_type<Owned> *>(cpools)->has(entt)) && ...)
                        && ((std::is_same_v<Component, Get> || reg.pool<Get>()->has(entt)) && ...)
                        && !(reg.pool<Exclude>()->has(entt) || ...))
                {
                    const auto pos = this->owned++;
                    (reg.swap<Owned>(0, std::get<pool_type<Owned> *>(cpools), entt, pos), ...);
                }
            } else if constexpr(std::disjunction_v<std::is_same<Exclude, Component>...>) {
                if((std::get<pool_type<Owned> *>(cpools)->has(entt) && ...)
                        && (reg.pool<Get>()->has(entt) && ...)
                        && ((std::is_same_v<Exclude, Component> || !reg.pool<Exclude>()->has(entt)) && ...))
                {
                    const auto pos = this->owned++;
                    (reg.swap<Owned>(0, std::get<pool_type<Owned> *>(cpools), entt, pos), ...);
                }
            }
        }

        template<typename... Args>
        void discard_if(basic_registry &reg, const Entity entt, const Args &...) {
            const auto cpools = std::make_tuple(reg.pool<Owned>()...);

            if(std::get<0>(cpools)->has(entt) && std::get<0>(cpools)->sparse_set<Entity>::get(entt) < this->owned) {
                const auto pos = --this->owned;
                (reg.swap<Owned>(0, std::get<pool_type<Owned> *>(cpools), entt, pos), ...);
            }
        }
    };

    struct pool_data {
        std::unique_ptr<sparse_set<Entity>> pool;
        std::unique_ptr<sparse_set<Entity>> (* clone)(const sparse_set<Entity> &);
        void (* remove)(basic_registry &, const Entity);
        ENTT_ID_TYPE runtime_type;
    };

    struct group_data {
        const std::size_t extent[3];
        std::unique_ptr<void, void(*)(void *)> group;
        bool(* const is_same)(const ENTT_ID_TYPE *);
    };

    struct ctx_variable {
        std::unique_ptr<void, void(*)(void *)> value;
        ENTT_ID_TYPE runtime_type;
    };

    template<typename Type, typename Family>
    static ENTT_ID_TYPE runtime_type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Type>) {
            return named_type_traits<Type>::value;
        } else {
            return Family::template type<Type>;
        }
    }

    void release(const Entity entity) {
        // lengthens the implicit list of destroyed entities
        const auto entt = entity & traits_type::entity_mask;
        const auto version = ((entity >> traits_type::entity_shift) + 1) << traits_type::entity_shift;
        const auto node = (available ? next : ((entt + 1) & traits_type::entity_mask)) | version;
        entities[entt] = node;
        next = entt;
        ++available;
    }

    template<typename Component>
    inline auto swap(int, pool_type<Component> *cpool, const Entity entt, const std::size_t pos)
    -> decltype(std::swap(cpool->get(entt), cpool->raw()[pos]), void()) {
        std::swap(cpool->get(entt), cpool->raw()[pos]);
        cpool->swap(cpool->sparse_set<Entity>::get(entt), pos);
    }

    template<typename Component>
    inline void swap(char, pool_type<Component> *cpool, const Entity entt, const std::size_t pos) {
        cpool->swap(cpool->sparse_set<Entity>::get(entt), pos);
    }

    template<typename Component>
    inline const auto * pool() const ENTT_NOEXCEPT {
        const auto ctype = type<Component>();

        if constexpr(is_named_type_v<Component>) {
            const auto it = std::find_if(pools.begin()+skip_family_pools, pools.end(), [ctype](const auto &candidate) {
                return candidate.runtime_type == ctype;
            });

            return it == pools.cend() ? nullptr : static_cast<const pool_type<Component> *>(it->pool.get());
        } else {
            return ctype < skip_family_pools ? static_cast<const pool_type<Component> *>(pools[ctype].pool.get()) : nullptr;
        }
    }

    template<typename Component>
    inline auto * pool() ENTT_NOEXCEPT {
        return const_cast<pool_type<Component> *>(std::as_const(*this).template pool<Component>());
    }

    template<typename Component>
    auto * assure() {
        const auto ctype = type<Component>();
        pool_data *pdata = nullptr;

        if constexpr(is_named_type_v<Component>) {
            const auto it = std::find_if(pools.begin()+skip_family_pools, pools.end(), [ctype](const auto &candidate) {
                return candidate.runtime_type == ctype;
            });

            pdata = (it == pools.cend() ? &pools.emplace_back() : &(*it));
        } else {
            if(!(ctype < skip_family_pools)) {
                pools.reserve(pools.size()+ctype-skip_family_pools+1);

                while(!(ctype < skip_family_pools)) {
                    pools.emplace(pools.begin()+(skip_family_pools++), pool_data{});
                }
            }

            pdata = &pools[ctype];
        }

        if(!pdata->pool) {
            pdata->runtime_type = ctype;
            pdata->pool = std::make_unique<pool_type<Component>>();

            pdata->clone = +[](const sparse_set<Entity> &other) -> std::unique_ptr<sparse_set<Entity>> {
                if constexpr(std::is_copy_constructible_v<std::decay_t<Component>>) {
                    return std::make_unique<pool_type<Component>>(static_cast<const pool_type<Component> &>(other));
                } else {
                    return nullptr;
                }
            };

            pdata->remove = +[](basic_registry &registry, const Entity entt) {
                registry.pool<Component>()->remove(registry, entt);
            };
        }

        return static_cast<pool_type<Component> *>(pdata->pool.get());
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Unsigned integer type. */
    using component_type = ENTT_ID_TYPE;

    /*! @brief Default constructor. */
    basic_registry() ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;

    /**
     * @brief Returns the numeric identifier of a component.
     *
     * The given component doesn't need to be necessarily in use.<br/>
     * Do not use this functionality to generate numeric identifiers for types
     * at runtime. They aren't guaranteed to be stable between different runs.
     *
     * @tparam Component Type of component to query.
     * @return Runtime numeric identifier of the given type of component.
     */
    template<typename Component>
    inline static component_type type() ENTT_NOEXCEPT {
        return runtime_type<Component, component_family>();
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->size() : size_type{};
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    size_type size() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return Number of entities still in use.
     */
    size_type alive() const ENTT_NOEXCEPT {
        return entities.size() - available;
    }

    /**
     * @brief Increases the capacity of the pool for the given component.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @tparam Component Type of component for which to reserve storage.
     * @param cap Desired capacity.
     */
    template<typename Component>
    void reserve(const size_type cap) {
        assure<Component>()->reserve(cap);
    }

    /**
     * @brief Increases the capacity of a registry in terms of entities.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        entities.reserve(cap);
    }

    /**
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    size_type capacity() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->capacity() : size_type{};
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return entities.capacity();
    }

    /**
     * @brief Requests the removal of unused capacity for a given component.
     * @tparam Component Type of component for which to reclaim unused capacity.
     */
    template<typename Component>
    void shrink_to_fit() {
        assure<Component>()->shrink_to_fit();
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->empty() : true;
    }

    /**
     * @brief Checks if there exists at least an entity still in use.
     * @return True if at least an entity is still in use, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return entities.size() == available;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * There are no guarantees on the order of the components. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @note
     * Empty components aren't explicitly instantiated. Only one instance of the
     * given type is created. Therefore, this function always returns a pointer
     * to that instance.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    const Component * raw() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->raw() : nullptr;
    }

    /*! @copydoc raw */
    template<typename Component>
    inline Component * raw() ENTT_NOEXCEPT {
        return const_cast<Component *>(std::as_const(*this).template raw<Component>());
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * There are no guarantees on the order of the entities. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->data() : nullptr;
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    bool valid(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity & traits_type::entity_mask;
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(entity >> traits_type::entity_shift);
    }

    /**
     * @brief Returns the actual version for an entity identifier.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the registry results
     * in undefined behavior. An entity belongs to the registry even if it has
     * been previously destroyed and/or recycled.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * registry doesn't own the given entity.
     *
     * @param entity A valid entity identifier.
     * @return Actual version for the given entity identifier.
     */
    version_type current(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        ENTT_ASSERT(pos < entities.size());
        return version_type(entities[pos] >> traits_type::entity_shift);
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `valid` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * The returned entity has assigned the given components, if any. The
     * components must be at least default constructible. A compilation error
     * will occur otherwhise.
     *
     * @tparam Component Types of components to assign to the entity.
     * @return A valid entity identifier if the component list is empty, a tuple
     * containing the entity identifier and the references to the components
     * just created otherwise.
     */
    template<typename... Component>
    decltype(auto) create() {
        entity_type entity;

        if(available) {
            const auto entt = next;
            const auto version = entities[entt] & (traits_type::version_mask << traits_type::entity_shift);
            next = entities[entt] & traits_type::entity_mask;
            entity = entt | version;
            entities[entt] = entity;
            --available;
        } else {
            entity = entities.emplace_back(entity_type(entities.size()));
            // traits_type::entity_mask is reserved to allow for null identifiers
            ENTT_ASSERT(entity < traits_type::entity_mask);
        }

        if constexpr(sizeof...(Component) == 0) {
            return entity;
        } else {
            return std::tuple<entity_type, decltype(assign<Component>(entity))...>{entity, assign<Component>(entity)...};
        }
    }

    /**
     * @brief Assigns each element in a range an entity.
     *
     * @sa create
     *
     * @tparam Component Types of components to assign to the entity.
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     * @return No return value if the component list is empty, a tuple
     * containing the pointers to the arrays of components just created and
     * sorted the same of the entities otherwise.
     */
    template<typename... Component, typename It>
    auto create(It first, It last) {
        static_assert(std::is_convertible_v<entity_type, typename std::iterator_traits<It>::value_type>);
        const auto length = size_type(std::distance(first, last));
        const auto sz = std::min(available, length);
        [[maybe_unused]] entity_type candidate{};

        available -= sz;

        const auto tail = std::generate_n(first, sz, [&candidate, this]() mutable {
            if constexpr(sizeof...(Component) > 0) {
                candidate = std::max(candidate, next);
            } else {
                // suppress warnings
                (void)candidate;
            }

            const auto entt = next;
            const auto version = entities[entt] & (traits_type::version_mask << traits_type::entity_shift);
            next = entities[entt] & traits_type::entity_mask;
            return (entities[entt] = entt | version);
        });

        std::generate(tail, last, [this]() {
            return entities.emplace_back(entity_type(entities.size()));
        });

        if constexpr(sizeof...(Component) > 0) {
            return std::make_tuple(assure<Component>()->batch(*this, first, last)...);
        }
    }

    /**
     * @brief Destroys an entity and lets the registry recycle the identifier.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time. In case entity identifers are stored around,
     * the `valid` member function can be used to know if they are still valid
     * or the entity has been destroyed and potentially recycled.
     *
     * @warning
     * In case there are listeners that observe the destruction of components
     * and assign other components to the entity in their bodies, the result of
     * invoking this function may not be as expected. In the worst case, it
     * could lead to undefined behavior. An assertion will abort the execution
     * at runtime in debug mode if a violation is detected.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool && pdata.pool->has(entity)) {
                pdata.remove(*this, entity);
            }
        };

        // just a way to protect users from listeners that attach components
        ENTT_ASSERT(orphan(entity));
        release(entity);
    }

    /**
     * @brief Destroys all the entities in a range.
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void destroy(It first, It last) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool) {
                std::for_each(first, last, [&pdata, this](const auto entity) {
                    if(pdata.pool->has(entity)) {
                        pdata.remove(*this, entity);
                    }
                });
            }
        };

        // just a way to protect users from listeners that attach components
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return orphan(entity); }));

        std::for_each(first, last, [this](const auto entity) {
            release(entity);
        });
    }

    /**
     * @brief Assigns the given component to an entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * @warning
     * Attempting to use an invalid entity or to assign a component to an entity
     * that already owns it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity already owns an instance of the given
     * component.
     *
     * @tparam Component Type of component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) assign(const entity_type entity, [[maybe_unused]] Args &&... args) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>()->assign(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to remove a component from an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to remove.
     * @param entity A valid entity identifier.
     */
    template<typename Component>
    void remove(const entity_type entity) {
        ENTT_ASSERT(valid(entity));
        pool<Component>()->remove(*this, entity);
    }

    /**
     * @brief Checks if an entity has all the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has all the components, false otherwise.
     */
    template<typename... Component>
    bool has(const entity_type entity) const ENTT_NOEXCEPT {
        ENTT_ASSERT(valid(entity));
        [[maybe_unused]] const auto cpools = std::make_tuple(pool<Component>()...);
        return ((std::get<const pool_type<Component> *>(cpools) ? std::get<const pool_type<Component> *>(cpools)->has(entity) : false) && ...);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (pool<Component>()->get(entity), ...);
        } else {
            return std::tuple<decltype(get<Component>(entity))...>{get<Component>(entity)...};
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    inline decltype(auto) get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (pool<Component>()->get(entity), ...);
        } else {
            return std::tuple<decltype(get<Component>(entity))...>{get<Component>(entity)...};
        }
    }

    /**
     * @brief Returns a reference to the given component for an entity.
     *
     * In case the entity doesn't own the component, the parameters provided are
     * used to construct it.<br/>
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.get<Component>(entity) : registry.assign<Component>(entity, args...);
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to get.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return Reference to the component owned by the entity.
     */
    template<typename Component, typename... Args>
    decltype(auto) get_or_assign(const entity_type entity, Args &&... args) ENTT_NOEXCEPT {
        ENTT_ASSERT(valid(entity));
        auto *cpool = assure<Component>();
        return cpool->has(entity) ? cpool->get(entity) : cpool->assign(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return Pointers to the components owned by the entity.
     */
    template<typename... Component>
    auto try_get([[maybe_unused]] const entity_type entity) const ENTT_NOEXCEPT {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            const auto cpools = std::make_tuple(pool<Component>()...);
            return ((std::get<const pool_type<Component> *>(cpools) ? std::get<const pool_type<Component> *>(cpools)->try_get(entity) : nullptr), ...);
        } else {
            return std::tuple<std::add_const_t<Component> *...>{try_get<Component>(entity)...};
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    inline auto try_get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component *>(std::as_const(*this).template try_get<Component>(entity)), ...);
        } else {
            return std::tuple<Component *...>{try_get<Component>(entity)...};
        }
    }

    /**
     * @brief Replaces the given component for an entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * @warning
     * Attempting to use an invalid entity or to replace a component of an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        return pool<Component>()->replace(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.assign<Component>(entity, args...);
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) assign_or_replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto *cpool = assure<Component>();
        return cpool->has(entity) ? cpool->replace(*this, entity, std::forward<Args>(args)...) : cpool->assign(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given component is created and assigned to
     * an entity.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity, Component &);
     * @endcode
     *
     * Listeners are invoked **after** the component has been assigned to the
     * entity. The order of invocation of the listeners isn't guaranteed.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_construct() ENTT_NOEXCEPT {
        return assure<Component>()->on_construct.sink();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is explicitly replaced.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity, Component &);
     * @endcode
     *
     * Listeners are invoked **before** the component has been replaced. The
     * order of invocation of the listeners isn't guaranteed.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_replace() ENTT_NOEXCEPT {
        return assure<Component>()->on_replace.sink();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is removed from an entity and
     * thus destroyed.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** the component has been removed from the
     * entity. The order of invocation of the listeners isn't guaranteed.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_destroy() ENTT_NOEXCEPT {
        return assure<Component>()->on_destroy.sink();
    }

    /**
     * @brief Sorts the pool of entities for the given component.
     *
     * The order of the elements in a pool is highly affected by assignments
     * of components to entities and deletions. Components are arranged to
     * maximize the performance during iterations and users should not make any
     * assumption on the order.<br/>
     * This function can be used to impose an order to the elements in the pool
     * of the given component. The order is kept valid until a component of the
     * given type is assigned or removed from an entity.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * bool(const Component &, const Component &);
     * @endcode
     *
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * The comparison funtion object received by the sort function object hasn't
     * necessarily the type of the one passed along with the other parameters to
     * this member function.
     *
     * @warning
     * Pools of components that are owned by a group cannot be sorted.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam Component Type of components to sort.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!owned<Component>());
        assure<Component>()->sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    /**
     * @brief Sorts two pools of components in the same way.
     *
     * The order of the elements in a pool is highly affected by assignments
     * of components to entities and deletions. Components are arranged to
     * maximize the performance during iterations and users should not make any
     * assumption on the order.
     *
     * It happens that different pools of components must be sorted the same way
     * because of runtime and/or performance constraints. This function can be
     * used to order a pool of components according to the order between the
     * entities in another pool of components.
     *
     * @b How @b it @b works
     *
     * Being `A` and `B` the two sets where `B` is the master (the one the order
     * of which rules) and `A` is the slave (the one to sort), after a call to
     * this function an iterator for `A` will return the entities according to
     * the following rules:
     *
     * * All the entities in `A` that are also in `B` are returned first
     *   according to the order they have in `B`.
     * * All the entities in `A` that are not in `B` are returned in no
     *   particular order after all the other entities.
     *
     * Any subsequent change to `B` won't affect the order in `A`.
     *
     * @warning
     * Pools of components that are owned by a group cannot be sorted.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ENTT_ASSERT(!owned<To>());
        assure<To>()->respect(*assure<From>());
    }

    /**
     * @brief Resets the given component for an entity.
     *
     * If the entity has an instance of the component, this function removes the
     * component from the entity. Otherwise it does nothing.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to reset.
     * @param entity A valid entity identifier.
     */
    template<typename Component>
    void reset(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        if(auto *cpool = assure<Component>(); cpool->has(entity)) {
            cpool->remove(*this, entity);
        }
    }

    /**
     * @brief Resets the pool of the given component.
     *
     * For each entity that has an instance of the given component, the
     * component itself is removed and thus destroyed.
     *
     * @tparam Component Type of component whose pool must be reset.
     */
    template<typename Component>
    void reset() {
        if(auto *cpool = assure<Component>(); cpool->on_destroy.empty()) {
            // no group set, otherwise the signal wouldn't be empty
            cpool->reset();
        } else {
            for(const auto entity: static_cast<const sparse_set<entity_type> &>(*cpool)) {
                cpool->remove(*this, entity);
            }
        }
    }

    /**
     * @brief Resets a whole registry.
     *
     * Destroys all the entities. After a call to `reset`, all the entities
     * still in use are recycled with a new version number. In case entity
     * identifers are stored around, the `valid` member function can be used
     * to know if they are still valid.
     */
    void reset() {
        each([this](const auto entity) {
            // useless this-> used to suppress a warning with clang
            this->destroy(entity);
        });
    }

    /**
     * @brief Iterates all the entities that are still in use.
     *
     * The function object is invoked for each entity that is still in use.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Entity);
     * @endcode
     *
     * This function is fairly slow and should not be used frequently. However,
     * it's useful for iterating all the entities still in use, regardless of
     * their components.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        static_assert(std::is_invocable_v<Func, entity_type>);

        if(available) {
            for(auto pos = entities.size(); pos; --pos) {
                const auto curr = entity_type(pos - 1);
                const auto entity = entities[curr];
                const auto entt = entity & traits_type::entity_mask;

                if(curr == entt) {
                    func(entity);
                }
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        }
    }

    /**
     * @brief Checks if an entity has components assigned.
     * @param entity A valid entity identifier.
     * @return True if the entity has no components assigned, false otherwise.
     */
    bool orphan(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        bool orphan = true;

        for(std::size_t pos{}, last = pools.size(); pos < last && orphan; ++pos) {
            const auto &pdata = pools[pos];
            orphan = !(pdata.pool && pdata.pool->has(entity));
        }

        return orphan;
    }

    /**
     * @brief Iterates orphans and applies them the given function object.
     *
     * The function object is invoked for each entity that is still in use and
     * has no components assigned.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Entity);
     * @endcode
     *
     * This function can be very slow and should not be used frequently.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void orphans(Func func) const {
        static_assert(std::is_invocable_v<Func, entity_type>);

        each([&func, this](const auto entity) {
            if(orphan(entity)) {
                func(entity);
            }
        });
    }

    /**
     * @brief Returns a view for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Views do their best to iterate the smallest set of candidate entities.
     * In particular:
     *
     * * Single component views are incredibly fast and iterate a packed array
     *   of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     *   component and pick up a reference to the smallest set of candidates to
     *   test for the given components.
     *
     * Views in no way affect the functionalities of the registry nor those of
     * the underlying pools.
     *
     * @note
     * Multi component views are pretty fast. However their performance tend to
     * degenerate when the number of components to iterate grows up and the most
     * of the entities have all the given components.<br/>
     * To get a performance boost, consider using a group instead.
     *
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename... Component>
    entt::basic_view<Entity, Component...> view() {
        return { assure<Component>()... };
    }

    /*! @copydoc view */
    template<typename... Component>
    inline entt::basic_view<Entity, Component...> view() const {
        static_assert(std::conjunction_v<std::is_const<Component>...>);
        return const_cast<basic_registry *>(this)->view<Component...>();
    }

    /**
     * @brief Checks whether a given component belongs to a group.
     * @tparam Component Type of component in which one is interested.
     * @return True if the component belongs to a group, false otherwise.
     */
    template<typename Component>
    bool owned() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool && cpool->group;
    }

    /**
     * @brief Returns a group for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a group after the use. Creating and destroying a
     * group is an incredibly cheap operation because they do not require any
     * type of initialization, but for the first time they are requested.<br/>
     * As a rule of thumb, storing a group should never be an option.
     *
     * Groups support exclusion lists and can own types of components. The more
     * types are owned by a group, the faster it is to iterate entities and
     * components.<br/>
     * However, groups also affect some features of the registry such as the
     * creation and destruction of components, which will consequently be
     * slightly slower (nothing that can be noticed in most cases).
     *
     * @note
     * Pools of components that are owned by a group cannot be sorted anymore.
     * The group takes the ownership of the pools and arrange components so as
     * to iterate them as fast as possible.
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    inline entt::basic_group<Entity, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0);
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1);

        using handler_type = group_handler<type_list<Exclude...>, type_list<Get...>, Owned...>;

        const std::size_t extent[] = { sizeof...(Owned), sizeof...(Get), sizeof...(Exclude) };
        const ENTT_ID_TYPE types[] = { type<Owned>()..., type<Get>()..., type<Exclude>()... };
        handler_type *curr = nullptr;

        if(auto it = std::find_if(groups.begin(), groups.end(), [&extent, &types](auto &&gdata) {
            return std::equal(std::begin(extent), std::end(extent), gdata.extent) && gdata.is_same(types);
        }); it != groups.cend())
        {
            curr = static_cast<handler_type *>(it->group.get());
        }

        if(!curr) {
            ENTT_ASSERT(!(owned<Owned>() || ...));

            groups.push_back(group_data{
                { sizeof...(Owned), sizeof...(Get), sizeof...(Exclude) },
                decltype(group_data::group){new handler_type{}, +[](void *gptr) { delete static_cast<handler_type *>(gptr); }},
                +[](const ENTT_ID_TYPE *other) {
                    const std::size_t ctypes[] = { type<Owned>()..., type<Get>()..., type<Exclude>()... };
                    return std::equal(std::begin(ctypes), std::end(ctypes), other);
                }
            });

            const auto cpools = std::make_tuple(assure<Owned>()..., assure<Get>()..., assure<Exclude>()...);
            curr = static_cast<handler_type *>(groups.back().group.get());

            ((std::get<pool_type<Owned> *>(cpools)->group = curr), ...);
            (std::get<pool_type<Owned> *>(cpools)->on_construct.sink().template connect<&handler_type::template maybe_valid_if<Owned, Owned>>(curr), ...);
            (std::get<pool_type<Owned> *>(cpools)->on_destroy.sink().template connect<&handler_type::template discard_if<>>(curr), ...);

            (std::get<pool_type<Get> *>(cpools)->on_construct.sink().template connect<&handler_type::template maybe_valid_if<Get, Get>>(curr), ...);
            (std::get<pool_type<Get> *>(cpools)->on_destroy.sink().template connect<&handler_type::template discard_if<>>(curr), ...);

            (std::get<pool_type<Exclude> *>(cpools)->on_destroy.sink().template connect<&handler_type::template maybe_valid_if<Exclude>>(curr), ...);
            (std::get<pool_type<Exclude> *>(cpools)->on_construct.sink().template connect<&handler_type::template discard_if<Exclude>>(curr), ...);

            const auto *cpool = std::min({
                static_cast<sparse_set<Entity> *>(std::get<pool_type<Owned> *>(cpools))...,
                static_cast<sparse_set<Entity> *>(std::get<pool_type<Get> *>(cpools))...
            }, [](const auto *lhs, const auto *rhs) {
                return lhs->size() < rhs->size();
            });

            // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
            std::for_each(cpool->data(), cpool->data() + cpool->size(), [curr, &cpools, this](const auto entity) {
                if((std::get<pool_type<Owned> *>(cpools)->has(entity) && ...)
                        && (std::get<pool_type<Get> *>(cpools)->has(entity) && ...)
                        && !(std::get<pool_type<Exclude> *>(cpools)->has(entity) || ...))
                {
                    if constexpr(sizeof...(Owned) == 0) {
                        curr->construct(entity);
                        // suppress warnings
                        (void)this;
                    } else {
                        const auto pos = curr->owned++;
                        // useless this-> used to suppress a warning with clang
                        (this->swap<Owned>(0, std::get<pool_type<Owned> *>(cpools), entity, pos), ...);
                    }
                }
            });
        }

        if constexpr(sizeof...(Owned) == 0) {
            return { static_cast<sparse_set<Entity> *>(curr), pool<Get>()... };
        } else {
            return { &curr->owned, pool<Owned>()... , pool<Get>()... };
        }
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Get, typename... Exclude>
    inline entt::basic_group<Entity, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>..., std::is_const<Get>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(entt::get<Get...>, exclude<Exclude...>);
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Exclude>
    inline entt::basic_group<Entity, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) {
        return group<Owned...>(entt::get<>, exclude<Exclude...>);
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Exclude>
    inline entt::basic_group<Entity, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(exclude<Exclude...>);
    }

    /**
     * @brief Returns a runtime view for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Users should throw away the view after use. Fortunately, creating and
     * destroying a runtime view is an incredibly cheap operation because they
     * do not require any type of initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Runtime views are to be used when users want to construct a view from
     * some external inputs and don't know at compile-time what are the required
     * components.<br/>
     * This is particularly well suited to plugin systems and mods in general.
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of components.
     * @param last An iterator past the last element of the range of components.
     * @return A newly created runtime view.
     */
    template<typename It>
    entt::basic_runtime_view<Entity> runtime_view(It first, It last) const {
        static_assert(std::is_convertible_v<typename std::iterator_traits<It>::value_type, component_type>);
        std::vector<const sparse_set<Entity> *> set(std::distance(first, last));

        std::transform(first, last, set.begin(), [this](const component_type ctype) {
            auto it = std::find_if(pools.begin(), pools.end(), [ctype](const auto &pdata) {
                return pdata.pool && pdata.runtime_type == ctype;
            });

            return it != pools.cend() && it->pool ? it->pool.get() : nullptr;
        });

        return { std::move(set) };
    }

    /**
     * @brief Clones the given components and all the entity identifiers.
     *
     * The components must be copiable for obvious reasons. The entities
     * maintain their versions once copied.<br/>
     * If no components are provided, the registry will try to clone all the
     * existing pools.
     *
     * @note
     * There isn't an efficient way to know if all the entities are assigned at
     * least one component once copied. Therefore, there may be orphans. It is
     * up to the caller to clean up the registry if necessary.
     *
     * @note
     * Listeners and groups aren't copied. It is up to the caller to connect the
     * listeners of interest to the new registry and to set up groups.
     *
     * @warning
     * Attempting to clone components that aren't copyable results in unexpected
     * behaviors.<br/>
     * A static assertion will abort the compilation when the components
     * provided aren't copy constructible. Otherwise, an assertion will abort
     * the execution at runtime in debug mode in case one or more pools cannot
     * be cloned.
     *
     * @tparam Component Types of components to clone.
     * @return A fresh copy of the registry.
     */
    template<typename... Component>
    basic_registry clone() const {
        static_assert(std::conjunction_v<std::is_copy_constructible<Component>...>);
        basic_registry other;

        other.pools.resize(pools.size());

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool && (!sizeof...(Component) || ... || (pdata.runtime_type == type<Component>()))) {
                auto &curr = other.pools[pos-1];
                curr.clone = pdata.clone;
                curr.pool = curr.clone(*pdata.pool);
                curr.runtime_type = pdata.runtime_type;
                ENTT_ASSERT(sizeof...(Component) == 0 || curr.pool);
            }
        }

        other.skip_family_pools = skip_family_pools;
        other.entities = entities;
        other.available = available;
        other.next = next;

        other.pools.erase(std::remove_if(other.pools.begin()+skip_family_pools, other.pools.end(), [](const auto &pdata) {
            return !pdata.pool;
        }), other.pools.end());

        return other;
    }

    /**
     * @brief Returns a temporary object to use to create snapshots.
     *
     * A snapshot is either a full or a partial dump of a registry.<br/>
     * It can be used to save and restore its internal state or to keep two or
     * more instances of this class in sync, as an example in a client-server
     * architecture.
     *
     * @return A temporary object to use to take snasphosts.
     */
    entt::basic_snapshot<Entity> snapshot() const ENTT_NOEXCEPT {
        using follow_fn_type = entity_type(const basic_registry &, const entity_type);
        const entity_type seed = available ? (next | (entities[next] & (traits_type::version_mask << traits_type::entity_shift))) : next;

        follow_fn_type *follow = [](const basic_registry &reg, const entity_type entity) -> entity_type {
            const auto &others = reg.entities;
            const auto entt = entity & traits_type::entity_mask;
            const auto curr = others[entt] & traits_type::entity_mask;
            return (curr | (others[curr] & (traits_type::version_mask << traits_type::entity_shift)));
        };

        return { this, seed, follow };
    }

    /**
     * @brief Returns a temporary object to use to load snapshots.
     *
     * A snapshot is either a full or a partial dump of a registry.<br/>
     * It can be used to save and restore its internal state or to keep two or
     * more instances of this class in sync, as an example in a client-server
     * architecture.
     *
     * @note
     * The loader returned by this function requires that the registry be empty.
     * In case it isn't, all the data will be automatically deleted before to
     * return.
     *
     * @return A temporary object to use to load snasphosts.
     */
    basic_snapshot_loader<Entity> loader() ENTT_NOEXCEPT {
        using force_fn_type = void(basic_registry &, const entity_type, const bool);

        force_fn_type *force = [](basic_registry &reg, const entity_type entity, const bool destroyed) {
            using promotion_type = std::conditional_t<sizeof(size_type) >= sizeof(entity_type), size_type, entity_type>;
            // explicit promotion to avoid warnings with std::uint16_t
            const auto entt = promotion_type{entity} & traits_type::entity_mask;
            auto &others = reg.entities;

            if(!(entt < others.size())) {
                auto curr = others.size();
                others.resize(entt + 1);
                std::iota(others.data() + curr, others.data() + entt, entity_type(curr));
            }

            others[entt] = entity;

            if(destroyed) {
                reg.destroy(entity);
                const auto version = entity & (traits_type::version_mask << traits_type::entity_shift);
                others[entt] = ((others[entt] & traits_type::entity_mask) | version);
            }
        };

        reset();
        entities.clear();
        available = {};

        return { this, force };
    }

    /**
     * @brief Binds an object to the context of the registry.
     *
     * If the value already exists it is overwritten, otherwise a new instance
     * of the given type is created and initialized with the arguments provided.
     *
     * @tparam Type Type of object to set.
     * @tparam Args Types of arguments to use to construct the object.
     * @param args Parameters to use to initialize the value.
     * @return A reference to the newly created object.
     */
    template<typename Type, typename... Args>
    Type & set(Args &&... args) {
        const auto ctype = runtime_type<Type, context_family>();
        auto it = std::find_if(vars.begin(), vars.end(), [ctype](const auto &candidate) {
            return candidate.runtime_type == ctype;
        });

        if(it == vars.cend()) {
            vars.push_back({
                decltype(ctx_variable::value){new Type{std::forward<Args>(args)...}, +[](void *ptr) { delete static_cast<Type *>(ptr); }},
                ctype
            });

            it = std::prev(vars.end());
        } else {
            it->value.reset(new Type{std::forward<Args>(args)...});
        }

        return *static_cast<Type *>(it->value.get());
    }

    /**
     * @brief Unsets a context variable if it exists.
     * @tparam Type Type of object to set.
     */
    template<typename Type>
    void unset() {
        vars.erase(std::remove_if(vars.begin(), vars.end(), [](auto &var) {
            return var.runtime_type == runtime_type<Type, context_family>();
        }), vars.end());
    }

    /**
     * @brief Binds an object to the context of the registry.
     *
     * In case the context doesn't contain the given object, the parameters
     * provided are used to construct it.
     *
     * @tparam Type Type of object to set.
     * @tparam Args Types of arguments to use to construct the object.
     * @param args Parameters to use to initialize the object.
     * @return Reference to the object.
     */
    template<typename Type, typename... Args>
    Type & ctx_or_set(Args &&... args) {
        auto *type = try_ctx<Type>();
        return type ? *type : set<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns a pointer to an object in the context of the registry.
     * @tparam Type Type of object to get.
     * @return A pointer to the object if it exists in the context of the
     * registry, a null pointer otherwise.
     */
    template<typename Type>
    const Type * try_ctx() const ENTT_NOEXCEPT {
        const auto it = std::find_if(vars.begin(), vars.end(), [](const auto &var) {
            return var.runtime_type == runtime_type<Type, context_family>();
        });

        return (it == vars.cend()) ? nullptr : static_cast<const Type *>(it->value.get());
    }

    /*! @copydoc try_ctx */
    template<typename Type>
    inline Type * try_ctx() ENTT_NOEXCEPT {
        return const_cast<Type *>(std::as_const(*this).template try_ctx<Type>());
    }

    /**
     * @brief Returns a reference to an object in the context of the registry.
     *
     * @warning
     * Attempting to get a context variable that doesn't exist results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid requests.
     *
     * @tparam Type Type of object to get.
     * @return A valid reference to the object in the context of the registry.
     */
    template<typename Type>
    const Type & ctx() const ENTT_NOEXCEPT {
        const auto *instance = try_ctx<Type>();
        ENTT_ASSERT(instance);
        return *instance;
    }

    /*! @copydoc ctx */
    template<typename Type>
    inline Type & ctx() ENTT_NOEXCEPT {
        return const_cast<Type &>(std::as_const(*this).template ctx<Type>());
    }

private:
    std::size_t skip_family_pools{};
    std::vector<pool_data> pools;
    std::vector<group_data> groups;
    std::vector<ctx_variable> vars;
    std::vector<entity_type> entities;
    size_type available{};
    entity_type next{};
};


}


#endif // ENTT_ENTITY_REGISTRY_HPP

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Dedicated to those who aren't confident with entity-component systems.
 *
 * Tiny wrapper around a registry, for all those users that aren't confident
 * with entity-component systems and prefer to iterate objects directly.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct basic_actor {
    /*! @brief Type of registry used internally. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs an actor by using the given registry.
     * @param ref An entity-component system properly initialized.
     */
    basic_actor(registry_type &ref)
        : reg{&ref}, entt{ref.create()}
    {}

    /*! @brief Default destructor. */
    virtual ~basic_actor() {
        reg->destroy(entt);
    }

    /**
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_actor(basic_actor &&other)
        : reg{other.reg}, entt{other.entt}
    {
        other.entt = null;
    }

    /**
     * @brief Move assignment operator.
     *
     * After actor move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This actor.
     */
    basic_actor & operator=(basic_actor &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            std::swap(reg, tmp.reg);
            std::swap(entt, tmp.entt);
        }

        return *this;
    }

    /**
     * @brief Assigns the given component to an actor.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the actor.<br/>
     * In case the actor already has a component of the given type, it's
     * replaced with the new one.
     *
     * @tparam Component Type of the component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) assign(Args &&... args) {
        return reg->template assign_or_replace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from an actor.
     * @tparam Component Type of the component to remove.
     */
    template<typename Component>
    void remove() {
        reg->template remove<Component>(entt);
    }

    /**
     * @brief Checks if an actor has the given component.
     * @tparam Component Type of the component for which to perform the check.
     * @return True if the actor has the component, false otherwise.
     */
    template<typename Component>
    bool has() const ENTT_NOEXCEPT {
        return reg->template has<Component>(entt);
    }

    /**
     * @brief Returns references to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the actor.
     */
    template<typename... Component>
    decltype(auto) get() const ENTT_NOEXCEPT {
        return std::as_const(*reg).template get<Component...>(entt);
    }

    /*! @copydoc get */
    template<typename... Component>
    decltype(auto) get() ENTT_NOEXCEPT {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the actor.
     */
    template<typename... Component>
    auto try_get() const ENTT_NOEXCEPT {
        return std::as_const(*reg).template try_get<Component...>(entt);
    }

    /*! @copydoc try_get */
    template<typename... Component>
    auto try_get() ENTT_NOEXCEPT {
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    inline const registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /*! @copydoc backend */
    inline registry_type & backend() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(std::as_const(*this).backend());
    }

    /**
     * @brief Returns the entity associated with an actor.
     * @return The entity associated with the actor.
     */
    inline entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

private:
    registry_type *reg;
    Entity entt;
};


}


#endif // ENTT_ENTITY_ACTOR_HPP

// #include "entity/entity.hpp"

// #include "entity/group.hpp"

// #include "entity/helper.hpp"
#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP


#include <type_traits>
// #include "../config/config.h"

// #include "../core/hashed_string.hpp"

// #include "../signal/sigh.hpp"

// #include "registry.hpp"



namespace entt {


/**
 * @brief Converts a registry to a view.
 * @tparam Const Constness of the accepted registry.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<bool Const, typename Entity>
struct as_view {
    /*! @brief Type of registry to convert. */
    using registry_type = std::conditional_t<Const, const entt::basic_registry<Entity>, entt::basic_registry<Entity>>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_view(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a view.
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename... Component>
    inline operator entt::basic_view<Entity, Component...>() const {
        return reg.template view<Component...>();
    }

private:
    registry_type &reg;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the constness of a registry directly from the instance
 * provided to the constructor.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_view(basic_registry<Entity> &) ENTT_NOEXCEPT -> as_view<false, Entity>;


/*! @copydoc as_view */
template<typename Entity>
as_view(const basic_registry<Entity> &) ENTT_NOEXCEPT -> as_view<true, Entity>;


/**
 * @brief Converts a registry to a group.
 * @tparam Const Constness of the accepted registry.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<bool Const, typename Entity>
struct as_group {
    /*! @brief Type of registry to convert. */
    using registry_type = std::conditional_t<Const, const entt::basic_registry<Entity>, entt::basic_registry<Entity>>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_group(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a group.
     *
     * @note
     * Unfortunately, only full owning groups are supported because of an issue
     * with msvc that doesn't manage to correctly deduce types.
     *
     * @tparam Owned Types of components owned by the group.
     * @return A newly created group.
     */
    template<typename... Owned>
    inline operator entt::basic_group<Entity, get_t<>, Owned...>() const {
        return reg.template group<Owned...>();
    }

private:
    registry_type &reg;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the constness of a registry directly from the instance
 * provided to the constructor.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_group(basic_registry<Entity> &) ENTT_NOEXCEPT -> as_group<false, Entity>;


/*! @copydoc as_group */
template<typename Entity>
as_group(const basic_registry<Entity> &) ENTT_NOEXCEPT -> as_group<true, Entity>;


/**
 * @brief Dependency function prototype.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * This is a prototype function to use to create dependencies.<br/>
 * It isn't intended for direct use, although nothing forbids using it freely.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component that triggers the dependency handler.
 * @tparam Dependency Types of components to assign to an entity if triggered.
 * @param reg A valid reference to a registry.
 * @param entt A valid entity identifier.
 */
template<typename Entity, typename Component, typename... Dependency>
void dependency(basic_registry<Entity> &reg, const Entity entt, const Component &) {
    ((reg.template has<Dependency>(entt) ? void() : (reg.template assign<Dependency>(entt), void())), ...);
}


/**
 * @brief Connects a dependency function to the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following adds components `a_type` and `another_type` whenever `my_type`
 * is assigned to an entity:
 * @code{.cpp}
 * entt::registry registry;
 * entt::connect<a_type, another_type>(registry.construction<my_type>());
 * @endcode
 *
 * @tparam Dependency Types of components to assign to an entity if triggered.
 * @tparam Component Type of component that triggers the dependency handler.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Component, typename Entity>
inline void connect(sink<void(basic_registry<Entity> &, const Entity, Component &)> sink) {
    sink.template connect<dependency<Entity, Component, Dependency...>>();
}


/**
 * @brief Disconnects a dependency function from the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following breaks the dependency between the component `my_type` and the
 * components `a_type` and `another_type`:
 * @code{.cpp}
 * entt::registry registry;
 * entt::disconnect<a_type, another_type>(registry.construction<my_type>());
 * @endcode
 *
 * @tparam Dependency Types of components used to create the dependency.
 * @tparam Component Type of component that triggers the dependency handler.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Component, typename Entity>
inline void disconnect(sink<void(basic_registry<Entity> &, const Entity, Component &)> sink) {
    sink.template disconnect<dependency<Entity, Component, Dependency...>>();
}


/**
 * @brief Alias template to ease the assignment of tags to entities.
 *
 * If used in combination with hashed strings, it simplifies the assignment of
 * tags to entities and the use of tags in general where a type would be
 * required otherwise.<br/>
 * As an example and where the user defined literal for hashed strings hasn't
 * been changed:
 * @code{.cpp}
 * entt::registry registry;
 * registry.assign<entt::tag<"enemy"_hs>>(entity);
 * @endcode
 *
 * @note
 * Tags are empty components and therefore candidates for the empty component
 * optimization.
 *
 * @tparam Value The numeric representation of an instance of hashed string.
 */
template<typename hashed_string::hash_type Value>
using tag = std::integral_constant<typename hashed_string::hash_type, Value>;


}


#endif // ENTT_ENTITY_HELPER_HPP

// #include "entity/prototype.hpp"
#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP


#include <tuple>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <unordered_map>
// #include "../config/config.h"

// #include "registry.hpp"

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Prototype container for _concepts_.
 *
 * A prototype is used to define a _concept_ in terms of components.<br/>
 * Prototypes act as templates for those specific types of an application which
 * users would otherwise define through a series of component assignments to
 * entities. In other words, prototypes can be used to assign components to
 * entities of a registry at once.
 *
 * @note
 * Components used along with prototypes must be copy constructible. Prototypes
 * wrap component types with custom types, so they do not interfere with other
 * users of the registry they were built with.
 *
 * @warning
 * Prototypes directly use their underlying registries to store entities and
 * components for their purposes. Users must ensure that the lifetime of a
 * registry and its contents exceed that of the prototypes that use it.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_prototype {
    using basic_fn_type = void(const basic_prototype &, basic_registry<Entity> &, const Entity);
    using component_type = typename basic_registry<Entity>::component_type;

    template<typename Component>
    struct component_wrapper { Component component; };

    struct component_handler {
        basic_fn_type *assign_or_replace;
        basic_fn_type *assign;
    };

    void release() {
        if(reg->valid(entity)) {
            reg->destroy(entity);
        }
    }

public:
    /*! @brief Registry type. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Constructs a prototype that is bound to a given registry.
     * @param ref A valid reference to a registry.
     */
    basic_prototype(registry_type &ref)
        : reg{&ref},
          entity{ref.create()}
    {}

    /**
     * @brief Releases all its resources.
     */
    ~basic_prototype() {
        release();
    }

    /**
     * @brief Move constructor.
     *
     * After prototype move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_prototype(basic_prototype &&other)
        : handlers{std::move(other.handlers)},
          reg{other.reg},
          entity{other.entity}
    {
        other.entity = null;
    }

    /**
     * @brief Move assignment operator.
     *
     * After prototype move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This prototype.
     */
    basic_prototype & operator=(basic_prototype &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            handlers.swap(tmp.handlers);
            std::swap(reg, tmp.reg);
            std::swap(entity, tmp.entity);
        }

        return *this;
    }

    /**
     * @brief Assigns to or replaces the given component of a prototype.
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & set(Args &&... args) {
        component_handler handler;

        handler.assign_or_replace = [](const basic_prototype &proto, registry_type &other, const Entity dst) {
            const auto &wrapper = proto.reg->template get<component_wrapper<Component>>(proto.entity);
            other.template assign_or_replace<Component>(dst, wrapper.component);
        };

        handler.assign = [](const basic_prototype &proto, registry_type &other, const Entity dst) {
            if(!other.template has<Component>(dst)) {
                const auto &wrapper = proto.reg->template get<component_wrapper<Component>>(proto.entity);
                other.template assign<Component>(dst, wrapper.component);
            }
        };

        handlers[reg->template type<Component>()] = handler;
        auto &wrapper = reg->template assign_or_replace<component_wrapper<Component>>(entity, Component{std::forward<Args>(args)...});
        return wrapper.component;
    }

    /**
     * @brief Removes the given component from a prototype.
     * @tparam Component Type of component to remove.
     */
    template<typename Component>
    void unset() ENTT_NOEXCEPT {
        reg->template reset<component_wrapper<Component>>(entity);
        handlers.erase(reg->template type<Component>());
    }

    /**
     * @brief Checks if a prototype owns all the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the prototype owns all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const ENTT_NOEXCEPT {
        return reg->template has<component_wrapper<Component>...>(entity);
    }

    /**
     * @brief Returns references to the given components.
     *
     * @warning
     * Attempting to get a component from a prototype that doesn't own it
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own an instance of the given component.
     *
     * @tparam Component Types of components to get.
     * @return References to the components owned by the prototype.
     */
    template<typename... Component>
    decltype(auto) get() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (std::as_const(*reg).template get<component_wrapper<Component...>>(entity).component);
        } else {
            return std::tuple<std::add_const_t<Component> &...>{get<Component>()...};
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    inline decltype(auto) get() ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component &>(std::as_const(*this).template get<Component>()), ...);
        } else {
            return std::tuple<Component &...>{get<Component>()...};
        }
    }

    /**
     * @brief Returns pointers to the given components.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the prototype.
     */
    template<typename... Component>
    auto try_get() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            const auto *wrapper = reg->template try_get<component_wrapper<Component...>>(entity);
            return wrapper ? &wrapper->component : nullptr;
        } else {
            return std::tuple<std::add_const_t<Component> *...>{try_get<Component>()...};
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    inline auto try_get() ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component *>(std::as_const(*this).template try_get<Component>()), ...);
        } else {
            return std::tuple<Component *...>{try_get<Component>()...};
        }
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(registry, entity);
     * @endcode
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @param other A valid reference to a registry.
     * @return A valid entity identifier.
     */
    entity_type create(registry_type &other) const {
        const auto entt = other.create();
        assign(other, entt);
        return entt;
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(entity);
     * @endcode
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @return A valid entity identifier.
     */
    inline entity_type create() const {
        return create(*reg);
    }

    /**
     * @brief Assigns the components of a prototype to a given entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only those components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    void assign(registry_type &other, const entity_type dst) const {
        for(auto &handler: handlers) {
            handler.second.assign(*this, other, dst);
        }
    }

    /**
     * @brief Assigns the components of a prototype to a given entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only those components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void assign(const entity_type dst) const {
        assign(*reg, dst);
    }

    /**
     * @brief Assigns or replaces the components of a prototype for an entity.
     *
     * Existing components are overwritten, if any. All the other components
     * will be copied over to the target entity.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    void assign_or_replace(registry_type &other, const entity_type dst) const {
        for(auto &handler: handlers) {
            handler.second.assign_or_replace(*this, other, dst);
        }
    }

    /**
     * @brief Assigns or replaces the components of a prototype for an entity.
     *
     * Existing components are overwritten, if any. All the other components
     * will be copied over to the target entity.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void assign_or_replace(const entity_type dst) const {
        assign_or_replace(*reg, dst);
    }

    /**
     * @brief Assigns the components of a prototype to an entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only the components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    inline void operator()(registry_type &other, const entity_type dst) const ENTT_NOEXCEPT {
        assign(other, dst);
    }

    /**
     * @brief Assigns the components of a prototype to an entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only the components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void operator()(const entity_type dst) const ENTT_NOEXCEPT {
        assign(*reg, dst);
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(registry, entity);
     * @endcode
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @param other A valid reference to a registry.
     * @return A valid entity identifier.
     */
    inline entity_type operator()(registry_type &other) const ENTT_NOEXCEPT {
        return create(other);
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(entity);
     * @endcode
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @return A valid entity identifier.
     */
    inline entity_type operator()() const ENTT_NOEXCEPT {
        return create(*reg);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    inline const registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /*! @copydoc backend */
    inline registry_type & backend() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(std::as_const(*this).backend());
    }

private:
    std::unordered_map<component_type, component_handler> handlers;
    registry_type *reg;
    entity_type entity;
};


}


#endif // ENTT_ENTITY_PROTOTYPE_HPP

// #include "entity/registry.hpp"

// #include "entity/runtime_view.hpp"

// #include "entity/snapshot.hpp"

// #include "entity/sparse_set.hpp"

// #include "entity/storage.hpp"

// #include "entity/view.hpp"

// #include "locator/locator.hpp"
#ifndef ENTT_LOCATOR_LOCATOR_HPP
#define ENTT_LOCATOR_LOCATOR_HPP


#include <memory>
#include <utility>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @brief Service locator, nothing more.
 *
 * A service locator can be used to do what it promises: locate services.<br/>
 * Usually service locators are tightly bound to the services they expose and
 * thus it's hard to define a general purpose class to do that. This template
 * based implementation tries to fill the gap and to get rid of the burden of
 * defining a different specific locator for each application.
 *
 * @tparam Service Type of service managed by the locator.
 */
template<typename Service>
struct service_locator {
    /*! @brief Type of service offered. */
    using service_type = Service;

    /*! @brief Default constructor, deleted on purpose. */
    service_locator() = delete;
    /*! @brief Default destructor, deleted on purpose. */
    ~service_locator() = delete;

    /**
     * @brief Tests if a valid service implementation is set.
     * @return True if the service is set, false otherwise.
     */
    inline static bool empty() ENTT_NOEXCEPT {
        return !static_cast<bool>(service);
    }

    /**
     * @brief Returns a weak pointer to a service implementation, if any.
     *
     * Clients of a service shouldn't retain references to it. The recommended
     * way is to retrieve the service implementation currently set each and
     * every time the need of using it arises. Otherwise users can incur in
     * unexpected behaviors.
     *
     * @return A reference to the service implementation currently set, if any.
     */
    inline static std::weak_ptr<Service> get() ENTT_NOEXCEPT {
        return service;
    }

    /**
     * @brief Returns a weak reference to a service implementation, if any.
     *
     * Clients of a service shouldn't retain references to it. The recommended
     * way is to retrieve the service implementation currently set each and
     * every time the need of using it arises. Otherwise users can incur in
     * unexpected behaviors.
     *
     * @warning
     * In case no service implementation has been set, a call to this function
     * results in undefined behavior.
     *
     * @return A reference to the service implementation currently set, if any.
     */
    inline static Service & ref() ENTT_NOEXCEPT {
        return *service;
    }

    /**
     * @brief Sets or replaces a service.
     * @tparam Impl Type of the new service to use.
     * @tparam Args Types of arguments to use to construct the service.
     * @param args Parameters to use to construct the service.
     */
    template<typename Impl = Service, typename... Args>
    inline static void set(Args &&... args) {
        service = std::make_shared<Impl>(std::forward<Args>(args)...);
    }

    /**
     * @brief Sets or replaces a service.
     * @param ptr Service to use to replace the current one.
     */
    inline static void set(std::shared_ptr<Service> ptr) {
        ENTT_ASSERT(static_cast<bool>(ptr));
        service = std::move(ptr);
    }

    /**
     * @brief Resets a service.
     *
     * The service is no longer valid after a reset.
     */
    inline static void reset() {
        service.reset();
    }

private:
    inline static std::shared_ptr<Service> service = nullptr;
};


}


#endif // ENTT_LOCATOR_LOCATOR_HPP

// #include "meta/factory.hpp"
#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <utility>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H

// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP

// #include "meta.hpp"
#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <tuple>
#include <array>
#include <memory>
#include <cstring>
#include <cstddef>
#include <utility>
#include <functional>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/hashed_string.hpp"



namespace entt {


class meta_any;
class meta_handle;
class meta_prop;
class meta_base;
class meta_conv;
class meta_ctor;
class meta_dtor;
class meta_data;
class meta_func;
class meta_type;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_type_node;


struct meta_prop_node {
    meta_prop_node * next;
    meta_any(* const key)();
    meta_any(* const value)();
    meta_prop(* const meta)();
};


struct meta_base_node {
    meta_base_node ** const underlying;
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)();
    void *(* const cast)(void *);
    meta_base(* const meta)();
};


struct meta_conv_node {
    meta_conv_node ** const underlying;
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)();
    meta_any(* const conv)(void *);
    meta_conv(* const meta)();
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node ** const underlying;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type);
    meta_any(* const invoke)(meta_any * const);
    meta_ctor(* const meta)();
};


struct meta_dtor_node {
    meta_dtor_node ** const underlying;
    meta_type_node * const parent;
    bool(* const invoke)(meta_handle);
    meta_dtor(* const meta)();
};


struct meta_data_node {
    meta_data_node ** const underlying;
    hashed_string name;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const type)();
    bool(* const set)(meta_handle, meta_any, meta_any);
    meta_any(* const get)(meta_handle, meta_any);
    meta_data(* const meta)();
};


struct meta_func_node {
    using size_type = std::size_t;
    meta_func_node ** const underlying;
    hashed_string name;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)();
    meta_type_node *(* const arg)(size_type);
    meta_any(* const invoke)(meta_handle, meta_any *);
    meta_func(* const meta)();
};


struct meta_type_node {
    using size_type = std::size_t;
    hashed_string name;
    meta_type_node * next;
    meta_prop_node * prop;
    const bool is_void;
    const bool is_integral;
    const bool is_floating_point;
    const bool is_array;
    const bool is_enum;
    const bool is_union;
    const bool is_class;
    const bool is_pointer;
    const bool is_function;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const size_type extent;
    meta_type(* const remove_pointer)();
    bool(* const destroy)(meta_handle);
    meta_type(* const meta)();
    meta_base_node *base;
    meta_conv_node *conv;
    meta_ctor_node *ctor;
    meta_dtor_node *dtor;
    meta_data_node *data;
    meta_func_node *func;
};


template<typename...>
struct meta_node {
    inline static meta_type_node *type = nullptr;
};


template<typename Type>
struct meta_node<Type> {
    inline static meta_type_node *type = nullptr;

    template<typename>
    inline static meta_base_node *base = nullptr;

    template<typename>
    inline static meta_conv_node *conv = nullptr;

    template<typename>
    inline static meta_ctor_node *ctor = nullptr;

    template<auto>
    inline static meta_dtor_node *dtor = nullptr;

    template<auto...>
    inline static meta_data_node *data = nullptr;

    template<auto>
    inline static meta_func_node *func = nullptr;

    inline static meta_type_node * resolve() ENTT_NOEXCEPT;
};


template<typename... Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>...> {};


template<typename Op, typename Node>
void iterate(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr) {
        op(curr);
        curr = curr->next;
    }
}


template<auto Member, typename Op>
void iterate(Op op, const meta_type_node *node) ENTT_NOEXCEPT {
    if(node) {
        auto *curr = node->base;
        iterate(op, node->*Member);

        while(curr) {
            iterate<Member>(op, curr->type());
            curr = curr->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr && !op(curr)) {
        curr = curr->next;
    }

    return curr;
}


template<auto Member, typename Op>
auto find_if(Op op, const meta_type_node *node) ENTT_NOEXCEPT
-> decltype(find_if(op, node->*Member)) {
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *curr = node->base;

        while(curr && !ret) {
            ret = find_if<Member>(op, curr->type());
            curr = curr->next;
        }
    }

    return ret;
}


template<typename Type>
const Type * try_cast(const meta_type_node *node, void *instance) ENTT_NOEXCEPT {
    const auto *type = meta_info<Type>::resolve();
    void *ret = nullptr;

    if(node == type) {
        ret = instance;
    } else {
        const auto *base = find_if<&meta_type_node::base>([type](auto *candidate) {
            return candidate->type() == type;
        }, node);

        ret = base ? base->cast(instance) : nullptr;
    }

    return static_cast<const Type *>(ret);
}


template<auto Member>
inline bool can_cast_or_convert(const meta_type_node *from, const meta_type_node *to) ENTT_NOEXCEPT {
    return (from == to) || find_if<Member>([to](auto *node) {
        return node->type() == to;
    }, from);
}


template<typename... Args, std::size_t... Indexes>
inline auto ctor(std::index_sequence<Indexes...>, const meta_type_node *node) ENTT_NOEXCEPT {
    return internal::find_if([](auto *candidate) {
        return candidate->size == sizeof...(Args) &&
                (([](auto *from, auto *to) {
                    return internal::can_cast_or_convert<&internal::meta_type_node::base>(from, to)
                            || internal::can_cast_or_convert<&internal::meta_type_node::conv>(from, to);
                }(internal::meta_info<Args>::resolve(), candidate->arg(Indexes))) && ...);
    }, node->ctor);
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Meta any object.
 *
 * A meta any is an opaque container for single values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to
 * completely eliminate the need to allocate memory, where possible.<br/>
 * From the user's point of view, nothing will change, but the elimination of
 * allocations will reduce the jumps in memory and therefore will avoid chasing
 * of pointers. This will greatly improve the use of the cache, thus increasing
 * the overall performance.
 */
class meta_any {
    /*! @brief A meta handle is allowed to _inherit_ from a meta any. */
    friend class meta_handle;

    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using compare_fn_type = bool(*)(const void *, const void *);
    using copy_fn_type = void *(*)(storage_type &, const void *);
    using destroy_fn_type = void(*)(storage_type &);

    template<typename Type>
    inline static auto compare(int, const Type &lhs, const Type &rhs)
    -> decltype(lhs == rhs, bool{}) {
        return lhs == rhs;
    }

    template<typename Type>
    inline static bool compare(char, const Type &lhs, const Type &rhs) {
        return &lhs == &rhs;
    }

    template<typename Type>
    static bool compare(const void *lhs, const void *rhs) {
        return compare(0, *static_cast<const Type *>(lhs), *static_cast<const Type *>(rhs));
    }

    template<typename Type>
    static void * copy_storage(storage_type &storage, const void *instance) {
        return new (&storage) Type{*static_cast<const Type *>(instance)};
    }

    template<typename Type>
    static void * copy_object(storage_type &storage, const void *instance) {
        using chunk_type = std::aligned_storage_t<sizeof(Type), alignof(Type)>;
        auto *chunk = new chunk_type;
        new (&storage) chunk_type *{chunk};
        return new (chunk) Type{*static_cast<const Type *>(instance)};
    }

    template<typename Type>
    static void destroy_storage(storage_type &storage) {
        auto *node = internal::meta_info<Type>::resolve();
        auto *instance = reinterpret_cast<Type *>(&storage);
        node->dtor ? node->dtor->invoke(*instance) : node->destroy(*instance);
    }

    template<typename Type>
    static void destroy_object(storage_type &storage) {
        using chunk_type = std::aligned_storage_t<sizeof(Type), alignof(Type)>;
        auto *node = internal::meta_info<Type>::resolve();
        auto *chunk = *reinterpret_cast<chunk_type **>(&storage);
        auto *instance = reinterpret_cast<Type *>(chunk);
        node->dtor ? node->dtor->invoke(*instance) : node->destroy(*instance);
        delete chunk;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{nullptr},
          node{nullptr},
          destroy_fn{nullptr},
          compare_fn{nullptr},
          copy_fn{nullptr}
    {}

    /**
     * @brief Constructs a meta any from a given value.
     *
     * This class uses a technique called small buffer optimization (SBO) to
     * completely eliminate the need to allocate memory, where possible.<br/>
     * From the user's point of view, nothing will change, but the elimination
     * of allocations will reduce the jumps in memory and therefore will avoid
     * chasing of pointers. This will greatly improve the use of the cache, thus
     * increasing the overall performance.
     *
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&type) {
        using actual_type = std::remove_cv_t<std::remove_reference_t<Type>>;
        node = internal::meta_info<Type>::resolve();

        compare_fn = &compare<actual_type>;

        if constexpr(sizeof(actual_type) <= sizeof(void *)) {
            instance = new (&storage) actual_type{std::forward<Type>(type)};
            destroy_fn = &destroy_storage<actual_type>;
            copy_fn = &copy_storage<actual_type>;
        } else {
            using chunk_type = std::aligned_storage_t<sizeof(actual_type), alignof(actual_type)>;

            auto *chunk = new chunk_type;
            instance = new (chunk) actual_type{std::forward<Type>(type)};
            new (&storage) chunk_type *{chunk};

            destroy_fn = &destroy_object<actual_type>;
            copy_fn = &copy_object<actual_type>;
        }
    }

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other)
        : meta_any{}
    {
        if(other) {
            instance = other.copy_fn(storage, other.instance);
            node = other.node;
            destroy_fn = other.destroy_fn;
            compare_fn = other.compare_fn;
            copy_fn = other.copy_fn;
        }
    }

    /**
     * @brief Move constructor.
     *
     * After meta any move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other) ENTT_NOEXCEPT
        : meta_any{}
    {
        swap(*this, other);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy_fn) {
            destroy_fn(storage);
        }
    }

    /**
     * @brief Assignment operator.
     * @param other The instance to assign.
     * @return This meta any object.
     */
    meta_any & operator=(meta_any other) {
        swap(other, *this);
        return *this;
    }

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    inline const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    /*! @copydoc data */
    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Checks if it's possible to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return True if the cast is viable, false otherwise.
     */
    template<typename Type>
    inline bool can_cast() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();
        return internal::can_cast_or_convert<&internal::meta_type_node::base>(node, type);
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the cast is possible.
     *
     * @warning
     * Attempting to perform a cast that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the cast is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    inline const Type & cast() const ENTT_NOEXCEPT {
        ENTT_ASSERT(can_cast<Type>());
        return *internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc cast */
    template<typename Type>
    inline Type & cast() ENTT_NOEXCEPT {
        return const_cast<Type &>(std::as_const(*this).cast<Type>());
    }

    /**
     * @brief Checks if it's possible to convert an instance to a given type.
     * @tparam Type Type to which to convert the instance.
     * @return True if the conversion is viable, false otherwise.
     */
    template<typename Type>
    inline bool can_convert() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();
        return internal::can_cast_or_convert<&internal::meta_type_node::conv>(node, type);
    }

    /**
     * @brief Tries to convert an instance to a given type and returns it.
     * @tparam Type Type to which to convert the instance.
     * @return A valid meta any object if the conversion is possible, an invalid
     * one otherwise.
     */
    template<typename Type>
    inline meta_any convert() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();
        meta_any any{};

        if(node == type) {
            any = *static_cast<const Type *>(instance);
        } else {
            const auto *conv = internal::find_if<&internal::meta_type_node::conv>([type](auto *other) {
                return other->type() == type;
            }, node);

            if(conv) {
                any = conv->conv(instance);
            }
        }

        return any;
    }

    /**
     * @brief Tries to convert an instance to a given type.
     * @tparam Type Type to which to convert the instance.
     * @return True if the conversion is possible, false otherwise.
     */
    template<typename Type>
    inline bool convert() ENTT_NOEXCEPT {
        bool valid = (node == internal::meta_info<Type>::resolve());

        if(!valid) {
            auto any = std::as_const(*this).convert<Type>();

            if(any) {
                std::swap(any, *this);
                valid = true;
            }
        }

        return valid;
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return destroy_fn;
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    inline bool operator==(const meta_any &other) const ENTT_NOEXCEPT {
        return (!instance && !other.instance) || (instance && other.instance && node == other.node && compare_fn(instance, other.instance));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) {
        using std::swap;

        if(lhs && rhs) {
            storage_type buffer;
            void *tmp = lhs.copy_fn(buffer, lhs.instance);
            lhs.destroy_fn(lhs.storage);
            lhs.instance = rhs.copy_fn(lhs.storage, rhs.instance);
            rhs.destroy_fn(rhs.storage);
            rhs.instance = lhs.copy_fn(rhs.storage, tmp);
            lhs.destroy_fn(buffer);
        } else if(lhs) {
            rhs.instance = lhs.copy_fn(rhs.storage, lhs.instance);
            lhs.destroy_fn(lhs.storage);
            lhs.instance = nullptr;
        } else if(rhs) {
            lhs.instance = rhs.copy_fn(lhs.storage, rhs.instance);
            rhs.destroy_fn(rhs.storage);
            rhs.instance = nullptr;
        }

        std::swap(lhs.node, rhs.node);
        std::swap(lhs.destroy_fn, rhs.destroy_fn);
        std::swap(lhs.compare_fn, rhs.compare_fn);
        std::swap(lhs.copy_fn, rhs.copy_fn);
    }

private:
    storage_type storage;
    void *instance;
    internal::meta_type_node *node;
    destroy_fn_type destroy_fn;
    compare_fn_type compare_fn;
    copy_fn_type copy_fn;
};


/**
 * @brief Meta handle object.
 *
 * A meta handle is an opaque pointer to an instance of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance. Users are
 * responsible for ensuring that the target object remains alive for the entire
 * interval of use of the handle.
 */
class meta_handle {
    meta_handle(int, meta_any &any) ENTT_NOEXCEPT
        : node{any.node},
          instance{any.instance}
    {}

    template<typename Type>
    meta_handle(char, Type &&obj) ENTT_NOEXCEPT
        : node{internal::meta_info<Type>::resolve()},
          instance{&obj}
    {}

public:
    /*! @brief Default constructor. */
    meta_handle() ENTT_NOEXCEPT
        : node{nullptr},
          instance{nullptr}
    {}

    /**
     * @brief Constructs a meta handle from a given instance.
     * @tparam Type Type of object to use to initialize the handle.
     * @param obj A reference to an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_handle>>>
    meta_handle(Type &&obj) ENTT_NOEXCEPT
        : meta_handle{0, std::forward<Type>(obj)}
    {}

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the conversion is possible.
     *
     * @warning
     * Attempting to perform a conversion that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the conversion is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A pointer to the contained instance.
     */
    template<typename Type>
    inline const Type * try_cast() const ENTT_NOEXCEPT {
        return internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc try_cast */
    template<typename Type>
    inline Type * try_cast() ENTT_NOEXCEPT {
        return const_cast<Type *>(std::as_const(*this).try_cast<Type>());
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    inline const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    /*! @copydoc data */
    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Returns false if a handle is empty, true otherwise.
     * @return False if the handle is empty, true otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return instance;
    }

private:
    const internal::meta_type_node *node;
    void *instance;
};


/**
 * @brief Checks if two containers differ in their content.
 * @param lhs A meta any object, either empty or not.
 * @param rhs A meta any object, either empty or not.
 * @return True if the two containers differ in their content, false otherwise.
 */
inline bool operator!=(const meta_any &lhs, const meta_any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta property object.
 *
 * A meta property is an opaque container for a key/value pair.<br/>
 * Properties are associated with any other meta object to enrich it.
 */
class meta_prop {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_prop(const internal::meta_prop_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    inline meta_prop() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    inline meta_any key() const ENTT_NOEXCEPT {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    inline meta_any value() const ENTT_NOEXCEPT {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_prop &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_prop_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_prop &lhs, const meta_prop &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta base object.
 *
 * A meta base is an opaque container for a base class to be used to walk
 * through hierarchies.
 */
class meta_base {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_base(const internal::meta_base_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    inline meta_base() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta base belongs.
     * @return The meta type to which the meta base belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of a given meta base.
     * @return The meta type of the meta base.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Casts an instance from a parent type to a base type.
     * @param instance The instance to cast.
     * @return An opaque pointer to the base type.
     */
    inline void * cast(void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_base &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_base_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_base &lhs, const meta_base &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta conversion function object.
 *
 * A meta conversion function is an opaque container for a conversion function
 * to be used to convert a given instance to another type.
 */
class meta_conv {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_conv(const internal::meta_conv_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    inline meta_conv() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta conversion function belongs.
     * @return The meta type to which the meta conversion function belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of a given meta conversion function.
     * @return The meta type of the meta conversion function.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Converts an instance to a given type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    inline meta_any convert(void *instance) const ENTT_NOEXCEPT {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_conv &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_conv_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_conv &lhs, const meta_conv &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta constructor object.
 *
 * A meta constructor is an opaque container for a function to be used to
 * construct instances of a given type.
 */
class meta_ctor {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_ctor(const internal::meta_ctor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /*! @brief Default constructor. */
    inline meta_ctor() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta constructor belongs.
     * @return The meta type to which the meta constructor belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta constructor.
     * @return The number of arguments accepted by the meta constructor.
     */
    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta constructor.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta constructor, if any.
     */
    inline meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        if(sizeof...(Args) == size()) {
            any = node->invoke(arguments.data());
        }

        return any;
    }

    /**
     * @brief Iterates all the properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_ctor &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_ctor_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_ctor &lhs, const meta_ctor &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta destructor object.
 *
 * A meta destructor is an opaque container for a function to be used to
 * destroy instances of a given type.
 */
class meta_dtor {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_dtor(const internal::meta_dtor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    inline meta_dtor() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta destructor belongs.
     * @return The meta type to which the meta destructor belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * destructor. Otherwise, invoking the meta destructor results in an
     * undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return True in case of success, false otherwise.
     */
    inline bool invoke(meta_handle handle) const {
        return node->invoke(handle);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_dtor &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_dtor_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_dtor &lhs, const meta_dtor &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta data object.
 *
 * A meta data is an opaque container for a data member associated with a given
 * type.
 */
class meta_data {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_data(const internal::meta_data_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    inline meta_data() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the name assigned to a given meta data.
     * @return The name assigned to the meta data.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Returns the meta type to which a meta data belongs.
     * @return The meta type to which the meta data belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Indicates whether a given meta data is constant or not.
     * @return True if the meta data is constant, false otherwise.
     */
    inline bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta data is static or not.
     *
     * A static meta data is such that it can be accessed using a null pointer
     * as an instance.
     *
     * @return True if the meta data is static, false otherwise.
     */
    inline bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type of a given meta data.
     * @return The meta type of the meta data.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must coincide exactly with that of the variable
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    inline bool set(meta_handle handle, Type &&value) const {
        return node->set(handle, meta_any{}, std::forward<Type>(value));
    }

    /**
     * @brief Sets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must coincide exactly with that of the array type
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param index Position of the underlying element to set.
     * @param value Parameter to use to set the underlying element.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    inline bool set(meta_handle handle, std::size_t index, Type &&value) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->set(handle, index, std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    inline meta_any get(meta_handle handle) const ENTT_NOEXCEPT {
        return node->get(handle, meta_any{});
    }

    /**
     * @brief Gets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param index Position of the underlying element to get.
     * @return A meta any containing the value of the underlying element.
     */
    inline meta_any get(meta_handle handle, std::size_t index) const ENTT_NOEXCEPT {
        ENTT_ASSERT(index < node->type()->extent);
        return node->get(handle, index);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_data &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_data_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_data &lhs, const meta_data &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta function object.
 *
 * A meta function is an opaque container for a member function associated with
 * a given type.
 */
class meta_func {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    inline meta_func(const internal::meta_func_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @brief Default constructor. */
    inline meta_func() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the name assigned to a given meta function.
     * @return The name assigned to the meta function.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Returns the meta type to which a meta function belongs.
     * @return The meta type to which the meta function belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta function.
     * @return The number of arguments accepted by the meta function.
     */
    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Indicates whether a given meta function is constant or not.
     * @return True if the meta function is constant, false otherwise.
     */
    inline bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta function is static or not.
     *
     * A static meta function is such that it can be invoked using a null
     * pointer as an instance.
     *
     * @return True if the meta function is static, false otherwise.
     */
    inline bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type of the return type of a meta function.
     * @return The meta type of the return type of the meta function.
     */
    inline meta_type ret() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of the i-th argument of a meta function.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta function, if any.
     */
    inline meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a meta function, the types of the parameters must coincide
     * exactly with those required by the underlying function. Otherwise, an
     * empty and then invalid container is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle handle, Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        if(sizeof...(Args) == size()) {
            any = node->invoke(handle, arguments.data());
        }

        return any;
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_func &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_func_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_func &lhs, const meta_func &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta type object.
 *
 * A meta type is the starting point for accessing a reflected type, thus being
 * able to work through it on real objects.
 */
class meta_type {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    /*! @brief A meta node is allowed to create meta objects. */
    template<typename...> friend struct internal::meta_node;

    inline meta_type(const internal::meta_type_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @brief Default constructor. */
    inline meta_type() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the name assigned to a given meta type.
     * @return The name assigned to the meta type.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Indicates whether a given meta type refers to void or not.
     * @return True if the underlying type is void, false otherwise.
     */
    inline bool is_void() const ENTT_NOEXCEPT {
        return node->is_void;
    }

    /**
     * @brief Indicates whether a given meta type refers to an integral type or
     * not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    inline bool is_integral() const ENTT_NOEXCEPT {
        return node->is_integral;
    }

    /**
     * @brief Indicates whether a given meta type refers to a floating-point
     * type or not.
     * @return True if the underlying type is a floating-point type, false
     * otherwise.
     */
    inline bool is_floating_point() const ENTT_NOEXCEPT {
        return node->is_floating_point;
    }

    /**
     * @brief Indicates whether a given meta type refers to an array type or
     * not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    inline bool is_array() const ENTT_NOEXCEPT {
        return node->is_array;
    }

    /**
     * @brief Indicates whether a given meta type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    inline bool is_enum() const ENTT_NOEXCEPT {
        return node->is_enum;
    }

    /**
     * @brief Indicates whether a given meta type refers to an union or not.
     * @return True if the underlying type is an union, false otherwise.
     */
    inline bool is_union() const ENTT_NOEXCEPT {
        return node->is_union;
    }

    /**
     * @brief Indicates whether a given meta type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    inline bool is_class() const ENTT_NOEXCEPT {
        return node->is_class;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    inline bool is_pointer() const ENTT_NOEXCEPT {
        return node->is_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a function type or
     * not.
     * @return True if the underlying type is a function, false otherwise.
     */
    inline bool is_function() const ENTT_NOEXCEPT {
        return node->is_function;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to data
     * member or not.
     * @return True if the underlying type is a pointer to data member, false
     * otherwise.
     */
    inline bool is_member_object_pointer() const ENTT_NOEXCEPT {
        return node->is_member_object_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to member
     * function or not.
     * @return True if the underlying type is a pointer to member function,
     * false otherwise.
     */
    inline bool is_member_function_pointer() const ENTT_NOEXCEPT {
        return node->is_member_function_pointer;
    }

    /**
     * @brief If a given meta type refers to an array type, provides the number
     * of elements of the array.
     * @return The number of elements of the array if the underlying type is an
     * array type, 0 otherwise.
     */
    inline size_type extent() const ENTT_NOEXCEPT {
        return node->extent;
    }

    /**
     * @brief Provides the meta type for which the pointer is defined.
     * @return The meta type for which the pointer is defined or this meta type
     * if it doesn't refer to a pointer type.
     */
    inline meta_type remove_pointer() const ENTT_NOEXCEPT {
        return node->remove_pointer();
    }

    /**
     * @brief Iterates all the meta base of a meta type.
     *
     * Iteratively returns **all** the base classes of the given type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void base(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::base>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta base associated with a given name.
     *
     * Searches recursively among **all** the base classes of the given type.
     *
     * @param str The name to use to search for a meta base.
     * @return The meta base associated with the given name, if any.
     */
    inline meta_base base(const char *str) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::base>([name = hashed_string{str}](auto *candidate) {
            return candidate->type()->name == name;
        }, node);

        return curr ? curr->meta() : meta_base{};
    }

    /**
     * @brief Iterates all the meta conversion functions of a meta type.
     *
     * Iteratively returns **all** the meta conversion functions of the given
     * type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void conv(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::conv>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta conversion function associated with a given type.
     *
     * Searches recursively among **all** the conversion functions of the given
     * type.
     *
     * @tparam Type The type to use to search for a meta conversion function.
     * @return The meta conversion function associated with the given type, if
     * any.
     */
    template<typename Type>
    inline meta_conv conv() const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::conv>([type = internal::meta_info<Type>::resolve()](auto *candidate) {
            return candidate->type() == type;
        }, node);

        return curr ? curr->meta() : meta_conv{};
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void ctor(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->ctor);
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    inline meta_ctor ctor() const ENTT_NOEXCEPT {
        const auto *curr = internal::ctor<Args...>(std::make_index_sequence<sizeof...(Args)>{}, node);
        return curr ? curr->meta() : meta_ctor{};
    }

    /**
     * @brief Returns the meta destructor associated with a given type.
     * @return The meta destructor associated with the given type, if any.
     */
    inline meta_dtor dtor() const ENTT_NOEXCEPT {
        return node->dtor ? node->dtor->meta() : meta_dtor{};
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     *
     * Iteratively returns **all** the meta data of the given type. This means
     * that the meta data of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void data(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::data>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta data associated with a given name.
     *
     * Searches recursively among **all** the meta data of the given type. This
     * means that the meta data of the base classes will also be inspected, if
     * any.
     *
     * @param str The name to use to search for a meta data.
     * @return The meta data associated with the given name, if any.
     */
    inline meta_data data(const char *str) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::data>([name = hashed_string{str}](auto *candidate) {
            return candidate->name == name;
        }, node);

        return curr ? curr->meta() : meta_data{};
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     *
     * Iteratively returns **all** the meta functions of the given type. This
     * means that the meta functions of the base classes will also be returned,
     * if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void func(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::func>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta function associated with a given name.
     *
     * Searches recursively among **all** the meta functions of the given type.
     * This means that the meta functions of the base classes will also be
     * inspected, if any.
     *
     * @param str The name to use to search for a meta function.
     * @return The meta function associated with the given name, if any.
     */
    inline meta_func func(const char *str) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::func>([name = hashed_string{str}](auto *candidate) {
            return candidate->name == name;
        }, node);

        return curr ? curr->meta() : meta_func{};
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any construct(Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        internal::iterate<&internal::meta_type_node::ctor>([data = arguments.data(), &any](auto *curr) -> bool {
            any = curr->invoke(data);
            return static_cast<bool>(any);
        }, node);

        return any;
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the underlying type.
     * Otherwise, invoking the meta destructor results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return True in case of success, false otherwise.
     */
    inline bool destroy(meta_handle handle) const {
        return node->dtor ? node->dtor->invoke(handle) : node->destroy(handle);
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     *
     * Iteratively returns **all** the properties of the given type. This means
     * that the properties of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::prop>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * Searches recursively among **all** the properties of the given type. This
     * means that the properties of the base classes will also be inspected, if
     * any.
     *
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::prop>([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    inline bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_type_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_type &lhs, const meta_type &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


inline meta_type meta_any::type() const ENTT_NOEXCEPT {
    return node ? node->meta() : meta_type{};
}


inline meta_type meta_handle::type() const ENTT_NOEXCEPT {
    return node ? node->meta() : meta_type{};
}


inline meta_type meta_base::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_base::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_conv::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_conv::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index)->meta() : meta_type{};
}


inline meta_type meta_dtor::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_data::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_data::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_func::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_func::ret() const ENTT_NOEXCEPT {
    return node->ret()->meta();
}


inline meta_type meta_func::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index)->meta() : meta_type{};
}


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename...>
struct meta_function_helper;


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...)> {
    using return_type = Ret;
    using args_type = std::tuple<Args...>;

    template<std::size_t Index>
    using arg_type = std::decay_t<std::tuple_element_t<Index, args_type>>;

    static constexpr auto size = sizeof...(Args);

    inline static auto arg(typename internal::meta_func_node::size_type index) {
        return std::array<meta_type_node *, sizeof...(Args)>{{meta_info<Args>::resolve()...}}[index];
    }
};


template<typename Class, typename Ret, typename... Args, bool Const, bool Static>
struct meta_function_helper<Class, Ret(Args...), std::bool_constant<Const>, std::bool_constant<Static>>: meta_function_helper<Ret(Args...)> {
    using class_type = Class;
    static constexpr auto is_const = Const;
    static constexpr auto is_static = Static;
};


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Class, Ret(Args...), std::bool_constant<false>, std::bool_constant<false>>
to_meta_function_helper(Ret(Class:: *)(Args...));


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Class, Ret(Args...), std::bool_constant<true>, std::bool_constant<false>>
to_meta_function_helper(Ret(Class:: *)(Args...) const);


template<typename Ret, typename... Args>
constexpr meta_function_helper<void, Ret(Args...), std::bool_constant<false>, std::bool_constant<true>>
to_meta_function_helper(Ret(*)(Args...));


template<auto Func>
struct meta_function_helper<std::integral_constant<decltype(Func), Func>>: decltype(to_meta_function_helper(Func)) {};


template<typename Type>
inline bool destroy([[maybe_unused]] meta_handle handle) {
    bool accepted = false;

    if constexpr(std::is_object_v<Type> && !std::is_array_v<Type>) {
        accepted = (handle.type() == meta_info<Type>::resolve()->meta());

        if(accepted) {
            static_cast<Type *>(handle.data())->~Type();
        }
    }

    return accepted;
}


template<typename Type, typename... Args, std::size_t... Indexes>
inline meta_any construct(meta_any * const args, std::index_sequence<Indexes...>) {
    [[maybe_unused]] std::array<bool, sizeof...(Args)> can_cast{{(args+Indexes)->can_cast<std::remove_cv_t<std::remove_reference_t<Args>>>()...}};
    [[maybe_unused]] std::array<bool, sizeof...(Args)> can_convert{{(std::get<Indexes>(can_cast) ? false : (args+Indexes)->can_convert<std::remove_cv_t<std::remove_reference_t<Args>>>())...}};
    meta_any any{};

    if(((std::get<Indexes>(can_cast) || std::get<Indexes>(can_convert)) && ...)) {
        ((std::get<Indexes>(can_convert) ? void((args+Indexes)->convert<std::remove_cv_t<std::remove_reference_t<Args>>>()) : void()), ...);
        any = Type{(args+Indexes)->cast<std::remove_cv_t<std::remove_reference_t<Args>>>()...};
    }

    return any;
}


template<bool Const, typename Type, auto Data>
bool setter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(!Const) {
        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using helper_type = meta_function_helper<std::integral_constant<decltype(Data), Data>>;
            using data_type = std::decay_t<std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>>;
            static_assert(std::is_invocable_v<decltype(Data), Type *, data_type>);
            accepted = value.can_cast<data_type>() || value.convert<data_type>();
            auto *clazz = handle.try_cast<Type>();

            if(accepted && clazz) {
                std::invoke(Data, clazz, value.cast<data_type>());
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
            static_assert(std::is_invocable_v<decltype(Data), Type>);
            auto *clazz = handle.try_cast<Type>();

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                accepted = index.can_cast<std::size_t>() && (value.can_cast<underlying_type>() || value.convert<underlying_type>());

                if(accepted && clazz) {
                    std::invoke(Data, clazz)[index.cast<std::size_t>()] = value.cast<underlying_type>();
                }
            } else {
                accepted = value.can_cast<data_type>() || value.convert<data_type>();

                if(accepted && clazz) {
                    std::invoke(Data, clazz) = value.cast<data_type>();
                }
            }
        } else {
            static_assert(std::is_pointer_v<decltype(Data)>);
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(*Data)>>;

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                accepted = index.can_cast<std::size_t>() && (value.can_cast<underlying_type>() || value.convert<underlying_type>());

                if(accepted) {
                    (*Data)[index.cast<std::size_t>()] = value.cast<underlying_type>();
                }
            } else {
                accepted = value.can_cast<data_type>() || value.convert<data_type>();

                if(accepted) {
                    *Data = value.cast<data_type>();
                }
            }
        }
    }

    return accepted;
}


template<typename Type, auto Data>
inline meta_any getter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index) {
    if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
       static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto *clazz = handle.try_cast<Type>();
        return clazz ? std::invoke(Data, clazz) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
        static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto *clazz = handle.try_cast<Type>();

        if constexpr(std::is_array_v<data_type>) {
            return (clazz && index.can_cast<std::size_t>()) ? std::invoke(Data, clazz)[index.cast<std::size_t>()] : meta_any{};
        } else {
            return clazz ? std::invoke(Data, clazz) : meta_any{};
        }
    } else {
        static_assert(std::is_pointer_v<decltype(Data)>);

        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            return index.can_cast<std::size_t>() ? (*Data)[index.cast<std::size_t>()] : meta_any{};
        } else {
            return *Data;
        }
    }
}


template<typename Type, auto Func, std::size_t... Indexes>
std::enable_if_t<std::is_function_v<std::remove_pointer_t<decltype(Func)>>, meta_any>
invoke(const meta_handle &, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper<std::integral_constant<decltype(Func), Func>>;
    meta_any any{};

    if((((args+Indexes)->can_cast<typename helper_type::template arg_type<Indexes>>()
            || (args+Indexes)->convert<typename helper_type::template arg_type<Indexes>>()) && ...))
    {
        if constexpr(std::is_void_v<typename helper_type::return_type>) {
            std::invoke(Func, (args+Indexes)->cast<typename helper_type::template arg_type<Indexes>>()...);
        } else {
            any = meta_any{std::invoke(Func, (args+Indexes)->cast<typename helper_type::template arg_type<Indexes>>()...)};
        }
    }

    return any;
}


template<typename Type, auto Member, std::size_t... Indexes>
std::enable_if_t<std::is_member_function_pointer_v<decltype(Member)>, meta_any>
invoke(meta_handle &handle, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper<std::integral_constant<decltype(Member), Member>>;
    static_assert(std::is_base_of_v<typename helper_type::class_type, Type>);
    auto *clazz = handle.try_cast<Type>();
    meta_any any{};

    if(clazz && (((args+Indexes)->can_cast<typename helper_type::template arg_type<Indexes>>()
                  || (args+Indexes)->convert<typename helper_type::template arg_type<Indexes>>()) && ...))
    {
        if constexpr(std::is_void_v<typename helper_type::return_type>) {
            std::invoke(Member, clazz, (args+Indexes)->cast<typename helper_type::template arg_type<Indexes>>()...);
        } else {
            any = meta_any{std::invoke(Member, clazz, (args+Indexes)->cast<typename helper_type::template arg_type<Indexes>>()...)};
        }
    }

    return any;
}


template<typename Type>
meta_type_node * meta_node<Type>::resolve() ENTT_NOEXCEPT {
    if(!type) {
        static meta_type_node node{
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_array_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_function_v<Type>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            std::extent_v<Type>,
            []() -> meta_type {
                return internal::meta_info<std::remove_pointer_t<Type>>::resolve();
            },
            &destroy<Type>,
            []() -> meta_type {
                return &node;
            }
        };

        type = &node;
    }

    return type;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


}


#endif // ENTT_META_META_HPP



namespace entt {


template<typename>
class meta_factory;


template<typename Type, typename... Property>
meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;


template<typename Type>
bool unregister() ENTT_NOEXCEPT;


/**
 * @brief A meta factory to be used for reflection purposes.
 *
 * A meta factory is an utility class used to reflect types, data and functions
 * of all sorts. This class ensures that the underlying web of types is built
 * correctly and performs some checks in debug mode to ensure that there are no
 * subtle errors at runtime.
 *
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory {
    static_assert(std::is_object_v<Type> && !(std::is_const_v<Type> || std::is_volatile_v<Type>));

    template<typename Node>
    inline bool duplicate(const hashed_string &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline bool duplicate(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename>
    internal::meta_prop_node * properties() {
        return nullptr;
    }

    template<typename Owner, typename Property, typename... Other>
    internal::meta_prop_node * properties(Property &&property, Other &&... other) {
        static std::remove_cv_t<std::remove_reference_t<Property>> prop{};

        static internal::meta_prop_node node{
            nullptr,
            []() -> meta_any {
                return std::get<0>(prop);
            },
            []() -> meta_any {
                return std::get<1>(prop);
            },
            []() -> meta_prop {
                return &node;
            }
        };

        prop = std::forward<Property>(property);
        node.next = properties<Owner>(std::forward<Other>(other)...);
        ENTT_ASSERT(!duplicate(meta_any{std::get<0>(prop)}, node.next));
        return &node;
    }

    template<typename... Property>
    meta_factory type(hashed_string name, Property &&... property) ENTT_NOEXCEPT {
        static internal::meta_type_node node{
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_array_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_function_v<Type>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            std::extent_v<Type>,
            []() -> meta_type {
                return internal::meta_info<std::remove_pointer_t<Type>>::resolve();
            },
            &internal::destroy<Type>,
            []() -> meta_type {
                return &node;
            }
        };

        node.name = name;
        node.next = internal::meta_info<>::type;
        node.prop = properties<Type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(name, node.next));
        ENTT_ASSERT(!internal::meta_info<Type>::type);
        internal::meta_info<Type>::type = &node;
        internal::meta_info<>::type = &node;

        return *this;
    }

    void unregister_prop(internal::meta_prop_node **prop) {
        while(*prop) {
            auto *node = *prop;
            *prop = node->next;
            node->next = nullptr;
        }
    }

    void unregister_dtor() {
        if(auto node = internal::meta_info<Type>::type->dtor; node) {
            internal::meta_info<Type>::type->dtor = nullptr;
            *node->underlying = nullptr;
        }
    }

    template<auto Member>
    auto unregister_all(int)
    -> decltype((internal::meta_info<Type>::type->*Member)->prop, void()) {
        while(internal::meta_info<Type>::type->*Member) {
            auto node = internal::meta_info<Type>::type->*Member;
            internal::meta_info<Type>::type->*Member = node->next;
            unregister_prop(&node->prop);
            node->next = nullptr;
            *node->underlying = nullptr;
        }
    }

    template<auto Member>
    void unregister_all(char) {
        while(internal::meta_info<Type>::type->*Member) {
            auto node = internal::meta_info<Type>::type->*Member;
            internal::meta_info<Type>::type->*Member = node->next;
            node->next = nullptr;
            *node->underlying = nullptr;
        }
    }

    bool unregister() ENTT_NOEXCEPT {
        const auto registered = internal::meta_info<Type>::type;

        if(registered) {
            if(auto *curr = internal::meta_info<>::type; curr == internal::meta_info<Type>::type) {
                internal::meta_info<>::type = internal::meta_info<Type>::type->next;
            } else {
                while(curr && curr->next != internal::meta_info<Type>::type) {
                    curr = curr->next;
                }

                if(curr) {
                    curr->next = internal::meta_info<Type>::type->next;
                }
            }

            unregister_prop(&internal::meta_info<Type>::type->prop);
            unregister_all<&internal::meta_type_node::base>(0);
            unregister_all<&internal::meta_type_node::conv>(0);
            unregister_all<&internal::meta_type_node::ctor>(0);
            unregister_all<&internal::meta_type_node::data>(0);
            unregister_all<&internal::meta_type_node::func>(0);
            unregister_dtor();

            internal::meta_info<Type>::type->name = {};
            internal::meta_info<Type>::type->next = nullptr;
            internal::meta_info<Type>::type = nullptr;
        }

        return registered;
    }

    meta_factory() ENTT_NOEXCEPT = default;

public:
    template<typename Other, typename... Property>
    friend meta_factory<Other> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;

    template<typename Other>
    friend bool unregister() ENTT_NOEXCEPT;

    /**
     * @brief Assigns a meta base to a meta type.
     *
     * A reflected base class must be a real base class of the reflected type.
     *
     * @tparam Base Type of the base class to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename Base>
    meta_factory base() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<Base, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            &internal::meta_info<Type>::template base<Base>,
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](void *instance) -> void * {
                return static_cast<Base *>(static_cast<Type *>(instance));
            },
            []() -> meta_base {
                return &node;
            }
        };

        node.next = type->base;
        ENTT_ASSERT((!internal::meta_info<Type>::template base<Base>));
        internal::meta_info<Type>::template base<Base> = &node;
        type->base = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * The given type must be such that an instance of the reflected type can be
     * converted to it.
     *
     * @tparam To Type of the conversion function to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename To>
    meta_factory conv() ENTT_NOEXCEPT {
        static_assert(std::is_convertible_v<Type, To>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            &internal::meta_info<Type>::template conv<To>,
            type,
            nullptr,
            &internal::meta_info<To>::resolve,
            [](void *instance) -> meta_any {
                return static_cast<To>(*static_cast<Type *>(instance));
            },
            []() -> meta_conv {
                return &node;
            }
        };

        node.next = type->conv;
        ENTT_ASSERT((!internal::meta_info<Type>::template conv<To>));
        internal::meta_info<Type>::template conv<To> = &node;
        type->conv = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of
     * constructors. All that is required is that they return an instance of the
     * underlying type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or a free function.
     *
     * @tparam Func The actual function to use as a constructor.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;
        static_assert(std::is_same_v<typename helper_type::return_type, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            &internal::meta_info<Type>::template ctor<typename helper_type::args_type>,
            type,
            nullptr,
            nullptr,
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func>(nullptr, any, std::make_index_sequence<helper_type::size>{});
            },
            []() -> meta_ctor {
                return &node;
            }
        };

        node.next = type->ctor;
        node.prop = properties<typename helper_type::args_type>(std::forward<Property>(property)...);
        ENTT_ASSERT((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * A meta constructor is uniquely identified by the types of its arguments
     * and is such that there exists an actual constructor of the underlying
     * type that can be invoked with parameters whose types are those given.
     *
     * @tparam Args Types of arguments to use to construct an instance.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<typename... Args, typename... Property>
    meta_factory ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<Type(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            &internal::meta_info<Type>::template ctor<typename helper_type::args_type>,
            type,
            nullptr,
            nullptr,
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, Args...>(any, std::make_index_sequence<helper_type::size>{});
            },
            []() -> meta_ctor {
                return &node;
            }
        };

        node.next = type->ctor;
        node.prop = properties<typename helper_type::args_type>(std::forward<Property>(property)...);
        ENTT_ASSERT((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of destructors.
     * The signature of the function should identical to the following:
     *
     * @code{.cpp}
     * void(Type *);
     * @endcode
     *
     * From a client's point of view, nothing changes if the destructor of a
     * meta type is the default one or a custom one.
     *
     * @tparam Func The actual function to use as a destructor.
     * @return A meta factory for the parent type.
     */
    template<auto *Func>
    meta_factory dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type *>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_dtor_node node{
            &internal::meta_info<Type>::template dtor<Func>,
            type,
            [](meta_handle handle) {
                return handle.type() == internal::meta_info<Type>::resolve()->meta()
                        ? ((*Func)(static_cast<Type *>(handle.data())), true)
                        : false;
            },
            []() -> meta_dtor {
                return &node;
            }
        };

        ENTT_ASSERT(!internal::meta_info<Type>::type->dtor);
        ENTT_ASSERT((!internal::meta_info<Type>::template dtor<Func>));
        internal::meta_info<Type>::template dtor<Func> = &node;
        internal::meta_info<Type>::type->dtor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type.
     *
     * Both data members and static and global variables, as well as constants
     * of any kind, can be assigned to a meta type.<br/>
     * From a client's point of view, all the variables associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Data The actual variable to attach to the meta type.
     * @tparam Property Types of properties to assign to the meta data.
     * @param str The name to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Data, typename... Property>
    meta_factory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::resolve();
        internal::meta_data_node *curr = nullptr;

        if constexpr(std::is_same_v<Type, decltype(Data)>) {
            static internal::meta_data_node node{
                &internal::meta_info<Type>::template data<Data>,
                {},
                type,
                nullptr,
                nullptr,
                true,
                true,
                &internal::meta_info<Type>::resolve,
                [](meta_handle, meta_any, meta_any) { return false; },
                [](meta_handle, meta_any) -> meta_any { return Data; },
                []() -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<Type, Data>>(std::forward<Property>(property)...);
            curr = &node;
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_reference_t<decltype(std::declval<Type>().*Data)>;

            static internal::meta_data_node node{
                &internal::meta_info<Type>::template data<Data>,
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data>,
                []() -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...);
            curr = &node;
        } else {
            static_assert(std::is_pointer_v<decltype(Data)>);
            using data_type = std::remove_pointer_t<decltype(Data)>;

            static internal::meta_data_node node{
                &internal::meta_info<Type>::template data<Data>,
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data>,
                []() -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...);
            curr = &node;
        }

        curr->name = hashed_string{str};
        curr->next = type->data;
        ENTT_ASSERT(!duplicate(hashed_string{str}, curr->next));
        ENTT_ASSERT((!internal::meta_info<Type>::template data<Data>));
        internal::meta_info<Type>::template data<Data> = curr;
        type->data = curr;

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     *
     * Setters and getters can be either free functions, member functions or a
     * mix of them.<br/>
     * In case of free functions, setters and getters must accept a pointer to
     * an instance of the parent type as their first argument. A setter has then
     * an extra argument of a type convertible to that of the parameter to
     * set.<br/>
     * In case of member functions, getters have no arguments at all, while
     * setters has an argument of a type convertible to that of the parameter to
     * set.
     *
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Property Types of properties to assign to the meta data.
     * @param str The name to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename... Property>
    meta_factory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::tuple<std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>;
        using underlying_type = std::invoke_result_t<decltype(Getter), Type *>;
        static_assert(std::is_invocable_v<decltype(Setter), Type *, underlying_type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_data_node node{
            &internal::meta_info<Type>::template data<Setter, Getter>,
            {},
            type,
            nullptr,
            nullptr,
            false,
            false,
            &internal::meta_info<underlying_type>::resolve,
            &internal::setter<false, Type, Setter>,
            &internal::getter<Type, Getter>,
            []() -> meta_data {
                return &node;
            }
        };

        node.name = hashed_string{str};
        node.next = type->data;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(hashed_string{str}, node.next));
        ENTT_ASSERT((!internal::meta_info<Type>::template data<Setter, Getter>));
        internal::meta_info<Type>::template data<Setter, Getter> = &node;
        type->data = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta funcion to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Func The actual function to attach to the meta type.
     * @tparam Property Types of properties to assign to the meta function.
     * @param str The name to assign to the meta function.
     * @param property Properties to assign to the meta function.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::integral_constant<decltype(Func), Func>;
        using func_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            &internal::meta_info<Type>::template func<Func>,
            {},
            type,
            nullptr,
            nullptr,
            func_type::size,
            func_type::is_const,
            func_type::is_static,
            &internal::meta_info<typename func_type::return_type>::resolve,
            &func_type::arg,
            [](meta_handle handle, meta_any *any) {
                return internal::invoke<Type, Func>(handle, any, std::make_index_sequence<func_type::size>{});
            },
            []() -> meta_func {
                return &node;
            }
        };

        node.name = hashed_string{str};
        node.next = type->func;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(hashed_string{str}, node.next));
        ENTT_ASSERT((!internal::meta_info<Type>::template func<Func>));
        internal::meta_info<Type>::template func<Func> = &node;
        type->func = &node;

        return *this;
    }
};


/**
 * @brief Basic function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @tparam Property Types of properties to assign to the reflected type.
 * @param str The name to assign to the reflected type.
 * @param property Properties to assign to the reflected type.
 * @return A meta factory for the given type.
 */
template<typename Type, typename... Property>
inline meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return meta_factory<Type>{}.type(hashed_string{str}, std::forward<Property>(property)...);
}


/**
 * @brief Basic function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @return A meta factory for the given type.
 */
template<typename Type>
inline meta_factory<Type> reflect() ENTT_NOEXCEPT {
    return meta_factory<Type>{};
}


/**
 * @brief Basic function to unregister a type.
 *
 * This function unregisters a type and all its data members, member functions
 * and properties, as well as its constructors, destructors and conversion
 * functions if any.<br/>
 * Base classes aren't unregistered but the link between the two types is
 * removed.
 *
 * @tparam Type Type to unregister.
 * @return True if the type to unregister exists, false otherwise.
 */
template<typename Type>
inline bool unregister() ENTT_NOEXCEPT {
    return meta_factory<Type>().unregister();
}


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve()->meta();
}


/**
 * @brief Returns the meta type associated with a given name.
 * @param str The name to use to search for a meta type.
 * @return The meta type associated with the given name, if any.
 */
inline meta_type resolve(const char *str) ENTT_NOEXCEPT {
    const auto *curr = internal::find_if([name = hashed_string{str}](auto *node) {
        return node->name == name;
    }, internal::meta_info<>::type);

    return curr ? curr->meta() : meta_type{};
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
void resolve(Op op) ENTT_NOEXCEPT {
    internal::iterate([op = std::move(op)](auto *node) {
        op(node->meta());
    }, internal::meta_info<>::type);
}


}


#endif // ENTT_META_FACTORY_HPP

// #include "meta/meta.hpp"

// #include "process/process.hpp"
#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP


#include <utility>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @brief Base class for processes.
 *
 * This class stays true to the CRTP idiom. Derived classes must specify what's
 * the intended type for elapsed times.<br/>
 * A process should expose publicly the following member functions whether
 * required:
 *
 * * @code{.cpp}
 *   void update(Delta, void *);
 *   @endcode
 *
 *   It's invoked once per tick until a process is explicitly aborted or it
 *   terminates either with or without errors. Even though it's not mandatory to
 *   declare this member function, as a rule of thumb each process should at
 *   least define it to work properly. The `void *` parameter is an opaque
 *   pointer to user data (if any) forwarded directly to the process during an
 *   update.
 *
 * * @code{.cpp}
 *   void init();
 *   @endcode
 *
 *   It's invoked when the process joins the running queue of a scheduler. This
 *   happens as soon as it's attached to the scheduler if the process is a top
 *   level one, otherwise when it replaces its parent if the process is a
 *   continuation.
 *
 * * @code{.cpp}
 *   void succeeded();
 *   @endcode
 *
 *   It's invoked in case of success, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void failed();
 *   @endcode
 *
 *   It's invoked in case of errors, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void aborted();
 *   @endcode
 *
 *   It's invoked only if a process is explicitly aborted. There is no guarantee
 *   that it executes in the same tick, this depends solely on whether the
 *   process is aborted immediately or not.
 *
 * Derived classes can change the internal state of a process by invoking the
 * `succeed` and `fail` protected member functions and even pause or unpause the
 * process itself.
 *
 * @sa scheduler
 *
 * @tparam Derived Actual type of process that extends the class template.
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Derived, typename Delta>
class process {
    enum class state: unsigned int {
        UNINITIALIZED = 0,
        RUNNING,
        PAUSED,
        SUCCEEDED,
        FAILED,
        ABORTED,
        FINISHED
    };

    template<state value>
    using state_value_t = std::integral_constant<state, value>;

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::UNINITIALIZED>)
    -> decltype(std::declval<Target>().init()) {
        static_cast<Target *>(this)->init();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::RUNNING>, Delta delta, void *data)
    -> decltype(std::declval<Target>().update(delta, data)) {
        static_cast<Target *>(this)->update(delta, data);
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::SUCCEEDED>)
    -> decltype(std::declval<Target>().succeeded()) {
        static_cast<Target *>(this)->succeeded();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::FAILED>)
    -> decltype(std::declval<Target>().failed()) {
        static_cast<Target *>(this)->failed();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::ABORTED>)
    -> decltype(std::declval<Target>().aborted()) {
        static_cast<Target *>(this)->aborted();
    }

    template<state value, typename... Args>
    void tick(char, state_value_t<value>, Args &&...) const ENTT_NOEXCEPT {}

protected:
    /**
     * @brief Terminates a process with success if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     */
    void succeed() ENTT_NOEXCEPT {
        if(alive()) {
            current = state::SUCCEEDED;
        }
    }

    /**
     * @brief Terminates a process with errors if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     */
    void fail() ENTT_NOEXCEPT {
        if(alive()) {
            current = state::FAILED;
        }
    }

    /**
     * @brief Stops a process if it's in a running state.
     *
     * The function is idempotent and it does nothing if the process isn't
     * running.
     */
    void pause() ENTT_NOEXCEPT {
        if(current == state::RUNNING) {
            current = state::PAUSED;
        }
    }

    /**
     * @brief Restarts a process if it's paused.
     *
     * The function is idempotent and it does nothing if the process isn't
     * paused.
     */
    void unpause() ENTT_NOEXCEPT {
        if(current  == state::PAUSED) {
            current  = state::RUNNING;
        }
    }

public:
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /*! @brief Default destructor. */
    virtual ~process() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<process, Derived>);
    }

    /**
     * @brief Aborts a process if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     *
     * @param immediately Requests an immediate operation.
     */
    void abort(const bool immediately = false) ENTT_NOEXCEPT {
        if(alive()) {
            current = state::ABORTED;

            if(immediately) {
                tick(0);
            }
        }
    }

    /**
     * @brief Returns true if a process is either running or paused.
     * @return True if the process is still alive, false otherwise.
     */
    bool alive() const ENTT_NOEXCEPT {
        return current == state::RUNNING || current == state::PAUSED;
    }

    /**
     * @brief Returns true if a process is already terminated.
     * @return True if the process is terminated, false otherwise.
     */
    bool dead() const ENTT_NOEXCEPT {
        return current == state::FINISHED;
    }

    /**
     * @brief Returns true if a process is currently paused.
     * @return True if the process is paused, false otherwise.
     */
    bool paused() const ENTT_NOEXCEPT {
        return current == state::PAUSED;
    }

    /**
     * @brief Returns true if a process terminated with errors.
     * @return True if the process terminated with errors, false otherwise.
     */
    bool rejected() const ENTT_NOEXCEPT {
        return stopped;
    }

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void tick(const Delta delta, void *data = nullptr) {
        switch (current) {
        case state::UNINITIALIZED:
            tick(0, state_value_t<state::UNINITIALIZED>{});
            current = state::RUNNING;
            break;
        case state::RUNNING:
            tick(0, state_value_t<state::RUNNING>{}, delta, data);
            break;
        default:
            // suppress warnings
            break;
        }

        // if it's dead, it must be notified and removed immediately
        switch(current) {
        case state::SUCCEEDED:
            tick(0, state_value_t<state::SUCCEEDED>{});
            current = state::FINISHED;
            break;
        case state::FAILED:
            tick(0, state_value_t<state::FAILED>{});
            current = state::FINISHED;
            stopped = true;
            break;
        case state::ABORTED:
            tick(0, state_value_t<state::ABORTED>{});
            current = state::FINISHED;
            stopped = true;
            break;
        default:
            // suppress warnings
            break;
        }
    }

private:
    state current{state::UNINITIALIZED};
    bool stopped{false};
};


/**
 * @brief Adaptor for lambdas and functors to turn them into processes.
 *
 * Lambdas and functors can't be used directly with a scheduler for they are not
 * properly defined processes with managed life cycles.<br/>
 * This class helps in filling the gap and turning lambdas and functors into
 * full featured processes usable by a scheduler.
 *
 * The signature of the function call operator should be equivalent to the
 * following:
 *
 * @code{.cpp}
 * void(Delta delta, void *data, auto succeed, auto fail);
 * @endcode
 *
 * Where:
 *
 * * `delta` is the elapsed time.
 * * `data` is an opaque pointer to user data if any, `nullptr` otherwise.
 * * `succeed` is a function to call when a process terminates with success.
 * * `fail` is a function to call when a process terminates with errors.
 *
 * The signature of the function call operator of both `succeed` and `fail`
 * is equivalent to the following:
 *
 * @code{.cpp}
 * void();
 * @endcode
 *
 * Usually users shouldn't worry about creating adaptors. A scheduler will
 * create them internally each and avery time a lambda or a functor is used as
 * a process.
 *
 * @sa process
 * @sa scheduler
 *
 * @tparam Func Actual type of process.
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Func, typename Delta>
struct process_adaptor: process<process_adaptor<Func, Delta>, Delta>, private Func {
    /**
     * @brief Constructs a process adaptor from a lambda or a functor.
     * @tparam Args Types of arguments to use to initialize the actual process.
     * @param args Parameters to use to initialize the actual process.
     */
    template<typename... Args>
    process_adaptor(Args &&... args)
        : Func{std::forward<Args>(args)...}
    {}

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const Delta delta, void *data) {
        Func::operator()(delta, data, [this]() { this->succeed(); }, [this]() { this->fail(); });
    }
};


}


#endif // ENTT_PROCESS_PROCESS_HPP

// #include "process/scheduler.hpp"
#ifndef ENTT_PROCESS_SCHEDULER_HPP
#define ENTT_PROCESS_SCHEDULER_HPP


#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"

// #include "process.hpp"



namespace entt {


/**
 * @brief Cooperative scheduler for processes.
 *
 * A cooperative scheduler runs processes and helps managing their life cycles.
 *
 * Each process is invoked once per tick. If a process terminates, it's
 * removed automatically from the scheduler and it's never invoked again.<br/>
 * A process can also have a child. In this case, the process is replaced with
 * its child when it terminates if it returns with success. In case of errors,
 * both the process and its child are discarded.
 *
 * Example of use (pseudocode):
 *
 * @code{.cpp}
 * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
 *     // code
 * }).then<my_process>(arguments...);
 * @endcode
 *
 * In order to invoke all scheduled processes, call the `update` member function
 * passing it the elapsed time to forward to the tasks.
 *
 * @sa process
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta>
class scheduler {
    struct process_handler {
        using instance_type = std::unique_ptr<void, void(*)(void *)>;
        using update_fn_type = bool(process_handler &, Delta, void *);
        using abort_fn_type = void(process_handler &, bool);
        using next_type = std::unique_ptr<process_handler>;

        instance_type instance;
        update_fn_type *update;
        abort_fn_type *abort;
        next_type next;
    };

    struct continuation {
        continuation(process_handler *ref)
            : handler{ref}
        {
            ENTT_ASSERT(handler);
        }

        template<typename Proc, typename... Args>
        continuation then(Args &&... args) {
            static_assert(std::is_base_of_v<process<Proc, Delta>, Proc>);
            auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &scheduler::deleter<Proc>};
            handler->next.reset(new process_handler{std::move(proc), &scheduler::update<Proc>, &scheduler::abort<Proc>, nullptr});
            handler = handler->next.get();
            return *this;
        }

        template<typename Func>
        continuation then(Func &&func) {
            return then<process_adaptor<std::decay_t<Func>, Delta>>(std::forward<Func>(func));
        }

    private:
        process_handler *handler;
    };

    template<typename Proc>
    static bool update(process_handler &handler, const Delta delta, void *data) {
        auto *process = static_cast<Proc *>(handler.instance.get());
        process->tick(delta, data);

        auto dead = process->dead();

        if(dead) {
            if(handler.next && !process->rejected()) {
                handler = std::move(*handler.next);
                // forces the process to exit the uninitialized state
                dead = handler.update(handler, {}, nullptr);
            } else {
                handler.instance.reset();
            }
        }

        return dead;
    }

    template<typename Proc>
    static void abort(process_handler &handler, const bool immediately) {
        static_cast<Proc *>(handler.instance.get())->abort(immediately);
    }

    template<typename Proc>
    static void deleter(void *proc) {
        delete static_cast<Proc *>(proc);
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<process_handler>::size_type;

    /*! @brief Default constructor. */
    scheduler() ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    scheduler(scheduler &&) = default;

    /*! @brief Default move assignment operator. @return This scheduler. */
    scheduler & operator=(scheduler &&) = default;

    /**
     * @brief Number of processes currently scheduled.
     * @return Number of processes currently scheduled.
     */
    size_type size() const ENTT_NOEXCEPT {
        return handlers.size();
    }

    /**
     * @brief Returns true if at least a process is currently scheduled.
     * @return True if there are scheduled processes, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return handlers.empty();
    }

    /**
     * @brief Discards all scheduled processes.
     *
     * Processes aren't aborted. They are discarded along with their children
     * and never executed again.
     */
    void clear() {
        handlers.clear();
    }

    /**
     * @brief Schedules a process for the next tick.
     *
     * Returned value is an opaque object that can be used to attach a child to
     * the given process. The child is automatically scheduled when the process
     * terminates and only if the process returns with success.
     *
     * Example of use (pseudocode):
     *
     * @code{.cpp}
     * // schedules a task in the form of a process class
     * scheduler.attach<my_process>(arguments...)
     * // appends a child in the form of a lambda function
     * .then([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of another process class
     * .then<my_other_process>();
     * @endcode
     *
     * @tparam Proc Type of process to schedule.
     * @tparam Args Types of arguments to use to initialize the process.
     * @param args Parameters to use to initialize the process.
     * @return An opaque object to use to concatenate processes.
     */
    template<typename Proc, typename... Args>
    auto attach(Args &&... args) {
        static_assert(std::is_base_of_v<process<Proc, Delta>, Proc>);
        auto proc = typename process_handler::instance_type{new Proc{std::forward<Args>(args)...}, &scheduler::deleter<Proc>};
        process_handler handler{std::move(proc), &scheduler::update<Proc>, &scheduler::abort<Proc>, nullptr};
        // forces the process to exit the uninitialized state
        handler.update(handler, {}, nullptr);
        return continuation{&handlers.emplace_back(std::move(handler))};
    }

    /**
     * @brief Schedules a process for the next tick.
     *
     * A process can be either a lambda or a functor. The scheduler wraps both
     * of them in a process adaptor internally.<br/>
     * The signature of the function call operator should be equivalent to the
     * following:
     *
     * @code{.cpp}
     * void(Delta delta, void *data, auto succeed, auto fail);
     * @endcode
     *
     * Where:
     *
     * * `delta` is the elapsed time.
     * * `data` is an opaque pointer to user data if any, `nullptr` otherwise.
     * * `succeed` is a function to call when a process terminates with success.
     * * `fail` is a function to call when a process terminates with errors.
     *
     * The signature of the function call operator of both `succeed` and `fail`
     * is equivalent to the following:
     *
     * @code{.cpp}
     * void();
     * @endcode
     *
     * Returned value is an opaque object that can be used to attach a child to
     * the given process. The child is automatically scheduled when the process
     * terminates and only if the process returns with success.
     *
     * Example of use (pseudocode):
     *
     * @code{.cpp}
     * // schedules a task in the form of a lambda function
     * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of another lambda function
     * .then([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of a process class
     * .then<my_process>(arguments...);
     * @endcode
     *
     * @sa process_adaptor
     *
     * @tparam Func Type of process to schedule.
     * @param func Either a lambda or a functor to use as a process.
     * @return An opaque object to use to concatenate processes.
     */
    template<typename Func>
    auto attach(Func &&func) {
        using Proc = process_adaptor<std::decay_t<Func>, Delta>;
        return attach<Proc>(std::forward<Func>(func));
    }

    /**
     * @brief Updates all scheduled processes.
     *
     * All scheduled processes are executed in no specific order.<br/>
     * If a process terminates with success, it's replaced with its child, if
     * any. Otherwise, if a process terminates with an error, it's removed along
     * with its child.
     *
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const Delta delta, void *data = nullptr) {
        bool clean = false;

        for(auto pos = handlers.size(); pos; --pos) {
            auto &handler = handlers[pos-1];
            const bool dead = handler.update(handler, delta, data);
            clean = clean || dead;
        }

        if(clean) {
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [](auto &handler) {
                return !handler.instance;
            }), handlers.end());
        }
    }

    /**
     * @brief Aborts all scheduled processes.
     *
     * Unless an immediate operation is requested, the abort is scheduled for
     * the next tick. Processes won't be executed anymore in any case.<br/>
     * Once a process is fully aborted and thus finished, it's discarded along
     * with its child, if any.
     *
     * @param immediately Requests an immediate operation.
     */
    void abort(const bool immediately = false) {
        decltype(handlers) exec;
        exec.swap(handlers);

        std::for_each(exec.begin(), exec.end(), [immediately](auto &handler) {
            handler.abort(handler, immediately);
        });

        std::move(handlers.begin(), handlers.end(), std::back_inserter(exec));
        handlers.swap(exec);
    }

private:
    std::vector<process_handler> handlers{};
};


}


#endif // ENTT_PROCESS_SCHEDULER_HPP

// #include "resource/cache.hpp"
#ifndef ENTT_RESOURCE_CACHE_HPP
#define ENTT_RESOURCE_CACHE_HPP


#include <memory>
#include <utility>
#include <type_traits>
#include <unordered_map>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H

// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP

// #include "handle.hpp"
#ifndef ENTT_RESOURCE_HANDLE_HPP
#define ENTT_RESOURCE_HANDLE_HPP


#include <memory>
#include <utility>
// #include "../config/config.h"

// #include "fwd.hpp"
#ifndef ENTT_RESOURCE_FWD_HPP
#define ENTT_RESOURCE_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @class resource_cache */
template<typename>
class resource_cache;

/*! @class resource_handle */
template<typename>
class resource_handle;

/*! @class resource_loader */
template<typename, typename>
class resource_loader;


}


#endif // ENTT_RESOURCE_FWD_HPP



namespace entt {


/**
 * @brief Shared resource handle.
 *
 * A shared resource handle is a small class that wraps a resource and keeps it
 * alive even if it's deleted from the cache. It can be either copied or
 * moved. A handle shares a reference to the same resource with all the other
 * handles constructed for the same identifier.<br/>
 * As a rule of thumb, resources should never be copied nor moved. Handles are
 * the way to go to keep references to them.
 *
 * @tparam Resource Type of resource managed by a handle.
 */
template<typename Resource>
class resource_handle {
    /*! @brief Resource handles are friends of their caches. */
    friend class resource_cache<Resource>;

    resource_handle(std::shared_ptr<Resource> res) ENTT_NOEXCEPT
        : resource{std::move(res)}
    {}

public:
    /*! @brief Default constructor. */
    resource_handle() ENTT_NOEXCEPT = default;

    /**
     * @brief Gets a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A reference to the managed resource.
     */
    const Resource & get() const ENTT_NOEXCEPT {
        ENTT_ASSERT(static_cast<bool>(resource));
        return *resource;
    }

    /*! @copydoc get */
    Resource & get() ENTT_NOEXCEPT {
        return const_cast<Resource &>(std::as_const(*this).get());
    }

    /*! @copydoc get */
    inline operator const Resource & () const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    inline operator Resource & () ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    inline const Resource & operator *() const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    inline Resource & operator *() ENTT_NOEXCEPT { return get(); }

    /**
     * @brief Gets a pointer to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A pointer to the managed resource or `nullptr` if the handle
     * contains no resource at all.
     */
    inline const Resource * operator->() const ENTT_NOEXCEPT {
        ENTT_ASSERT(static_cast<bool>(resource));
        return resource.get();
    }

    /*! @copydoc operator-> */
    inline Resource * operator->() ENTT_NOEXCEPT {
        return const_cast<Resource *>(std::as_const(*this).operator->());
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    explicit operator bool() const { return static_cast<bool>(resource); }

private:
    std::shared_ptr<Resource> resource;
};


}


#endif // ENTT_RESOURCE_HANDLE_HPP

// #include "loader.hpp"
#ifndef ENTT_RESOURCE_LOADER_HPP
#define ENTT_RESOURCE_LOADER_HPP


#include <memory>
// #include "fwd.hpp"



namespace entt {


/**
 * @brief Base class for resource loaders.
 *
 * Resource loaders must inherit from this class and stay true to the CRTP
 * idiom. Moreover, a resource loader must expose a public, const member
 * function named `load` that accepts a variable number of arguments and returns
 * a shared pointer to the resource just created.<br/>
 * As an example:
 *
 * @code{.cpp}
 * struct my_resource {};
 *
 * struct my_loader: entt::resource_loader<my_loader, my_resource> {
 *     std::shared_ptr<my_resource> load(int) const {
 *         // use the integer value somehow
 *         return std::make_shared<my_resource>();
 *     }
 * };
 * @endcode
 *
 * In general, resource loaders should not have a state or retain data of any
 * type. They should let the cache manage their resources instead.
 *
 * @note
 * Base class and CRTP idiom aren't strictly required with the current
 * implementation. One could argue that a cache can easily work with loaders of
 * any type. However, future changes won't be breaking ones by forcing the use
 * of a base class today and that's why the model is already in its place.
 *
 * @tparam Loader Type of the derived class.
 * @tparam Resource Type of resource for which to use the loader.
 */
template<typename Loader, typename Resource>
class resource_loader {
    /*! @brief Resource loaders are friends of their caches. */
    friend class resource_cache<Resource>;

    /**
     * @brief Loads the resource and returns it.
     * @tparam Args Types of arguments for the loader.
     * @param args Arguments for the loader.
     * @return The resource just loaded or an empty pointer in case of errors.
     */
    template<typename... Args>
    std::shared_ptr<Resource> get(Args &&... args) const {
        return static_cast<const Loader *>(this)->load(std::forward<Args>(args)...);
    }
};


}


#endif // ENTT_RESOURCE_LOADER_HPP

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Simple cache for resources of a given type.
 *
 * Minimal implementation of a cache for resources of a given type. It doesn't
 * offer much functionalities but it's suitable for small or medium sized
 * applications and can be freely inherited to add targeted functionalities for
 * large sized applications.
 *
 * @tparam Resource Type of resources managed by a cache.
 */
template<typename Resource>
class resource_cache {
    using container_type = std::unordered_map<hashed_string::hash_type, std::shared_ptr<Resource>>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename container_type::size_type;
    /*! @brief Type of resources managed by a cache. */
    using resource_type = typename hashed_string::hash_type;

    /*! @brief Default constructor. */
    resource_cache() = default;

    /*! @brief Default move constructor. */
    resource_cache(resource_cache &&) = default;

    /*! @brief Default move assignment operator. @return This cache. */
    resource_cache & operator=(resource_cache &&) = default;

    /**
     * @brief Number of resources managed by a cache.
     * @return Number of resources currently stored.
     */
    size_type size() const ENTT_NOEXCEPT {
        return resources.size();
    }

    /**
     * @brief Returns true if a cache contains no resources, false otherwise.
     * @return True if the cache contains no resources, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return resources.empty();
    }

    /**
     * @brief Clears a cache and discards all its resources.
     *
     * Handles are not invalidated and the memory used by a resource isn't
     * freed as long as at least a handle keeps the resource itself alive.
     */
    void clear() ENTT_NOEXCEPT {
        resources.clear();
    }

    /**
     * @brief Loads the resource that corresponds to a given identifier.
     *
     * In case an identifier isn't already present in the cache, it loads its
     * resource and stores it aside for future uses. Arguments are forwarded
     * directly to the loader in order to construct properly the requested
     * resource.
     *
     * @note
     * If the identifier is already present in the cache, this function does
     * nothing and the arguments are simply discarded.
     *
     * @warning
     * If the resource cannot be loaded correctly, the returned handle will be
     * invalid and any use of it will result in undefined behavior.
     *
     * @tparam Loader Type of loader to use to load the resource if required.
     * @tparam Args Types of arguments to use to load the resource if required.
     * @param id Unique resource identifier.
     * @param args Arguments to use to load the resource if required.
     * @return A handle for the given resource.
     */
    template<typename Loader, typename... Args>
    resource_handle<Resource> load(const resource_type id, Args &&... args) {
        static_assert(std::is_base_of_v<resource_loader<Loader, Resource>, Loader>);
        resource_handle<Resource> handle{};

        if(auto it = resources.find(id); it == resources.cend()) {
            if(auto resource = Loader{}.get(std::forward<Args>(args)...); resource) {
                resources[id] = resource;
                handle = std::move(resource);
            }
        } else {
            handle = it->second;
        }

        return handle;
    }

    /**
     * @brief Reloads a resource or loads it for the first time if not present.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * cache.discard(id);
     * cache.load(id, args...);
     * @endcode
     *
     * Arguments are forwarded directly to the loader in order to construct
     * properly the requested resource.
     *
     * @warning
     * If the resource cannot be loaded correctly, the returned handle will be
     * invalid and any use of it will result in undefined behavior.
     *
     * @tparam Loader Type of loader to use to load the resource.
     * @tparam Args Types of arguments to use to load the resource.
     * @param id Unique resource identifier.
     * @param args Arguments to use to load the resource.
     * @return A handle for the given resource.
     */
    template<typename Loader, typename... Args>
    resource_handle<Resource> reload(const resource_type id, Args &&... args) {
        return (discard(id), load<Loader>(id, std::forward<Args>(args)...));
    }

    /**
     * @brief Creates a temporary handle for a resource.
     *
     * Arguments are forwarded directly to the loader in order to construct
     * properly the requested resource. The handle isn't stored aside and the
     * cache isn't in charge of the lifetime of the resource itself.
     *
     * @tparam Loader Type of loader to use to load the resource.
     * @tparam Args Types of arguments to use to load the resource.
     * @param args Arguments to use to load the resource.
     * @return A handle for the given resource.
     */
    template<typename Loader, typename... Args>
    resource_handle<Resource> temp(Args &&... args) const {
        return { Loader{}.get(std::forward<Args>(args)...) };
    }

    /**
     * @brief Creates a handle for a given resource identifier.
     *
     * A resource handle can be in a either valid or invalid state. In other
     * terms, a resource handle is properly initialized with a resource if the
     * cache contains the resource itself. Otherwise the returned handle is
     * uninitialized and accessing it results in undefined behavior.
     *
     * @sa resource_handle
     *
     * @param id Unique resource identifier.
     * @return A handle for the given resource.
     */
    resource_handle<Resource> handle(const resource_type id) const {
        auto it = resources.find(id);
        return { it == resources.end() ? nullptr : it->second };
    }

    /**
     * @brief Checks if a cache contains a given identifier.
     * @param id Unique resource identifier.
     * @return True if the cache contains the resource, false otherwise.
     */
    bool contains(const resource_type id) const ENTT_NOEXCEPT {
        return (resources.find(id) != resources.cend());
    }

    /**
     * @brief Discards the resource that corresponds to a given identifier.
     *
     * Handles are not invalidated and the memory used by the resource isn't
     * freed as long as at least a handle keeps the resource itself alive.
     *
     * @param id Unique resource identifier.
     */
    void discard(const resource_type id) ENTT_NOEXCEPT {
        if(auto it = resources.find(id); it != resources.end()) {
            resources.erase(it);
        }
    }

private:
    container_type resources;
};


}


#endif // ENTT_RESOURCE_CACHE_HPP

// #include "resource/handle.hpp"

// #include "resource/loader.hpp"

// #include "signal/delegate.hpp"
#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <cstring>
#include <algorithm>
#include <functional>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Ret, typename... Args>
auto to_function_pointer(Ret(*)(Args...)) -> Ret(*)(Args...);


template<typename Ret, typename... Args, typename Type>
auto to_function_pointer(Ret(*)(Type *, Args...), Type *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), Class *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, Class *) -> Ret(*)(Args...);


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Used to wrap a function or a member of a specified type. */
template<auto>
struct connect_arg_t {};


/*! @brief Constant of type connect_arg_t used to disambiguate calls. */
template<auto Func>
constexpr connect_arg_t<Func> connect_arg{};


/**
 * @brief Basic delegate implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class delegate;


/**
 * @brief Utility class to use to send around functions and members.
 *
 * Unmanaged delegate for function pointers and members. Users of this class are
 * in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions (with or without payload) and members provided along with an
 * instance on which to invoke them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using proto_fn_type = Ret(const void *, Args...);

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : fn{nullptr}, data{nullptr}
    {}

    /**
     * @brief Constructs a delegate and connects a free function to it.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Function>();
    }

    /**
     * @brief Constructs a delegate and connects a member for a given instance
     * or a free function with payload.
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type *value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(value_or_instance);
    }

    /**
     * @brief Connects a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), Args...>);
        data = nullptr;

        fn = [](const void *, Args... args) -> Ret {
            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Function, args...));
        };
    }

    /**
     * @brief Connects a member function for a given instance or a free function
     * with payload to a delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>);
        data = value_or_instance;

        fn = [](const void *payload, Args... args) -> Ret {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(payload);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(payload));
            }

            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Candidate, curr, args...));
        };
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate cannot be invoked anymore.
     */
    void reset() ENTT_NOEXCEPT {
        fn = nullptr;
        data = nullptr;
    }

    /**
     * @brief Returns the instance linked to a delegate, if any.
     * @return An opaque pointer to the instance linked to the delegate, if any.
     */
    const void * instance() const ENTT_NOEXCEPT {
        return data;
    }

    /**
     * @brief Triggers a delegate.
     *
     * The delegate invokes the underlying function and returns the result.
     *
     * @warning
     * Attempting to trigger an invalid delegate results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * delegate has not yet been set.
     *
     * @param args Arguments to use to invoke the underlying function.
     * @return The value returned by the underlying function.
     */
    Ret operator()(Args... args) const {
        ENTT_ASSERT(fn);
        return fn(data, args...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also data
        return fn;
    }

    /**
     * @brief Checks if the connected functions differ.
     *
     * Instances connected to delegates are ignored by this operator. Use the
     * `instance` member function instead.
     *
     * @param other Delegate with which to compare.
     * @return False if the connected functions differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Checks if the connected functions differ.
 *
 * Instances connected to delegates are ignored by this operator. Use the
 * `instance` member function instead.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the connected functions differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a
 * function provided to the constructor.
 *
 * @tparam Function A valid free function pointer.
 */
template<auto Function>
delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Function))>>;


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a member
 * or a free function with payload provided to the constructor.
 *
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type *) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Candidate, std::declval<Type *>()))>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP

// #include "signal/dispatcher.hpp"
#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP


#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/family.hpp"
#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @brief Dynamic identifier generator.
 *
 * Utility class template that can be used to assign unique identifiers to types
 * at runtime. Use different specializations to create separate sets of
 * identifiers.
 */
template<typename...>
class family {
    inline static maybe_atomic_t<ENTT_ID_TYPE> identifier;

    template<typename...>
    inline static const auto inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used in place of family_type here
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif // ENTT_PAGE_SIZE


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct fnv1a_traits;


template<>
struct fnv1a_traits<std::uint32_t> {
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const char *curr) ENTT_NOEXCEPT: str{curr} {}
        const char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    inline static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = ENTT_ID_TYPE;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = hashed_string::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    inline static constexpr hash_type to_value(const char (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    inline static hash_type to_value(const char *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * hashed_string hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr hashed_string(const char (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const char * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const char *str;
    hash_type hash;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const hashed_string &lhs, const hashed_string &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the given type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Unsigned integer type. */
    static constexpr auto size = sizeof...(Type);
};


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

// #include "sigh.hpp"
#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>
#include <functional>
#include <type_traits>
// #include "../config/config.h"

// #include "delegate.hpp"

// #include "fwd.hpp"
#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @class delegate */
template<typename>
class delegate;

/*! @class sink */
template<typename>
class sink;

/*! @class sigh */
template<typename, typename>
struct sigh;


}


#endif // ENTT_SIGNAL_FWD_HPP



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename, typename>
struct invoker;


template<typename Ret, typename... Args, typename Collector>
struct invoker<Ret(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &collector, const delegate<Ret(Args...)> &delegate, Args... args) const {
        return collector(delegate(args...));
    }
};


template<typename... Args, typename Collector>
struct invoker<void(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &, const delegate<void(Args...)> &delegate, Args... args) const {
        return (delegate(args...), true);
    }
};


template<typename Ret>
struct null_collector {
    using result_type = Ret;
    bool operator()(result_type) const ENTT_NOEXCEPT { return true; }
};


template<>
struct null_collector<void> {
    using result_type = void;
    bool operator()() const ENTT_NOEXCEPT { return true; }
};


template<typename>
struct default_collector;


template<typename Ret, typename... Args>
struct default_collector<Ret(Args...)> {
    using collector_type = null_collector<Ret>;
};


template<typename Function>
using default_collector_type = typename default_collector<Function>::collector_type;


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Sink implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler declaration.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Function, typename Collector = internal::default_collector_type<Function>>
struct sigh;


/**
 * @brief Sink implementation.
 *
 * A sink is an opaque object used to connect listeners to signals.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of a class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    /*! @brief A signal is allowed to create sinks. */
    template<typename, typename>
    friend struct sigh;

    template<typename Type>
    Type * payload_type(Ret(*)(Type *, Args...));

    sink(std::vector<delegate<Ret(Args...)>> *ref) ENTT_NOEXCEPT
        : calls{ref}
    {}

public:
    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return calls->empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() {
        disconnect<Function>();
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls->emplace_back(std::move(delegate));
    }

    /**
     * @brief Connects a member function or a free function with payload to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) {
        if constexpr(std::is_member_function_pointer_v<decltype(Candidate)>) {
            disconnect<Candidate>(value_or_instance);
        } else {
            disconnect<Candidate>();
        }

        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls->emplace_back(std::move(delegate));
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        delegate<Ret(Args...)> delegate{};

        if constexpr(std::is_invocable_r_v<Ret, decltype(Function), Args...>) {
            delegate.template connect<Function>();
        } else {
            decltype(payload_type(Function)) payload = nullptr;
            delegate.template connect<Function>(payload);
        }

        calls->erase(std::remove(calls->begin(), calls->end(), std::move(delegate)), calls->end());
    }

    /**
     * @brief Disconnects a given member function from a signal.
     * @tparam Member Member function to disconnect from the signal.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class>
    void disconnect(Class *instance) {
        static_assert(std::is_member_function_pointer_v<decltype(Member)>);
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Member>(instance);
        calls->erase(std::remove_if(calls->begin(), calls->end(), [&delegate](const auto &other) {
            return other == delegate && other.instance() == delegate.instance();
        }), calls->end());
    }

    /**
     * @brief Disconnects all the listeners from a signal.
     */
    void disconnect() {
        calls->clear();
    }

private:
    std::vector<delegate<Ret(Args...)>> *calls;
};


/**
 * @brief Unmanaged signal handler definition.
 *
 * Unmanaged signal handler. It works directly with naked pointers to classes
 * and pointers to member functions as well as pointers to free functions. Users
 * of this class are in charge of disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals used later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * The default collector does nothing. To properly collect data, define and use
 * a class that has a call operator the signature of which is `bool(Param)` and:
 *
 * * `Param` is a type to which `Ret` can be converted.
 * * The return type is true if the handler must stop collecting data, false
 * otherwise.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Ret, typename... Args, typename Collector>
struct sigh<Ret(Args...), Collector>: private internal::invoker<Ret(Args...), Collector> {
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
    /*! @brief Collector type. */
    using collector_type = Collector;
    /*! @brief Sink type. */
    using sink_type = entt::sink<Ret(Args...)>;

    /**
     * @brief Instance type when it comes to connecting member functions.
     * @tparam Class Type of class to which the member function belongs.
     */
    template<typename Class>
    using instance_type = Class *;

    /**
     * @brief Number of listeners connected to the signal.
     * @return Number of listeners currently connected.
     */
    size_type size() const ENTT_NOEXCEPT {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return calls.empty();
    }

    /**
     * @brief Returns a sink object for the given signal.
     *
     * A sink is an opaque object used to connect listeners to signals.<br/>
     * The function type for a listener is the one of the signal to which it
     * belongs. The order of invocation of the listeners isn't guaranteed.
     *
     * @return A temporary sink object.
     */
    sink_type sink() ENTT_NOEXCEPT {
        return { &calls };
    }

    /**
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            auto &call = calls[pos-1];
            call(args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     * @param args Arguments to use to invoke listeners.
     * @return An instance of the collector filled with collected data.
     */
    collector_type collect(Args... args) const {
        collector_type collector;

        for(auto &&call: calls) {
            if(!this->invoke(collector, call, args...)) {
                break;
            }
        }

        return collector;
    }

    /**
     * @brief Swaps listeners between the two signals.
     * @param lhs A valid signal object.
     * @param rhs A valid signal object.
     */
    friend void swap(sigh &lhs, sigh &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


}


#endif // ENTT_SIGNAL_SIGH_HPP



namespace entt {


/**
 * @brief Basic dispatcher implementation.
 *
 * A dispatcher can be used either to trigger an immediate event or to enqueue
 * events to be published all together once per tick.<br/>
 * Listeners are provided in the form of member functions. For each event of
 * type `Event`, listeners are such that they can be invoked with an argument of
 * type `const Event &`, no matter what the return type is.
 *
 * The type of the instances is `Class *` (a naked pointer). It means that users
 * must guarantee that the lifetimes of the instances overcome the one of the
 * dispatcher itself to avoid crashes.
 */
class dispatcher {
    using event_family = family<struct internal_dispatcher_event_family>;

    template<typename Class, typename Event>
    using instance_type = typename sigh<void(const Event &)>::template instance_type<Class>;

    struct base_wrapper {
        virtual ~base_wrapper() = default;
        virtual void publish() = 0;
    };

    template<typename Event>
    struct signal_wrapper: base_wrapper {
        using signal_type = sigh<void(const Event &)>;
        using sink_type = typename signal_type::sink_type;

        void publish() override {
            for(const auto &event: events[current]) {
                signal.publish(event);
            }

            events[current++].clear();
            current %= std::extent<decltype(events)>::value;
        }

        inline sink_type sink() ENTT_NOEXCEPT {
            return signal.sink();
        }

        template<typename... Args>
        inline void trigger(Args &&... args) {
            signal.publish({ std::forward<Args>(args)... });
        }

        template<typename... Args>
        inline void enqueue(Args &&... args) {
            events[current].emplace_back(std::forward<Args>(args)...);
        }

    private:
        signal_type signal{};
        std::vector<Event> events[2];
        int current{};
    };

    struct wrapper_data {
        std::unique_ptr<base_wrapper> wrapper;
        ENTT_ID_TYPE runtime_type;
    };

    template<typename Event>
    static auto type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Event>) {
            return named_type_traits<Event>::value;
        } else {
            return event_family::type<Event>;
        }
    }

    template<typename Event>
    signal_wrapper<Event> & assure() {
        const auto wtype = type<Event>();
        wrapper_data *wdata = nullptr;

        if constexpr(is_named_type_v<Event>) {
            const auto it = std::find_if(wrappers.begin(), wrappers.end(), [wtype](const auto &candidate) {
                return candidate.wrapper && candidate.runtime_type == wtype;
            });

            wdata = (it == wrappers.cend() ? &wrappers.emplace_back() : &(*it));
        } else {
            if(!(wtype < wrappers.size())) {
                wrappers.resize(wtype+1);
            }

            wdata = &wrappers[wtype];

            if(wdata->wrapper && wdata->runtime_type != wtype) {
                wrappers.emplace_back();
                std::swap(wrappers[wtype], wrappers.back());
                wdata = &wrappers[wtype];
            }
        }

        if(!wdata->wrapper) {
            wdata->wrapper = std::make_unique<signal_wrapper<Event>>();
            wdata->runtime_type = wtype;
        }

        return static_cast<signal_wrapper<Event> &>(*wdata->wrapper);
    }

public:
    /*! @brief Type of sink for the given event. */
    template<typename Event>
    using sink_type = typename signal_wrapper<Event>::sink_type;

    /**
     * @brief Returns a sink object for the given event.
     *
     * A sink is an opaque object used to connect listeners to events.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(const Event &);
     * @endcode
     *
     * The order of invocation of the listeners isn't guaranteed.
     *
     * @sa sink
     *
     * @tparam Event Type of event of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Event>
    inline sink_type<Event> sink() ENTT_NOEXCEPT {
        return assure<Event>().sink();
    }

    /**
     * @brief Triggers an immediate event of the given type.
     *
     * All the listeners registered for the given type are immediately notified.
     * The event is discarded after the execution.
     *
     * @tparam Event Type of event to trigger.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    inline void trigger(Args &&... args) {
        assure<Event>().trigger(std::forward<Args>(args)...);
    }

    /**
     * @brief Triggers an immediate event of the given type.
     *
     * All the listeners registered for the given type are immediately notified.
     * The event is discarded after the execution.
     *
     * @tparam Event Type of event to trigger.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    inline void trigger(Event &&event) {
        assure<std::decay_t<Event>>().trigger(std::forward<Event>(event));
    }

    /**
     * @brief Enqueues an event of the given type.
     *
     * An event of the given type is queued. No listener is invoked. Use the
     * `update` member function to notify listeners when ready.
     *
     * @tparam Event Type of event to enqueue.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    inline void enqueue(Args &&... args) {
        assure<Event>().enqueue(std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueues an event of the given type.
     *
     * An event of the given type is queued. No listener is invoked. Use the
     * `update` member function to notify listeners when ready.
     *
     * @tparam Event Type of event to enqueue.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    inline void enqueue(Event &&event) {
        assure<std::decay_t<Event>>().enqueue(std::forward<Event>(event));
    }

    /**
     * @brief Delivers all the pending events of the given type.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     *
     * @tparam Event Type of events to send.
     */
    template<typename Event>
    inline void update() {
        assure<Event>().publish();
    }

    /**
     * @brief Delivers all the pending events.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     */
    inline void update() const {
        for(auto pos = wrappers.size(); pos; --pos) {
            auto &wdata = wrappers[pos-1];

            if(wdata.wrapper) {
                wdata.wrapper->publish();
            }
        }
    }

private:
    std::vector<wrapper_data> wrappers;
};


}


#endif // ENTT_SIGNAL_DISPATCHER_HPP

// #include "signal/emitter.hpp"
#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP


#include <type_traits>
#include <functional>
#include <algorithm>
#include <utility>
#include <memory>
#include <vector>
#include <list>
// #include "../config/config.h"

// #include "../core/family.hpp"

// #include "../core/type_traits.hpp"



namespace entt {


/**
 * @brief General purpose event emitter.
 *
 * The emitter class template follows the CRTP idiom. To create a custom emitter
 * type, derived classes must inherit directly from the base class as:
 *
 * @code{.cpp}
 * struct my_emitter: emitter<my_emitter> {
 *     // ...
 * }
 * @endcode
 *
 * Handlers for the type of events are created internally on the fly. It's not
 * required to specify in advance the full list of accepted types.<br/>
 * Moreover, whenever an event is published, an emitter provides the listeners
 * with a reference to itself along with a const reference to the event.
 * Therefore listeners have an handy way to work with it without incurring in
 * the need of capturing a reference to the emitter.
 *
 * @tparam Derived Actual type of emitter that extends the class template.
 */
template<typename Derived>
class emitter {
    using handler_family = family<struct internal_emitter_handler_family>;

    struct base_handler {
        virtual ~base_handler() = default;
        virtual bool empty() const ENTT_NOEXCEPT = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct event_handler: base_handler {
        using listener_type = std::function<void(const Event &, Derived &)>;
        using element_type = std::pair<bool, listener_type>;
        using container_type = std::list<element_type>;
        using connection_type = typename container_type::iterator;

        bool empty() const ENTT_NOEXCEPT override {
            auto pred = [](auto &&element) { return element.first; };

            return std::all_of(once_list.cbegin(), once_list.cend(), pred) &&
                    std::all_of(on_list.cbegin(), on_list.cend(), pred);
        }

        void clear() ENTT_NOEXCEPT override {
            if(publishing) {
                auto func = [](auto &&element) { element.first = true; };
                std::for_each(once_list.begin(), once_list.end(), func);
                std::for_each(on_list.begin(), on_list.end(), func);
            } else {
                once_list.clear();
                on_list.clear();
            }
        }

        inline connection_type once(listener_type listener) {
            return once_list.emplace(once_list.cend(), false, std::move(listener));
        }

        inline connection_type on(listener_type listener) {
            return on_list.emplace(on_list.cend(), false, std::move(listener));
        }

        void erase(connection_type conn) ENTT_NOEXCEPT {
            conn->first = true;

            if(!publishing) {
                auto pred = [](auto &&element) { return element.first; };
                once_list.remove_if(pred);
                on_list.remove_if(pred);
            }
        }

        void publish(const Event &event, Derived &ref) {
            container_type swap_list;
            once_list.swap(swap_list);

            auto func = [&event, &ref](auto &&element) {
                return element.first ? void() : element.second(event, ref);
            };

            publishing = true;

            std::for_each(on_list.rbegin(), on_list.rend(), func);
            std::for_each(swap_list.rbegin(), swap_list.rend(), func);

            publishing = false;

            on_list.remove_if([](auto &&element) { return element.first; });
        }

    private:
        bool publishing{false};
        container_type once_list{};
        container_type on_list{};
    };

    struct handler_data {
        std::unique_ptr<base_handler> handler;
        ENTT_ID_TYPE runtime_type;
    };

    template<typename Event>
    static auto type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Event>) {
            return named_type_traits<Event>::value;
        } else {
            return handler_family::type<Event>;
        }
    }

    template<typename Event>
    event_handler<Event> * assure() const ENTT_NOEXCEPT {
        const auto htype = type<Event>();
        handler_data *hdata = nullptr;

        if constexpr(is_named_type_v<Event>) {
            const auto it = std::find_if(handlers.begin(), handlers.end(), [htype](const auto &candidate) {
                return candidate.handler && candidate.runtime_type == htype;
            });

            hdata = (it == handlers.cend() ? &handlers.emplace_back() : &(*it));
        } else {
            if(!(htype < handlers.size())) {
                handlers.resize(htype+1);
            }

            hdata = &handlers[htype];

            if(hdata->handler && hdata->runtime_type != htype) {
                handlers.emplace_back();
                std::swap(handlers[htype], handlers.back());
                hdata = &handlers[htype];
            }
        }

        if(!hdata->handler) {
            hdata->handler = std::make_unique<event_handler<Event>>();
            hdata->runtime_type = htype;
        }

        return static_cast<event_handler<Event> *>(hdata->handler.get());
    }

public:
    /** @brief Type of listeners accepted for the given event. */
    template<typename Event>
    using listener = typename event_handler<Event>::listener_type;

    /**
     * @brief Generic connection type for events.
     *
     * Type of the connection object returned by the event emitter whenever a
     * listener for the given type is registered.<br/>
     * It can be used to break connections still in use.
     *
     * @tparam Event Type of event for which the connection is created.
     */
    template<typename Event>
    struct connection: private event_handler<Event>::connection_type {
        /** @brief Event emitters are friend classes of connections. */
        friend class emitter;

        /*! @brief Default constructor. */
        connection() ENTT_NOEXCEPT = default;

        /**
         * @brief Creates a connection that wraps its underlying instance.
         * @param conn A connection object to wrap.
         */
        connection(typename event_handler<Event>::connection_type conn)
            : event_handler<Event>::connection_type{std::move(conn)}
        {}
    };

    /*! @brief Default constructor. */
    emitter() ENTT_NOEXCEPT = default;

    /*! @brief Default destructor. */
    virtual ~emitter() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<emitter<Derived>, Derived>);
    }

    /*! @brief Default move constructor. */
    emitter(emitter &&) = default;

    /*! @brief Default move assignment operator. @return This emitter. */
    emitter & operator=(emitter &&) = default;

    /**
     * @brief Emits the given event.
     *
     * All the listeners registered for the specific event type are invoked with
     * the given event. The event type must either have a proper constructor for
     * the arguments provided or be an aggregate type.
     *
     * @tparam Event Type of event to publish.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Parameters to use to initialize the event.
     */
    template<typename Event, typename... Args>
    void publish(Args &&... args) {
        assure<Event>()->publish({ std::forward<Args>(args)... }, *static_cast<Derived *>(this));
    }

    /**
     * @brief Registers a long-lived listener with the event emitter.
     *
     * This method can be used to register a listener designed to be invoked
     * more than once for the given event type.<br/>
     * The connection returned by the method can be freely discarded. It's meant
     * to be used later to disconnect the listener if required.
     *
     * The listener is as a callable object that can be moved and the type of
     * which is `void(const Event &, Derived &)`.
     *
     * @note
     * Whenever an event is emitted, the emitter provides the listener with a
     * reference to the derived class. Listeners don't have to capture those
     * instances for later uses.
     *
     * @tparam Event Type of event to which to connect the listener.
     * @param instance The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    connection<Event> on(listener<Event> instance) {
        return assure<Event>()->on(std::move(instance));
    }

    /**
     * @brief Registers a short-lived listener with the event emitter.
     *
     * This method can be used to register a listener designed to be invoked
     * only once for the given event type.<br/>
     * The connection returned by the method can be freely discarded. It's meant
     * to be used later to disconnect the listener if required.
     *
     * The listener is as a callable object that can be moved and the type of
     * which is `void(const Event &, Derived &)`.
     *
     * @note
     * Whenever an event is emitted, the emitter provides the listener with a
     * reference to the derived class. Listeners don't have to capture those
     * instances for later uses.
     *
     * @tparam Event Type of event to which to connect the listener.
     * @param instance The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    connection<Event> once(listener<Event> instance) {
        return assure<Event>()->once(std::move(instance));
    }

    /**
     * @brief Disconnects a listener from the event emitter.
     *
     * Do not use twice the same connection to disconnect a listener, it results
     * in undefined behavior. Once used, discard the connection object.
     *
     * @tparam Event Type of event of the connection.
     * @param conn A valid connection.
     */
    template<typename Event>
    void erase(connection<Event> conn) ENTT_NOEXCEPT {
        assure<Event>()->erase(std::move(conn));
    }

    /**
     * @brief Disconnects all the listeners for the given event type.
     *
     * All the connections previously returned for the given event are
     * invalidated. Using them results in undefined behavior.
     *
     * @tparam Event Type of event to reset.
     */
    template<typename Event>
    void clear() ENTT_NOEXCEPT {
        assure<Event>()->clear();
    }

    /**
     * @brief Disconnects all the listeners.
     *
     * All the connections previously returned are invalidated. Using them
     * results in undefined behavior.
     */
    void clear() ENTT_NOEXCEPT {
        std::for_each(handlers.begin(), handlers.end(), [](auto &&hdata) {
            return hdata.handler ? hdata.handler->clear() : void();
        });
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Event Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Event>
    bool empty() const ENTT_NOEXCEPT {
        return assure<Event>()->empty();
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return std::all_of(handlers.cbegin(), handlers.cend(), [](auto &&hdata) {
            return !hdata.handler || hdata.handler->empty();
        });
    }

private:
    mutable std::vector<handler_data> handlers{};
};


}


#endif // ENTT_SIGNAL_EMITTER_HPP

// #include "signal/sigh.hpp"

