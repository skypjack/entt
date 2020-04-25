// #include "core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
// #include "utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


#include <utility>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/*! @brief Identity function object (waiting for C++20). */
struct identity {
    /**
     * @brief Returns its argument unchanged.
     * @tparam Type Type of the argument.
     * @param value The actual argument.
     * @return The submitted value as-is.
     */
    template<class Type>
    constexpr Type && operator()(Type &&value) const ENTT_NOEXCEPT {
        return std::forward<Type>(value);
    }
};


/**
 * @brief Constant utility to disambiguate overloaded members of a class.
 * @tparam Type Type of the desired overload.
 * @tparam Class Type of class to which the member belongs.
 * @param member A valid pointer to a member.
 * @return Pointer to the member.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Func Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Func>
constexpr auto overload(Func *func) ENTT_NOEXCEPT { return func; }


/**
 * @brief Helper type for visitors.
 * @tparam Func Types of function objects.
 */
template<class... Func>
struct overloaded: Func... {
    using Func::operator()...;
};


/**
 * @brief Deduction guide.
 * @tparam Func Types of function objects.
 */
template<class... Func>
overloaded(Func...) -> overloaded<Func...>;


/**
 * @brief Basic implementation of a y-combinator.
 * @tparam Func Type of a potentially recursive function.
 */
template<class Func>
struct y_combinator {
    /**
     * @brief Constructs a y-combinator from a given function.
     * @param recursive A potentially recursive function.
     */
    y_combinator(Func recursive):
        func{std::move(recursive)}
    {}

    /**
     * @brief Invokes a y-combinator and therefore its underlying function.
     * @tparam Args Types of arguments to use to invoke the underlying function.
     * @param args Parameters to use to invoke the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) const {
        return func(*this, std::forward<Args>(args)...);
    }

    /*! @copydoc operator()() */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};


}


#endif



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
        if(first < last) {
            for(auto it = first+1; it < last; ++it) {
                auto value = std::move(*it);
                auto pre = it;

                for(; pre > first && compare(value, *(pre-1)); --pre) {
                    *pre = std::move(*(pre-1));
                }

                *pre = std::move(value);
            }
        }
    }
};


/**
 * @brief Function object for performing LSD radix sort.
 * @tparam Bit Number of bits processed per pass.
 * @tparam N Maximum number of bits to sort.
 */
template<std::size_t Bit, std::size_t N>
struct radix_sort {
    static_assert((N % Bit) == 0);

    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given _getter_ to access the
     * actual data to be sorted.
     *
     * This implementation is inspired by the online book
     * [Physically Based Rendering](http://www.pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies.html#RadixSort).
     *
     * @tparam It Type of random access iterator.
     * @tparam Getter Type of _getter_ function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param getter A valid _getter_ function object.
     */
    template<typename It, typename Getter = identity>
    void operator()(It first, It last, Getter getter = Getter{}) const {
        if(first < last) {
            static constexpr auto mask = (1 << Bit) - 1;
            static constexpr auto buckets = 1 << Bit;
            static constexpr auto passes = N / Bit;

            using value_type = typename std::iterator_traits<It>::value_type;
            std::vector<value_type> aux(std::distance(first, last));

            auto part = [getter = std::move(getter)](auto from, auto to, auto out, auto start) {
                std::size_t index[buckets]{};
                std::size_t count[buckets]{};

                std::for_each(from, to, [&getter, &count, start](const value_type &item) {
                    ++count[(getter(item) >> start) & mask];
                });

                std::for_each(std::next(std::begin(index)), std::end(index), [index = std::begin(index), count = std::begin(count)](auto &item) mutable {
                    item = *(index++) + *(count++);
                });

                std::for_each(from, to, [&getter, &out, &index, start](value_type &item) {
                    out[index[(getter(item) >> start) & mask]++] = std::move(item);
                });
            };

            for(std::size_t pass = 0; pass < (passes & ~1); pass += 2) {
                part(first, last, aux.begin(), pass * Bit);
                part(aux.begin(), aux.end(), first, (pass + 1) * Bit);
            }

            if constexpr(passes & 1) {
                part(first, last, aux.begin(), (passes - 1) * Bit);
                std::move(aux.begin(), aux.end(), first);
            }
        }
    }
};


}


#endif

// #include "core/attribute.h"
#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

// #include "core/family.hpp"
#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


// #include "../config/config.h"

// #include "fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif



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
    inline static ENTT_MAYBE_ATOMIC(id_type) identifier{};

public:
    /*! @brief Unsigned integer type. */
    using family_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const family_type type = identifier++;
};


}


#endif

// #include "core/hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>
// #include "../config/config.h"

// #include "fwd.hpp"



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
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    using type = std::uint64_t;
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr id_type helper(const Char *curr) ENTT_NOEXCEPT {
        auto value = traits_type::offset;

        while(*curr != 0) {
            value = (value ^ static_cast<traits_type::type>(*(curr++))) * traits_type::prime;
        }

        return value;
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
    /*! @brief Unsigned integer type. */
    using hash_type = id_type;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        id_type partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * basic_hashed_string<char> hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const value_type * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /*! @copydoc data */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return data(); }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return value(); }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const basic_hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const value_type *str;
    hash_type hash;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the character type of the hashed string directly from a
 * human-readable identifer provided to the constructor.
 *
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifer.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) ENTT_NOEXCEPT
-> basic_hashed_string<Char>;


/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;


/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
constexpr entt::hashed_wstring operator"" ENTT_HWS_SUFFIX(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_wstring{str};
}


#endif

// #include "core/ident.hpp"
#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP


#include <tuple>
#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "fwd.hpp"



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
    static constexpr id_type get(std::index_sequence<Indexes...>) {
        static_assert(std::disjunction_v<std::is_same<Type, Types>...>);
        return (0 + ... + (std::is_same_v<Type, std::tuple_element_t<Indexes, tuple_type>> ? id_type(Indexes) : id_type{}));
    }

public:
    /*! @brief Unsigned integer type. */
    using identifier_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename Type>
    static constexpr identifier_type type = get<std::decay_t<Type>>(std::index_sequence_for<Types...>{});
};


}


#endif

// #include "core/monostate.hpp"
#ifndef ENTT_CORE_MONOSTATE_HPP
#define ENTT_CORE_MONOSTATE_HPP


// #include "../config/config.h"

// #include "fwd.hpp"



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
template<id_type>
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
    inline static ENTT_MAYBE_ATOMIC(Type) value{};
};


/**
 * @brief Helper variable template.
 * @tparam Value Value used to differentiate between different variables.
 */
template<id_type Value>
inline monostate<Value> monostate_v = {};


}


#endif

// #include "core/type_info.hpp"
#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


// #include "../config/config.h"

// #include "../core/attribute.h"
#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

// #include "hashed_string.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_index {
    static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Type index.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_index::next();
        return value;
    }
};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * indexable, false otherwise.
 * @tparam Type Potentially indexable type.
 */
template<typename, typename = void>
struct has_type_index: std::false_type {};


/*! @brief has_type_index */
template<typename Type>
struct has_type_index<Type, std::void_t<decltype(type_index<Type>::value())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially indexable type.
 */
template<typename Type>
inline constexpr bool has_type_index_v = has_type_index<Type>::value;


/**
 * @brief Type info.
 * @tparam Type Type for which to generate information.
 */
template<typename Type, typename = void>
struct ENTT_API type_info {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    static constexpr id_type id() ENTT_NOEXCEPT {
        constexpr auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION_CONSTEXPR);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    static id_type id() ENTT_NOEXCEPT {
        static const auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    static id_type id() ENTT_NOEXCEPT {
        return type_index<Type>::value();
    }
#endif
};


}


#endif

// #include "core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "hashed_string.hpp"

// #include "fwd.hpp"



namespace entt {


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
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>);

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


/**
 * @brief Defines an enum class to use for opaque identifiers and a dedicate
 * `to_integer` function to convert the identifiers to their underlying type.
 * @param clazz The name to use for the enum class.
 * @param type The underlying type for the enum class.
 */
#define ENTT_OPAQUE_TYPE(clazz, type)\
    enum class clazz: type {};\
    constexpr auto to_integral(const clazz id) ENTT_NOEXCEPT {\
        return static_cast<std::underlying_type_t<clazz>>(id);\
    }\
    static_assert(true)


#endif

// #include "core/utility.hpp"

// #include "entity/actor.hpp"
#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <utility>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "registry.hpp"
#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// #include "../config/config.h"

// #include "../core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
// #include "utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


#include <utility>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/*! @brief Identity function object (waiting for C++20). */
struct identity {
    /**
     * @brief Returns its argument unchanged.
     * @tparam Type Type of the argument.
     * @param value The actual argument.
     * @return The submitted value as-is.
     */
    template<class Type>
    constexpr Type && operator()(Type &&value) const ENTT_NOEXCEPT {
        return std::forward<Type>(value);
    }
};


/**
 * @brief Constant utility to disambiguate overloaded members of a class.
 * @tparam Type Type of the desired overload.
 * @tparam Class Type of class to which the member belongs.
 * @param member A valid pointer to a member.
 * @return Pointer to the member.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Func Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Func>
constexpr auto overload(Func *func) ENTT_NOEXCEPT { return func; }


/**
 * @brief Helper type for visitors.
 * @tparam Func Types of function objects.
 */
template<class... Func>
struct overloaded: Func... {
    using Func::operator()...;
};


/**
 * @brief Deduction guide.
 * @tparam Func Types of function objects.
 */
template<class... Func>
overloaded(Func...) -> overloaded<Func...>;


/**
 * @brief Basic implementation of a y-combinator.
 * @tparam Func Type of a potentially recursive function.
 */
template<class Func>
struct y_combinator {
    /**
     * @brief Constructs a y-combinator from a given function.
     * @param recursive A potentially recursive function.
     */
    y_combinator(Func recursive):
        func{std::move(recursive)}
    {}

    /**
     * @brief Invokes a y-combinator and therefore its underlying function.
     * @tparam Args Types of arguments to use to invoke the underlying function.
     * @param args Parameters to use to invoke the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) const {
        return func(*this, std::forward<Args>(args)...);
    }

    /*! @copydoc operator()() */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};


}


#endif



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
        if(first < last) {
            for(auto it = first+1; it < last; ++it) {
                auto value = std::move(*it);
                auto pre = it;

                for(; pre > first && compare(value, *(pre-1)); --pre) {
                    *pre = std::move(*(pre-1));
                }

                *pre = std::move(value);
            }
        }
    }
};


/**
 * @brief Function object for performing LSD radix sort.
 * @tparam Bit Number of bits processed per pass.
 * @tparam N Maximum number of bits to sort.
 */
template<std::size_t Bit, std::size_t N>
struct radix_sort {
    static_assert((N % Bit) == 0);

    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given _getter_ to access the
     * actual data to be sorted.
     *
     * This implementation is inspired by the online book
     * [Physically Based Rendering](http://www.pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies.html#RadixSort).
     *
     * @tparam It Type of random access iterator.
     * @tparam Getter Type of _getter_ function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param getter A valid _getter_ function object.
     */
    template<typename It, typename Getter = identity>
    void operator()(It first, It last, Getter getter = Getter{}) const {
        if(first < last) {
            static constexpr auto mask = (1 << Bit) - 1;
            static constexpr auto buckets = 1 << Bit;
            static constexpr auto passes = N / Bit;

            using value_type = typename std::iterator_traits<It>::value_type;
            std::vector<value_type> aux(std::distance(first, last));

            auto part = [getter = std::move(getter)](auto from, auto to, auto out, auto start) {
                std::size_t index[buckets]{};
                std::size_t count[buckets]{};

                std::for_each(from, to, [&getter, &count, start](const value_type &item) {
                    ++count[(getter(item) >> start) & mask];
                });

                std::for_each(std::next(std::begin(index)), std::end(index), [index = std::begin(index), count = std::begin(count)](auto &item) mutable {
                    item = *(index++) + *(count++);
                });

                std::for_each(from, to, [&getter, &out, &index, start](value_type &item) {
                    out[index[(getter(item) >> start) & mask]++] = std::move(item);
                });
            };

            for(std::size_t pass = 0; pass < (passes & ~1); pass += 2) {
                part(first, last, aux.begin(), pass * Bit);
                part(aux.begin(), aux.end(), first, (pass + 1) * Bit);
            }

            if constexpr(passes & 1) {
                part(first, last, aux.begin(), (passes - 1) * Bit);
                std::move(aux.begin(), aux.end(), first);
            }
        }
    }
};


}


#endif

// #include "../core/fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif

// #include "../core/type_info.hpp"
#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


// #include "../config/config.h"

// #include "../core/attribute.h"
#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

// #include "hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>
// #include "../config/config.h"

// #include "fwd.hpp"



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
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    using type = std::uint64_t;
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr id_type helper(const Char *curr) ENTT_NOEXCEPT {
        auto value = traits_type::offset;

        while(*curr != 0) {
            value = (value ^ static_cast<traits_type::type>(*(curr++))) * traits_type::prime;
        }

        return value;
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
    /*! @brief Unsigned integer type. */
    using hash_type = id_type;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        id_type partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * basic_hashed_string<char> hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const value_type * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /*! @copydoc data */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return data(); }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return value(); }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const basic_hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const value_type *str;
    hash_type hash;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the character type of the hashed string directly from a
 * human-readable identifer provided to the constructor.
 *
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifer.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) ENTT_NOEXCEPT
-> basic_hashed_string<Char>;


/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;


/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
constexpr entt::hashed_wstring operator"" ENTT_HWS_SUFFIX(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_wstring{str};
}


#endif

// #include "fwd.hpp"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_index {
    static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Type index.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_index::next();
        return value;
    }
};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * indexable, false otherwise.
 * @tparam Type Potentially indexable type.
 */
template<typename, typename = void>
struct has_type_index: std::false_type {};


/*! @brief has_type_index */
template<typename Type>
struct has_type_index<Type, std::void_t<decltype(type_index<Type>::value())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially indexable type.
 */
template<typename Type>
inline constexpr bool has_type_index_v = has_type_index<Type>::value;


/**
 * @brief Type info.
 * @tparam Type Type for which to generate information.
 */
template<typename Type, typename = void>
struct ENTT_API type_info {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    static constexpr id_type id() ENTT_NOEXCEPT {
        constexpr auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION_CONSTEXPR);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    static id_type id() ENTT_NOEXCEPT {
        static const auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    static id_type id() ENTT_NOEXCEPT {
        return type_index<Type>::value();
    }
#endif
};


}


#endif

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "hashed_string.hpp"

// #include "fwd.hpp"



namespace entt {


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
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>);

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


/**
 * @brief Defines an enum class to use for opaque identifiers and a dedicate
 * `to_integer` function to convert the identifiers to their underlying type.
 * @param clazz The name to use for the enum class.
 * @param type The underlying type for the enum class.
 */
#define ENTT_OPAQUE_TYPE(clazz, type)\
    enum class clazz: type {};\
    constexpr auto to_integral(const clazz id) ENTT_NOEXCEPT {\
        return static_cast<std::underlying_type_t<clazz>>(id);\
    }\
    static_assert(true)


#endif

// #include "../signal/sigh.hpp"
#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "delegate.hpp"
#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <tuple>
#include <cstddef>
#include <utility>
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
auto function_pointer(Ret(*)(Args...)) -> Ret(*)(Args...);


template<typename Ret, typename Type, typename... Args, typename Other>
auto function_pointer(Ret(*)(Type, Args...), Other &&) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args, typename... Other>
auto function_pointer(Ret(Class:: *)(Args...), Other &&...) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args, typename... Other>
auto function_pointer(Ret(Class:: *)(Args...) const, Other &&...) -> Ret(*)(Args...);


template<typename Class, typename Type, typename... Other>
auto function_pointer(Type Class:: *, Other &&...) -> Type(*)();


template<typename... Type>
using function_pointer_t = decltype(internal::function_pointer(std::declval<Type>()...));


template<typename... Class, typename Ret, typename... Args>
constexpr auto index_sequence_for(Ret(*)(Args...)) {
    return std::index_sequence_for<Class..., Args...>{};
}


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
inline constexpr connect_arg_t<Func> connect_arg{};


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
 * A delegate can be used as a general purpose invoker without memory overhead
 * for free functions possibly with payloads and bound or unbound members.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using proto_fn_type = Ret(const void *, Args...);

    template<auto Candidate, std::size_t... Index>
    auto wrap(std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            return Ret(std::invoke(Candidate, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    auto wrap(Type &, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *payload, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            return Ret(std::invoke(Candidate, *curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    auto wrap(Type *, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *payload, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            return Ret(std::invoke(Candidate, curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : fn{nullptr}, data{nullptr}
    {}

    /**
     * @brief Constructs a delegate and connects a free function or an unbound
     * member.
     * @tparam Candidate Function or member to connect to the delegate.
     */
    template<auto Candidate>
    delegate(connect_arg_t<Candidate>) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>();
    }

    /**
     * @brief Constructs a delegate and connects a free function with payload or
     * a bound member.
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type &&value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(std::forward<Type>(value_or_instance));
    }

    /**
     * @brief Connects a free function or an unbound member to a delegate.
     * @tparam Candidate Function or member to connect to the delegate.
     */
    template<auto Candidate>
    void connect() ENTT_NOEXCEPT {
        data = nullptr;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>) {
            fn = [](const void *, Args... args) -> Ret {
                return Ret(std::invoke(Candidate, std::forward<Args>(args)...));
            };
        } else if constexpr(std::is_member_pointer_v<decltype(Candidate)>) {
            fn = wrap<Candidate>(internal::index_sequence_for<std::tuple_element_t<0, std::tuple<Args...>>>(internal::function_pointer_t<decltype(Candidate)>{}));
        } else {
            fn = wrap<Candidate>(internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate)>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) ENTT_NOEXCEPT {
        data = &value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, Args...>) {
            fn = [](const void *payload, Args... args) -> Ret {
                Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
                return Ret(std::invoke(Candidate, *curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * @sa connect(Type &)
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) ENTT_NOEXCEPT {
        data = value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>) {
            fn = [](const void *payload, Args... args) -> Ret {
                Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
                return Ret(std::invoke(Candidate, curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
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
     * @brief Returns the instance or the payload linked to a delegate, if any.
     * @return An opaque pointer to the underlying data.
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
        return fn(data, std::forward<Args>(args)...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also data
        return !(fn == nullptr);
    }

    /**
     * @brief Compares the contents of two delegates.
     * @param other Delegate with which to compare.
     * @return False if the two contents differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn && data == other.data;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Compares the contents of two delegates.
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the two contents differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 */
template<auto Candidate>
delegate(connect_arg_t<Candidate>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate)>>>;


/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &&) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate), Type>>>;


}


#endif

// #include "fwd.hpp"
#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP


namespace entt {


/*! @class delegate */
template<typename>
class delegate;

/*! @class dispatcher */
class dispatcher;

/*! @class emitter */
template<typename>
class emitter;

/*! @class connection */
class connection;

/*! @class scoped_connection */
struct scoped_connection;

/*! @class sink */
template<typename>
class sink;

/*! @class sigh */
template<typename>
class sigh;


}


#endif



namespace entt {


/**
 * @brief Sink class.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sigh;


/**
 * @brief Unmanaged signal handler.
 *
 * It works directly with references to classes and pointers to member functions
 * as well as pointers to free functions. Users of this class are in charge of
 * disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals to use later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sigh<Ret(Args...)> {
    /*! @brief A sink is allowed to modify a signal. */
    friend class sink<Ret(Args...)>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
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
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) const {
        for(auto &&call: std::as_const(calls)) {
            call(args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     *
     * The collector must expose a call operator with the following properties:
     *
     * * The return type is either `void` or such that it's convertible to
     *   `bool`. In the second case, a true value will stop the iteration.
     * * The list of parameters is empty if `Ret` is `void`, otherwise it
     *   contains a single element such that `Ret` is convertible to it.
     *
     * @tparam Func Type of collector to use, if any.
     * @param func A valid function object.
     * @param args Arguments to use to invoke listeners.
     */
    template<typename Func>
    void collect(Func func, Args... args) const {
        for(auto &&call: calls) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    call(args...);
                    if(func()) { break; }
                } else {
                    call(args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    if(func(call(args...))) { break; }
                } else {
                    func(call(args...));
                }
            }
        }
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


/**
 * @brief Connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.
 */
class connection {
    /*! @brief A sink is allowed to create connection objects. */
    template<typename>
    friend class sink;

    connection(delegate<void(void *)> fn, void *ref)
        : disconnect{fn}, signal{ref}
    {}

public:
    /*! @brief Default constructor. */
    connection() = default;

    /**
     * @brief Checks whether a connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(disconnect);
    }

    /*! @brief Breaks the connection. */
    void release() {
        if(disconnect) {
            disconnect(signal);
            disconnect.reset();
        }
    }

private:
    delegate<void(void *)> disconnect;
    void *signal{};
};


/**
 * @brief Scoped connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.<br/>
 * A scoped connection automatically breaks the link between the two objects
 * when it goes out of scope.
 */
struct scoped_connection {
    /*! @brief Default constructor. */
    scoped_connection() = default;

    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param other A valid connection object.
     */
    scoped_connection(const connection &other)
        : conn{other}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        conn.release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const scoped_connection &) = delete;

    /**
     * @brief Acquires a connection.
     * @param other The connection object to acquire.
     * @return This scoped connection.
     */
    scoped_connection & operator=(connection other) {
        conn = std::move(other);
        return *this;
    }

    /**
     * @brief Checks whether a scoped connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(conn);
    }

    /*! @brief Breaks the connection. */
    void release() {
        conn.release();
    }

private:
    connection conn;
};


/**
 * @brief Sink class.
 *
 * A sink is used to connect listeners to signals and to disconnect them.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of the class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    using signal_type = sigh<Ret(Args...)>;
    using difference_type = typename std::iterator_traits<typename decltype(signal_type::calls)::iterator>::difference_type;

    template<auto Candidate, typename Type>
    static void release(Type value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Candidate>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>();
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : offset{},
          signal{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return signal->calls.empty();
    }

    /**
     * @brief Returns a sink that connects before a given free function or an
     * unbound member.
     * @tparam Function A valid free function pointer.
     * @return A properly initialized sink object.
     */
    template<auto Function>
    sink before() {
        delegate<Ret(Args...)> call{};
        call.template connect<Function>();

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a free function with payload
     * or a bound member.
     * @tparam Candidate Member or free function to look for.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<auto Candidate, typename Type>
    sink before(Type &&value_or_instance) {
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(std::forward<Type>(value_or_instance));

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    sink before(Type &value_or_instance) {
        return before(&value_or_instance);
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    sink before(Type *value_or_instance) {
        sink other{*this};

        if(value_or_instance) {
            const auto &calls = signal->calls;
            const auto it = std::find_if(calls.cbegin(), calls.cend(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            });

            other.offset = std::distance(it, calls.cend());
        }

        return other;
    }

    /**
     * @brief Returns a sink that connects before anything else.
     * @return A properly initialized sink object.
     */
    sink before() {
        sink other{*this};
        other.offset = signal->calls.size();
        return other;
    }

    /**
     * @brief Connects a free function or an unbound member to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for the
     * same function.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @return A properly initialized connection object.
     */
    template<auto Candidate>
    connection connect() {
        disconnect<Candidate>();

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate>>();
        return { std::move(conn), signal };
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one of the signal. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the signal itself.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type &&value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(value_or_instance);
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type>>(value_or_instance);
        return { std::move(conn), signal };
    }

    /**
     * @brief Disconnects a free function or an unbound member from a signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     */
    template<auto Candidate>
    void disconnect() {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects a free function with payload or a bound member from a
     * signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type &&value_or_instance) {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(std::forward<Type>(value_or_instance));
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type &value_or_instance) {
        disconnect(&value_or_instance);
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type *value_or_instance) {
        if(value_or_instance) {
            auto &calls = signal->calls;
            calls.erase(std::remove_if(calls.begin(), calls.end(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            }), calls.end());
        }
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
    difference_type offset;
    signal_type *signal;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of a sink directly from the signal it
 * refers to.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
sink(sigh<Ret(Args...)> &) ENTT_NOEXCEPT -> sink<Ret(Args...)>;


}


#endif

// #include "entity.hpp"
#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include <cstdint>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "../core/fwd.hpp"



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


class null {
    template<typename Entity>
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

public:
    template<typename Entity>
    constexpr operator Entity() const ENTT_NOEXCEPT {
        return Entity{traits_type<Entity>::entity_mask};
    }

    constexpr bool operator==(null) const ENTT_NOEXCEPT {
        return true;
    }

    constexpr bool operator!=(null) const ENTT_NOEXCEPT {
        return false;
    }

    template<typename Entity>
    constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return (to_integral(entity) & traits_type<Entity>::entity_mask) == to_integral(static_cast<Entity>(*this));
    }

    template<typename Entity>
    constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
    }
};


template<typename Entity>
constexpr bool operator==(const Entity entity, null other) ENTT_NOEXCEPT {
    return other.operator==(entity);
}


template<typename Entity>
constexpr bool operator!=(const Entity entity, null other) ENTT_NOEXCEPT {
    return !(other == entity);
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Default entity identifier. */
ENTT_OPAQUE_TYPE(entity, id_type);


/**
 * @brief Compile-time constant for null entities.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
inline constexpr auto null = internal::null{};


}


#endif

// #include "fwd.hpp"
#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


// #include "../core/fwd.hpp"



namespace entt {


/*! @class basic_registry */
template <typename>
class basic_registry;

/*! @class basic_view */
template<typename...>
class basic_view;

/*! @class basic_runtime_view */
template<typename>
class basic_runtime_view;

/*! @class basic_group */
template<typename...>
class basic_group;

/*! @class basic_observer */
template<typename>
class basic_observer;

/*! @struct basic_actor */
template <typename>
struct basic_actor;

/*! @class basic_snapshot */
template<typename>
class basic_snapshot;

/*! @class basic_snapshot_loader */
template<typename>
class basic_snapshot_loader;

/*! @class basic_continuous_loader */
template<typename>
class basic_continuous_loader;

/*! @class entity */
enum class entity: id_type;

/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;

/*! @brief Alias declaration for the most common use case. */
using observer = basic_observer<entity>;

/*! @brief Alias declaration for the most common use case. */
using actor = basic_actor<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot = basic_snapshot<entity>;

/*! @brief Alias declaration for the most common use case. */
using snapshot_loader = basic_snapshot_loader<entity>;

/*! @brief Alias declaration for the most common use case. */
using continuous_loader = basic_continuous_loader<entity>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Types Types of components iterated by the view.
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


#endif

// #include "group.hpp"
#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP


#include <tuple>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "sparse_set.hpp"
#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/algorithm.hpp"

// #include "entity.hpp"

// #include "fwd.hpp"



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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0));
    static constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(typename traits_type::entity_type);

    class sparse_set_iterator final {
        friend class sparse_set<Entity>;

        using direct_type = std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        sparse_set_iterator(const direct_type &ref, const index_type idx) ENTT_NOEXCEPT
            : direct{&ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() ENTT_NOEXCEPT = default;

        sparse_set_iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        sparse_set_iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator++(), orig;
        }

        sparse_set_iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        sparse_set_iterator operator--(int) ENTT_NOEXCEPT {
            sparse_set_iterator orig = *this;
            return operator--(), orig;
        }

        sparse_set_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        sparse_set_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            sparse_set_iterator copy = *this;
            return (copy += value);
        }

        sparse_set_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        sparse_set_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const {
            const auto pos = size_type(index-value-1);
            return (*direct)[pos];
        }

        bool operator==(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        bool operator!=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const {
            const auto pos = size_type(index-1);
            return &(*direct)[pos];
        }

        reference operator*() const {
            return *operator->();
        }

    private:
        const direct_type *direct;
        index_type index;
    };

    auto page(const Entity entt) const ENTT_NOEXCEPT {
        return std::size_t{(to_integral(entt) & traits_type::entity_mask) / entt_per_page};
    }

    auto offset(const Entity entt) const ENTT_NOEXCEPT {
        return std::size_t{to_integral(entt) & (entt_per_page - 1)};
    }

    Entity * assure(const std::size_t pos) {
        if(!(pos < reverse.size())) {
            reverse.resize(pos+1);
        }

        if(!reverse[pos]) {
            reverse[pos] = std::make_unique<entity_type[]>(entt_per_page);
            // null is safe in all cases for our purposes
            std::fill_n(reverse[pos].get(), entt_per_page, null);
        }

        return reverse[pos].get();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;

    /*! @brief Default constructor. */
    sparse_set() = default;

    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() = default;

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
    void reserve(const size_type cap) {
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
    void shrink_to_fit() {
        // conservative approach
        if(direct.empty()) {
            reverse.clear();
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
     * Random access iterators stay true to the order imposed by a call to
     * `respect`.
     *
     * @return An iterator to the first entity of the internal packed array.
     */
    iterator begin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = direct.size();
        return iterator{direct, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @note
     * Random access iterators stay true to the order imposed by a call to
     * `respect`.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
     */
    iterator end() const ENTT_NOEXCEPT {
        return iterator{direct, {}};
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator find(const entity_type entt) const {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        const auto curr = page(entt);
        // testing against null permits to avoid accessing the direct vector
        return (curr < reverse.size() && reverse[curr] && reverse[curr][offset(entt)] != null);
    }

    /*! @copydoc contains */
    [[deprecated("use ::contains instead")]]
    bool has(const entity_type entt) const {
        return contains(entt);
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
    size_type index(const entity_type entt) const {
        ENTT_ASSERT(contains(entt));
        return size_type(reverse[page(entt)][offset(entt)]);
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
    void emplace(const entity_type entt) {
        ENTT_ASSERT(!contains(entt));
        assure(page(entt))[offset(entt)] = entity_type(direct.size());
        direct.push_back(entt);
    }

    /*! @copydoc emplace */
    [[deprecated("use ::emplace instead")]]
    void construct(const entity_type entt) {
        emplace(entt);
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
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last) {
        std::for_each(first, last, [this, next = direct.size()](const auto entt) mutable {
            ENTT_ASSERT(!contains(entt));
            assure(page(entt))[offset(entt)] = entity_type(next++);
        });

        direct.insert(direct.end(), first, last);
    }

    /*! @copydoc insert */
    template<typename It>
    [[deprecated("use ::insert instead")]]
    void construct(It first, It last) {
        insert(std::move(first), std::move(last));
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
    void erase(const entity_type entt) {
        ENTT_ASSERT(contains(entt));
        const auto curr = page(entt);
        const auto pos = offset(entt);
        direct[size_type(reverse[curr][pos])] = entity_type(direct.back());
        reverse[page(direct.back())][offset(direct.back())] = reverse[curr][pos];
        reverse[curr][pos] = null;
        direct.pop_back();
    }

    /*! @copydoc erase */
    [[deprecated("use ::erase instead")]]
    void destroy(const entity_type entt) {
        erase(entt);
    }

    /**
     * @brief Swaps two entities in the internal packed array.
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
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    virtual void swap(const entity_type lhs, const entity_type rhs) {
        auto &from = reverse[page(lhs)][offset(lhs)];
        auto &to = reverse[page(rhs)][offset(rhs)];
        std::swap(direct[size_type(from)], direct[size_type(to)]);
        std::swap(from, to);
    }

    /**
     * @brief Sort elements according to the given comparison function.
     *
     * Sort the elements so that iterating the range with a couple of iterators
     * returns them in the expected order. See `begin` and `end` for more
     * details.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
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
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * `data` gives no guarantees on the order, even though `sort` has been
     * invoked.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator first, iterator last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = direct.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            reverse[page(direct[pos])][offset(direct[pos])] = entity_type(pos);
        }
    }

    /**
     * @brief Sort elements according to the given comparison function.
     *
     * @sa sort
     *
     * This function is a slightly slower version of `sort` that invokes the
     * caller to indicate which entities are swapped.<br/>
     * It's recommended when the caller wants to sort its own data structures to
     * align them with the order induced in the sparse set.
     *
     * The signature of the callback should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * @endcode
     *
     * @tparam Apply Type of function object to invoke to notify the caller.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param apply A valid function object to use as a callback.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Apply, typename Compare, typename Sort = std_sort, typename... Args>
    void arrange(iterator first, iterator last, Apply apply, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = direct.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            auto curr = pos;
            auto next = index(direct[curr]);

            while(curr != next) {
                apply(direct[curr], direct[next]);
                reverse[page(direct[curr])][offset(direct[curr])] = entity_type(curr);

                curr = next;
                next = index(direct[curr]);
            }
        }
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantees on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * `data` gives no guarantees on the order, even though `respect` has been
     * invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set &other) {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = direct.size() - 1;

        while(pos && from != to) {
            if(contains(*from)) {
                if(*from != direct[pos]) {
                    swap(direct[pos], *from);
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Clears a sparse set.
     */
    void clear() ENTT_NOEXCEPT {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<std::unique_ptr<entity_type[]>> reverse;
    std::vector<entity_type> direct;
};


}


#endif

// #include "storage.hpp"
#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP


#include <algorithm>
#include <iterator>
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
 * access (either to entities or objects) and when using random or input access
 * iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `raw` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Therefore, many of the functions
 * normally available for non-empty types will not be available for empty ones.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = std::void_t<>>
class storage: public sparse_set<Entity> {
    static_assert(std::is_move_constructible_v<Type>);
    static_assert(std::is_move_assignable_v<Type>);

    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    template<bool Const>
    class storage_iterator final {
        friend class storage<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        storage_iterator(instance_type &ref, const index_type idx) ENTT_NOEXCEPT
            : instances{&ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = std::conditional_t<Const, const value_type *, value_type *>;
        using reference = std::conditional_t<Const, const value_type &, value_type &>;
        using iterator_category = std::random_access_iterator_tag;

        storage_iterator() ENTT_NOEXCEPT = default;

        storage_iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        storage_iterator operator++(int) ENTT_NOEXCEPT {
            storage_iterator orig = *this;
            return operator++(), orig;
        }

        storage_iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        storage_iterator operator--(int) ENTT_NOEXCEPT {
            storage_iterator orig = *this;
            return operator--(), orig;
        }

        storage_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        storage_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            storage_iterator copy = *this;
            return (copy += value);
        }

        storage_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        storage_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const storage_iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*instances)[pos];
        }

        bool operator==(const storage_iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        bool operator!=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const storage_iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const storage_iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*instances)[pos];
        }

        reference operator*() const ENTT_NOEXCEPT {
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
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = storage_iterator<false>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = storage_iterator<true>;

    /**
     * @brief Increases the capacity of a storage.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        underlying_type::reserve(cap);
        instances.reserve(cap);
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
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
     * Random access iterators stay true to the order imposed by a call to
     * either `sort` or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    const_iterator cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator{instances, pos};
    }

    /*! @copydoc cbegin */
    const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    iterator begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator{instances, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Random access iterators stay true to the order imposed by a call to
     * either `sort` or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator cend() const ENTT_NOEXCEPT {
        return const_iterator{instances, {}};
    }

    /*! @copydoc cend */
    const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    iterator end() ENTT_NOEXCEPT {
        return iterator{instances, {}};
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
    const object_type & get(const entity_type entt) const {
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    object_type & get(const entity_type entt) {
        return const_cast<object_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entt A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entt) const {
        return underlying_type::contains(entt) ? (instances.data() + underlying_type::index(entt)) : nullptr;
    }

    /*! @copydoc try_get */
    object_type * try_get(const entity_type entt) {
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
     */
    template<typename... Args>
    void emplace(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<object_type>) {
            instances.push_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::emplace(entt);
    }

    /*! @copydoc emplace */
    template<typename... Args>
    [[deprecated("use ::emplace instead")]]
    void construct(const entity_type entt, Args &&... args) {
        emplace(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given instance.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the object to construct.
     */
    template<typename It>
    void insert(It first, It last, const object_type &value = {}) {
        instances.insert(instances.end(), std::distance(first, last), value);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
    }

    /*! @copydoc insert */
    template<typename It>
    [[deprecated("use ::insert instead")]]
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    construct(It first, It last, const object_type &value = {}) {
        insert(std::move(first), std::move(last), value);
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given range.
     *
     * @sa construct
     *
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of objects.
     * @param to An iterator past the last element of the range of objects.
     */
    template<typename EIt, typename CIt>
    void insert(EIt first, EIt last, CIt from, CIt to) {
        instances.insert(instances.end(), from, to);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
    }

    /*! @copydoc insert */
    template<typename EIt, typename CIt>
    [[deprecated("use ::insert instead")]]
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<EIt>::value_type, entity_type>, void>
    construct(EIt first, EIt last, CIt value) {
        insert(std::move(first), std::move(last), std::move(value));
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
    void erase(const entity_type entt) {
        auto other = std::move(instances.back());
        instances[underlying_type::index(entt)] = std::move(other);
        instances.pop_back();
        underlying_type::erase(entt);
    }

    /*! @copydoc erase */
    [[deprecated("use ::erase instead")]]
    void destroy(const entity_type entt) {
        erase(entt);
    }

    /**
     * @brief Swaps entities and objects in the internal packed arrays.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entities.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    void swap(const entity_type lhs, const entity_type rhs) override {
        std::swap(instances[underlying_type::index(lhs)], instances[underlying_type::index(rhs)]);
        underlying_type::swap(lhs, rhs);
    }

    /**
     * @brief Sort elements according to the given comparison function.
     *
     * Sort the elements so that iterating the range with a couple of iterators
     * returns them in the expected order. See `begin` and `end` for more
     * details.
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
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @warning
     * Empty types are never instantiated. Therefore, only comparison function
     * objects that require to return entities rather than components are
     * accepted.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator first, iterator last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto from = underlying_type::begin() + std::distance(begin(), first);
        const auto to = from + std::distance(first, last);

        const auto apply = [this](const auto lhs, const auto rhs) {
            std::swap(instances[underlying_type::index(lhs)], instances[underlying_type::index(rhs)]);
        };

        if constexpr(std::is_invocable_v<Compare, const object_type &, const object_type &>) {
            underlying_type::arrange(from, to, std::move(apply), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[underlying_type::index(lhs)]), std::as_const(instances[underlying_type::index(rhs)]));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            underlying_type::arrange(from, to, std::move(apply), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
    }

    /*! @brief Clears a storage. */
    void clear() {
        underlying_type::clear();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


/*! @copydoc storage */
template<typename Entity, typename Type>
class storage<Entity, Type, std::enable_if_t<ENTT_IS_EMPTY(Type)>>: public sparse_set<Entity> {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;
    using underlying_type = sparse_set<Entity>;

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Assigns an entity to a storage and constructs its object.
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
     */
    template<typename... Args>
    void emplace(const entity_type entt, Args &&... args) {
        [[maybe_unused]] object_type instance{std::forward<Args>(args)...};
        underlying_type::emplace(entt);
    }

    /*! @copydoc emplace */
    template<typename... Args>
    [[deprecated("use ::emplace instead")]]
    void construct(const entity_type entt, Args &&... args) {
        emplace(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns one or more entities to a storage.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last, const object_type & = {}) {
        underlying_type::insert(first, last);
    }

    /**
     * @copydoc insert
     * @param value An instance of the object to construct.
     */
    template<typename It>
    [[deprecated("use ::insert instead")]]
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    construct(It first, It last, const object_type &value = {}) {
        insert(std::move(first), std::move(last), value);
    }
};


}


#endif

// #include "utility.hpp"
#ifndef ENTT_ENTITY_UTILITY_HPP
#define ENTT_ENTITY_UTILITY_HPP


// #include "../core/type_traits.hpp"



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
inline constexpr exclude_t<Type...> exclude{};


/**
 * @brief Alias for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
struct get_t: type_list<Type...>{};


/**
 * @brief Variable template for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
inline constexpr get_t<Type...> get{};


}


#endif

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


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
 * A non-owning group returns all entities and only the entities that have at
 * least the given components. Moreover, it's guaranteed that the entity list
 * is tightly packed in memory for fast iterations.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.<br/>
 * Moreover, sorting a non-owning group affects all the instances of the same
 * group (it means that users don't have to call `sort` on each instance to sort
 * all of them because they _share_ entities and components).
 *
 * @warning
 * Lifetime of a group must not overcome that of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Type of components observed by the group.
 */
template<typename Entity, typename... Exclude, typename... Get>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>> {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    // we could use pool_type<Type> &..., but vs complains about it and refuses to compile for unknown reasons (most likely a bug)
    basic_group(sparse_set<Entity> &ref, storage<Entity, std::remove_const_t<Get>> &... gpool) ENTT_NOEXCEPT
        : handler{&ref},
          pools{&gpool...}
    {}

    template<typename Func, typename... Weak>
    void traverse(Func func, type_list<Weak...>) const {
        for(const auto entt: *handler) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Weak>({}))...>) {
                func(std::get<pool_type<Weak> *>(pools)->get(entt)...);
            } else {
                func(entt, std::get<pool_type<Weak> *>(pools)->get(entt)...);
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = typename sparse_set<Entity>::iterator;

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
     * @brief Checks whether a group or some pools are empty.
     * @tparam Component Types of components in which one is interested.
     * @return True if the group or the pools are empty, false otherwise.
     */
    template<typename... Component>
    bool empty() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 0) {
            return handler->empty();
        } else {
            return (std::get<pool_type<Component> *>(pools)->empty() && ...);
        }
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
    iterator begin() const ENTT_NOEXCEPT {
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
    iterator end() const ENTT_NOEXCEPT {
        return handler->end();
    }

    /**
     * @brief Returns the first entity that has the given components, if any.
     * @return The first entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given components, if any.
     * @return The last entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator find(const entity_type entt) const {
        const auto it = handler->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return handler->contains(entt);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
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
    decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>({}))...>{get<Component>(entt)...};
        }
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        using get_type_list = type_list_cat_t<std::conditional_t<ENTT_IS_EMPTY(Get), type_list<>, type_list<Get>>...>;
        traverse(std::move(func), get_type_list{});
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    [[deprecated("use ::each instead")]]
    void less(Func func) const {
        each(std::move(func));
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
     * bool(std::tuple<Component &...>, std::tuple<Component &...>);
     * bool(const Component &..., const Component &...);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Component` are such that they are iterated by the group.<br/>
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
        if constexpr(sizeof...(Component) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>);
            handler->sort(handler->begin(), handler->end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }  else if constexpr(sizeof...(Component) == 1) {
            handler->sort(handler->begin(), handler->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare((std::get<pool_type<Component> *>(pools)->get(lhs), ...), (std::get<pool_type<Component> *>(pools)->get(rhs), ...));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            handler->sort(handler->begin(), handler->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare(std::tuple<decltype(get<Component>({}))...>{std::get<pool_type<Component> *>(pools)->get(lhs)...}, std::tuple<decltype(get<Component>({}))...>{std::get<pool_type<Component> *>(pools)->get(rhs)...});
            }, std::move(algo), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Non-owning groups of the same type share with the registry a pool of
     * entities with its own order that doesn't depend on the order of any pool
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
 * Owning groups return all entities and only the entities that have at least
 * the given components. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that the lists of owned components are tightly packed in
 *   memory for even faster iterations and to allow direct access.
 * * They stay true to the order of the owned components and all instances have
 *   the same order in memory.
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
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
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
 * Lifetime of a group must not overcome that of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Types of components observed by the group.
 * @tparam Owned Types of components owned by the group.
 */
template<typename Entity, typename... Exclude, typename... Get, typename... Owned>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    template<typename Component>
    using component_iterator = decltype(std::declval<pool_type<Component>>().begin());

    // we could use pool_type<Type> &..., but vs complains about it and refuses to compile for unknown reasons (most likely a bug)
    basic_group(const std::size_t &ref, const std::size_t &extent, storage<Entity, std::remove_const_t<Owned>> &... opool, storage<Entity, std::remove_const_t<Get>> &... gpool) ENTT_NOEXCEPT
        : pools{&opool..., &gpool...},
          length{&extent},
          super{&ref}
    {}

    template<typename Func, typename... Strong, typename... Weak>
    void traverse(Func func, type_list<Strong...>, type_list<Weak...>) const {
        [[maybe_unused]] auto it = std::make_tuple((std::get<pool_type<Strong> *>(pools)->end() - *length)...);
        [[maybe_unused]] auto data = std::get<0>(pools)->sparse_set<entity_type>::end() - *length;

        for(auto next = *length; next; --next) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Strong>({}))..., decltype(get<Weak>({}))...>) {
                if constexpr(sizeof...(Weak) == 0) {
                    func(*(std::get<component_iterator<Strong>>(it)++)...);
                } else {
                    const auto entt = *(data++);
                    func(*(std::get<component_iterator<Strong>>(it)++)..., std::get<pool_type<Weak> *>(pools)->get(entt)...);
                }
            } else {
                const auto entt = *(data++);
                func(entt, *(std::get<component_iterator<Strong>>(it)++)..., std::get<pool_type<Weak> *>(pools)->get(entt)...);
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = typename sparse_set<Entity>::iterator;

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
     * @brief Checks whether a group or some pools are empty.
     * @tparam Component Types of components in which one is interested.
     * @return True if the group or the pools are empty, false otherwise.
     */
    template<typename... Component>
    bool empty() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 0) {
            return !*length;
        } else {
            return (std::get<pool_type<Component> *>(pools)->empty() && ...);
        }
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
    iterator begin() const ENTT_NOEXCEPT {
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
    iterator end() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->sparse_set<entity_type>::end();
    }

    /**
     * @brief Returns the first entity that has the given components, if any.
     * @return The first entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given components, if any.
     * @return The last entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator find(const entity_type entt) const {
        const auto it = std::get<0>(pools)->find(entt);
        return it != end() && it >= begin() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return std::get<0>(pools)->contains(entt) && (std::get<0>(pools)->index(entt) < (*length));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
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
    decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>({}))...>{get<Component>(entt)...};
        }
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        using owned_type_list = type_list_cat_t<std::conditional_t<ENTT_IS_EMPTY(Owned), type_list<>, type_list<Owned>>...>;
        using get_type_list = type_list_cat_t<std::conditional_t<ENTT_IS_EMPTY(Get), type_list<>, type_list<Get>>...>;
        traverse(std::move(func), owned_type_list{}, get_type_list{});
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    [[deprecated("use ::each instead")]]
    void less(Func func) const {
        each(std::move(func));
    }

    /**
     * @brief Checks whether the group can be sorted.
     * @return True if the group can be sorted, false otherwise.
     */
    bool sortable() const ENTT_NOEXCEPT {
        constexpr auto size = sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude);
        return *super == size;
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
     * bool(std::tuple<Component &...>, std::tuple<Component &...>);
     * bool(const Component &, const Component &);
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
        ENTT_ASSERT(sortable());
        auto *cpool = std::get<0>(pools);

        if constexpr(sizeof...(Component) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>);
            cpool->sort(cpool->end()-*length, cpool->end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else if constexpr(sizeof...(Component) == 1) {
            cpool->sort(cpool->end()-*length, cpool->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare((std::get<pool_type<Component> *>(pools)->get(lhs), ...), (std::get<pool_type<Component> *>(pools)->get(rhs), ...));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool->sort(cpool->end()-*length, cpool->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare(std::tuple<decltype(get<Component>({}))...>{std::get<pool_type<Component> *>(pools)->get(lhs)...}, std::tuple<decltype(get<Component>({}))...>{std::get<pool_type<Component> *>(pools)->get(rhs)...});
            }, std::move(algo), std::forward<Args>(args)...);
        }

        [this](auto *head, auto *... other) {
            for(auto next = *length; next; --next) {
                const auto pos = next - 1;
                [[maybe_unused]] const auto entt = head->data()[pos];
                (other->swap(other->data()[pos], entt), ...);
            }
        }(std::get<pool_type<Owned> *>(pools)...);
    }

private:
    const std::tuple<pool_type<Owned> *..., pool_type<Get> *...> pools;
    const size_type *length;
    const size_type *super;
};


}


#endif

// #include "runtime_view.hpp"
#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP


#include <iterator>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"

// #include "sparse_set.hpp"

// #include "fwd.hpp"



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
 * * The entity currently pointed is destroyed.
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
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_runtime_view {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using underlying_iterator = typename sparse_set<Entity>::iterator;

    class view_iterator final {
        friend class basic_runtime_view<Entity>;

        using direct_type = std::vector<const sparse_set<Entity> *>;

        view_iterator(const direct_type &all, underlying_iterator curr) ENTT_NOEXCEPT
            : pools{&all},
              it{curr}
        {
            if(it != (*pools)[0]->end() && !valid()) {
                ++(*this);
            }
        }

        bool valid() const {
            return std::all_of(pools->begin()++, pools->end(), [entt = *it](const auto *curr) {
                return curr->contains(entt);
            });
        }

    public:
        using difference_type = typename underlying_iterator::difference_type;
        using value_type = typename underlying_iterator::value_type;
        using pointer = typename underlying_iterator::pointer;
        using reference = typename underlying_iterator::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() ENTT_NOEXCEPT = default;

        view_iterator & operator++() {
            while(++it != (*pools)[0]->end() && !valid());
            return *this;
        }

        view_iterator operator++(int) {
            view_iterator orig = *this;
            return operator++(), orig;
        }

        view_iterator & operator--() ENTT_NOEXCEPT {
            while(--it != (*pools)[0]->begin() && !valid());
            return *this;
        }

        view_iterator operator--(int) ENTT_NOEXCEPT {
            view_iterator orig = *this;
            return operator--(), orig;
        }

        bool operator==(const view_iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        bool operator!=(const view_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const {
            return it.operator->();
        }

        reference operator*() const {
            return *operator->();
        }

    private:
        const direct_type *pools;
        underlying_iterator it;
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

    bool valid() const {
        return !pools.empty() && pools.front();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = view_iterator;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const {
        return valid() ? pools.front()->size() : size_type{};
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const {
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
    iterator begin() const {
        iterator it{};

        if(valid()) {
            it = { pools, pools[0]->begin() };
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
    iterator end() const {
        iterator it{};

        if(valid()) {
            it = { pools, pools[0]->end() };
        }

        return it;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return valid() && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *view) {
            return view->find(entt) != view->end();
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
        for(const auto entity: *this) {
            func(entity);
        }
    }

private:
    std::vector<const sparse_set<Entity> *> pools;
};


}


#endif

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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    basic_snapshot(const basic_registry<Entity> *source, Entity init, follow_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          seed{init},
          follow{fn}
    {}

    template<typename Component, typename Archive, typename It>
    void get(Archive &archive, std::size_t sz, It first, It last) const {
        archive(typename traits_type::entity_type(sz));

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
        archive(typename traits_type::entity_type(reg->alive()));
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
        archive(typename traits_type::entity_type(size));

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
        (component<Component>(archive, reg->template data<Component>(), reg->template data<Component>() + reg->template size<Component>()), ...);
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
        component<Component...>(archive, first, last, std::index_sequence_for<Component...>{});
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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    basic_snapshot_loader(basic_registry<Entity> *source, force_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          force{fn}
    {
        // to restore a snapshot as a whole requires a clean registry
        ENTT_ASSERT(reg->empty());
    }

    template<typename Archive>
    void assure(Archive &archive, bool discard) const {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            Entity entt{};
            archive(entt);
            force(*reg, entt, discard);
        }
    }

    template<typename Type, typename Archive, typename... Args>
    void assign(Archive &archive, Args... args) const {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            static constexpr auto discard = false;
            Entity entt{};

            if constexpr(std::is_empty_v<Type>) {
                archive(entt);
                force(*reg, entt, discard);
                reg->template emplace<Type>(args..., entt);
            } else {
                Type instance{};
                archive(entt, instance);
                force(*reg, entt, discard);
                reg->template emplace<Type>(args..., entt, std::as_const(instance));
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
        static constexpr auto discard = false;
        assure(archive, discard);
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
        static constexpr auto discard = true;
        assure(archive, discard);
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
 * (possibly) non-empty destination. The loader can accommodate in a registry
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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

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

    template<typename Container>
    auto update(int, Container &container)
    -> decltype(typename Container::mapped_type{}, void()) {
        // map like container
        Container other;

        for(auto &&pair: container) {
            using first_type = std::remove_const_t<typename std::decay_t<decltype(pair)>::first_type>;
            using second_type = typename std::decay_t<decltype(pair)>::second_type;

            if constexpr(std::is_same_v<first_type, Entity> && std::is_same_v<second_type, Entity>) {
                other.emplace(map(pair.first), map(pair.second));
            } else if constexpr(std::is_same_v<first_type, Entity>) {
                other.emplace(map(pair.first), std::move(pair.second));
            } else {
                static_assert(std::is_same_v<second_type, Entity>);
                other.emplace(std::move(pair.first), map(pair.second));
            }
        }

        std::swap(container, other);
    }

    template<typename Container>
    auto update(char, Container &container)
    -> decltype(typename Container::value_type{}, void()) {
        // vector like container
        static_assert(std::is_same_v<typename Container::value_type, Entity>);

        for(auto &&entt: container) {
            entt = map(entt);
        }
    }

    template<typename Other, typename Type, typename Member>
    void update([[maybe_unused]] Other &instance, [[maybe_unused]] Member Type:: *member) {
        if constexpr(!std::is_same_v<Other, Type>) {
            return;
        } else if constexpr(std::is_same_v<Member, Entity>) {
            instance.*member = map(instance.*member);
        } else {
            // maybe a container? let's try...
            update(0, instance.*member);
        }
    }

    template<typename Archive>
    void assure(Archive &archive, void(basic_continuous_loader:: *member)(Entity)) {
        typename traits_type::entity_type length{};
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
                reg->template remove_if_exists<Component>(local);
            }
        }
    }

    template<typename Other, typename Archive, typename... Type, typename... Member>
    void assign(Archive &archive, [[maybe_unused]] Member Type:: *... member) {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            Entity entt{};

            if constexpr(std::is_empty_v<Other>) {
                archive(entt);
                restore(entt);
                reg->template emplace_or_replace<Other>(map(entt));
            } else {
                Other instance{};
                archive(entt, instance);
                (update(instance, member), ...);
                restore(entt);
                reg->template emplace_or_replace<Other>(map(entt), std::as_const(instance));
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
    bool contains(entity_type entt) const ENTT_NOEXCEPT {
        return (remloc.find(entt) != remloc.cend());
    }

    /*! @copydoc contains */
    [[deprecated("use ::contains instead")]]
    bool has(entity_type entt) const ENTT_NOEXCEPT {
        return contains(entt);
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
    std::unordered_map<entity_type, std::pair<entity_type, bool>> remloc;
    basic_registry<entity_type> *reg;
};


}


#endif

// #include "sparse_set.hpp"

// #include "storage.hpp"

// #include "utility.hpp"

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

// #include "utility.hpp"

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_view;


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and uses the
 * smallest set in order to get a performance boost when iterate.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the view.
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Exclude, typename... Component>
class basic_view<Entity, exclude_t<Exclude...>, Component...> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    template<typename Comp>
    using pool_type = std::conditional_t<std::is_const_v<Comp>, const storage<Entity, std::remove_const_t<Comp>>, storage<Entity, Comp>>;

    template<typename Comp>
    using component_iterator = decltype(std::declval<pool_type<Comp>>().begin());

    using underlying_iterator = typename sparse_set<Entity>::iterator;
    using unchecked_type = std::array<const sparse_set<Entity> *, (sizeof...(Component) - 1)>;
    using filter_type = std::array<const sparse_set<Entity> *, sizeof...(Exclude)>;

    class view_iterator final {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        view_iterator(const sparse_set<Entity> &candidate, unchecked_type other, filter_type ignore, underlying_iterator curr) ENTT_NOEXCEPT
            : view{&candidate},
              unchecked{other},
              filter{ignore},
              it{curr}
        {
            if(it != view->end() && !valid()) {
                ++(*this);
            }
        }

        bool valid() const {
            return std::all_of(unchecked.cbegin(), unchecked.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->contains(entt); })
                    && std::none_of(filter.cbegin(), filter.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->contains(entt); });
        }

    public:
        using difference_type = typename underlying_iterator::difference_type;
        using value_type = typename underlying_iterator::value_type;
        using pointer = typename underlying_iterator::pointer;
        using reference = typename underlying_iterator::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() ENTT_NOEXCEPT = default;

        view_iterator & operator++() {
            while(++it != view->end() && !valid());
            return *this;
        }

        view_iterator operator++(int) {
            view_iterator orig = *this;
            return operator++(), orig;
        }

        view_iterator & operator--() ENTT_NOEXCEPT {
            while(--it != view->begin() && !valid());
            return *this;
        }

        view_iterator operator--(int) ENTT_NOEXCEPT {
            view_iterator orig = *this;
            return operator--(), orig;
        }

        bool operator==(const view_iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        bool operator!=(const view_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const {
            return it.operator->();
        }

        reference operator*() const {
            return *operator->();
        }

    private:
        const sparse_set<Entity> *view;
        unchecked_type unchecked;
        filter_type filter;
        underlying_iterator it;
    };

    // we could use pool_type<Component> &..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_view(storage<Entity, std::remove_const_t<Component>> &... component, storage<Entity, std::remove_const_t<Exclude>> &... epool) ENTT_NOEXCEPT
        : pools{&component..., &epool...}
    {}

    const sparse_set<Entity> & candidate() const ENTT_NOEXCEPT {
        return *std::min({ static_cast<const sparse_set<Entity> *>(std::get<pool_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    unchecked_type unchecked(const sparse_set<Entity> &view) const {
        std::size_t pos{};
        unchecked_type other{};
        ((std::get<pool_type<Component> *>(pools) == &view ? nullptr : (other[pos++] = std::get<pool_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename Other>
    decltype(auto) get([[maybe_unused]] component_iterator<Comp> it, [[maybe_unused]] pool_type<Other> *cpool, [[maybe_unused]] const Entity entt) const {
        if constexpr(std::is_same_v<Comp, Other>) {
            return *it;
        } else {
            return cpool->get(entt);
        }
    }

    template<typename Comp, typename Func, typename... Type>
    void traverse(Func func, type_list<Type...>) const {
        if constexpr(std::disjunction_v<std::is_same<Comp, Type>...>) {
            auto it = std::get<pool_type<Comp> *>(pools)->begin();

            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                auto curr = it++;

                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...) && (!std::get<pool_type<Exclude> *>(pools)->contains(entt) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    } else {
                        func(entt, get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    }
                }
            }
        } else {
            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...) && (!std::get<pool_type<Exclude> *>(pools)->contains(entt) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(std::get<pool_type<Type> *>(pools)->get(entt)...);
                    } else {
                        func(entt, std::get<pool_type<Type> *>(pools)->get(entt)...);
                    }
                }
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = view_iterator;

    /**
     * @brief Returns the number of existing components of the given type.
     *
     * This isn't the number of entities iterated by the view.
     *
     * @tparam Comp Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Comp>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->size();
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    size_type size() const ENTT_NOEXCEPT {
        return std::min({ std::get<pool_type<Component> *>(pools)->size()... });
    }

    /**
     * @brief Checks whether a view or some pools are empty.
     *
     * The view is definitely empty if one of the pools it uses is empty. In all
     * other cases, the view may be empty and not return entities even if this
     * function returns false.
     *
     * @tparam Comp Types of components in which one is interested.
     * @return True if the view or the pools are empty, false otherwise.
     */
    template<typename... Comp>
    bool empty() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Comp) == 0) {
            return (std::get<pool_type<Component> *>(pools)->empty() || ...);
        } else {
            return (std::get<pool_type<Comp> *>(pools)->empty() && ...);
        }
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
    iterator begin() const {
        const auto &view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        return iterator{view, unchecked(view), ignore, view.begin()};
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
    iterator end() const {
        const auto &view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        return iterator{view, unchecked(view), ignore, view.end()};
    }

    /**
     * @brief Returns the first entity that has the given components, if any.
     * @return The first entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given components, if any.
     * @return The last entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator find(const entity_type entt) const {
        const auto &view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        iterator it{view, unchecked(view), ignore, view.find(entt)};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return (std::get<pool_type<Component> *>(pools)->contains(entt) && ...)
                && (!std::get<pool_type<Exclude> *>(pools)->contains(entt) && ...);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
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
    decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 0) {
            static_assert(sizeof...(Component) == 1);
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<pool_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Comp>({}))...>{get<Comp>(entt)...};
        }
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        const auto &view = candidate();
        ((std::get<pool_type<Component> *>(pools) == &view ? each<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
    void each(Func func) const {
        using non_empty_type = type_list_cat_t<std::conditional_t<ENTT_IS_EMPTY(Component), type_list<>, type_list<Component>>...>;
        traverse<Comp>(std::move(func), non_empty_type{});
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
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    [[deprecated("use ::each instead")]]
    void less(Func func) const {
        each(std::move(func));
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @sa less
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    [[deprecated("use ::each instead")]]
    void less(Func func) const {
        each<Comp>(std::move(func));
    }

private:
    const std::tuple<pool_type<Component> *..., pool_type<Exclude> *...> pools;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pool iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class basic_view<Entity, exclude_t<>, Component> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    basic_view(pool_type &ref) ENTT_NOEXCEPT
        : pool{&ref}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = Component;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = typename sparse_set<Entity>::iterator;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether a view is empty.
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
    iterator begin() const ENTT_NOEXCEPT {
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
    iterator end() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::end();
    }

    /**
     * @brief Returns the first entity that has the given component, if any.
     * @return The first entity that has the given component if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given component, if any.
     * @return The last entity that has the given component if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator find(const entity_type entt) const {
        const auto it = pool->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return pool->contains(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
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
    template<typename Comp = Component>
    decltype(auto) get(const entity_type entt) const {
        static_assert(std::is_same_v<Comp, Component>);
        ENTT_ASSERT(contains(entt));
        return pool->get(entt);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to the component if it's a non-empty one.
     * The _constness_ of the component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if constexpr(ENTT_IS_EMPTY(Component)) {
            if constexpr(std::is_invocable_v<Func>) {
                for(auto pos = pool->size(); pos; --pos) {
                    func();
                }
            } else {
                for(const auto entt: *this) {
                    func(entt);
                }
            }
        } else {
            if constexpr(std::is_invocable_v<Func, decltype(get({}))>) {
                for(auto &&component: *pool) {
                    func(component);
                }
            } else {
                auto raw = pool->begin();

                for(const auto entt: *this) {
                    func(entt, *(raw++));
                }
            }
        }
    }

    /*! @copydoc each */
    template<typename Func>
    [[deprecated("use ::each instead")]]
    void less(Func func) const {
        each(std::move(func));
    }

private:
    pool_type *pool;
};


}


#endif



namespace entt {


/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components, then create
 * views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_registry {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    template<typename Component>
    struct pool_handler final: storage<Entity, Component> {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>);
        std::size_t super{};

        auto on_construct() ENTT_NOEXCEPT {
            return sink{construction};
        }

        auto on_update() ENTT_NOEXCEPT {
            return sink{update};
        }

        auto on_destroy() ENTT_NOEXCEPT {
            return sink{destruction};
        }

        template<typename... Args>
        decltype(auto) emplace(basic_registry &owner, const Entity entt, Args &&... args) {
            storage<entity_type, Component>::emplace(entt, std::forward<Args>(args)...);
            construction.publish(owner, entt);

            if constexpr(!ENTT_IS_EMPTY(Component)) {
                return this->get(entt);
            }
        }

        template<typename It, typename... Args>
        void insert(basic_registry &owner, It first, It last, Args &&... args) {
            storage<entity_type, Component>::insert(first, last, std::forward<Args>(args)...);

            if(!construction.empty()) {
                while(first != last) { construction.publish(owner, *(first++)); }
            }
        }

        void remove(basic_registry &owner, const Entity entt) {
            destruction.publish(owner, entt);
            this->erase(entt);
        }

        template<typename It>
        void remove(basic_registry &owner, It first, It last) {
            if(std::distance(first, last) == std::distance(this->begin(), this->end())) {
                if(!destruction.empty()) {
                    while(first != last) { destruction.publish(owner, *(first++)); }
                }

                this->clear();
            } else {
                while(first != last) { this->remove(owner, *(first++)); }
            }
        }

        template<typename... Func>
        decltype(auto) patch(basic_registry &owner, const Entity entt, Func &&... func) {
            (std::forward<Func>(func)(this->get(entt)), ...);
            update.publish(owner, entt);

            if constexpr(!ENTT_IS_EMPTY(Component)) {
                return this->get(entt);
            }
        }

        decltype(auto) replace(basic_registry &owner, const Entity entt, [[maybe_unused]] Component component) {
            if constexpr(ENTT_IS_EMPTY(Component)) {
                return patch(owner, entt);
            } else {
                return patch(owner, entt, [&component](auto &&curr) { curr = std::move(component); });
            }
        }

    private:
        sigh<void(basic_registry &, const Entity)> construction{};
        sigh<void(basic_registry &, const Entity)> destruction{};
        sigh<void(basic_registry &, const Entity)> update{};
    };

    struct pool_data {
        id_type type_id{};
        std::unique_ptr<sparse_set<Entity>> pool{};
        void(* remove)(sparse_set<Entity> &, basic_registry &, const Entity){};
    };

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...> {
        static_assert(std::conjunction_v<std::is_same<Owned, std::decay_t<Owned>>..., std::is_same<Get, std::decay_t<Get>>..., std::is_same<Exclude, std::decay_t<Exclude>>...>);
        std::conditional_t<sizeof...(Owned) == 0, sparse_set<Entity>, std::size_t> current{};

        template<typename Component>
        void maybe_valid_if(basic_registry &owner, const Entity entt) {
            static_assert(std::disjunction_v<std::is_same<Owned, std::decay_t<Owned>>..., std::is_same<Get, std::decay_t<Get>>..., std::is_same<Exclude, std::decay_t<Exclude>>...>);
            [[maybe_unused]] const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...);

            const auto is_valid = ((std::is_same_v<Component, Owned> || std::get<pool_handler<Owned> &>(cpools).contains(entt)) && ...)
                    && ((std::is_same_v<Component, Get> || owner.assure<Get>().contains(entt)) && ...)
                    && ((std::is_same_v<Component, Exclude> || !owner.assure<Exclude>().contains(entt)) && ...);

            if constexpr(sizeof...(Owned) == 0) {
                if(is_valid && !current.contains(entt)) {
                    current.emplace(entt);
                }
            } else {
                if(is_valid && !(std::get<0>(cpools).index(entt) < current)) {
                    const auto pos = current++;
                    (std::get<pool_handler<Owned> &>(cpools).swap(std::get<pool_handler<Owned> &>(cpools).data()[pos], entt), ...);
                }
            }
        }

        void discard_if([[maybe_unused]] basic_registry &owner, const Entity entt) {
            if constexpr(sizeof...(Owned) == 0) {
                if(current.contains(entt)) {
                    current.erase(entt);
                }
            } else {
                if(const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...); std::get<0>(cpools).contains(entt) && (std::get<0>(cpools).index(entt) < current)) {
                    const auto pos = --current;
                    (std::get<pool_handler<Owned> &>(cpools).swap(std::get<pool_handler<Owned> &>(cpools).data()[pos], entt), ...);
                }
            }
        }
    };

    struct group_data {
        std::size_t size;
        std::unique_ptr<void, void(*)(void *)> group;
        bool (* owned)(const id_type) ENTT_NOEXCEPT;
        bool (* get)(const id_type) ENTT_NOEXCEPT;
        bool (* exclude)(const id_type) ENTT_NOEXCEPT;
    };

    struct variable_data {
        id_type type_id;
        std::unique_ptr<void, void(*)(void *)> value;
    };

    template<typename Component>
    const pool_handler<Component> & assure() const {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>);

        if constexpr(has_type_index_v<Component>) {
            const auto index = type_index<Component>::value();

            if(!(index < pools.size())) {
                pools.resize(index+1);
            }

            if(auto &&pdata = pools[index]; !pdata.pool) {
                pdata.type_id = type_info<Component>::id();
                pdata.pool.reset(new pool_handler<Component>());
                pdata.remove = [](sparse_set<entity_type> &cpool, basic_registry &owner, const entity_type entt) {
                    static_cast<pool_handler<Component> &>(cpool).remove(owner, entt);
                };
            }

            return static_cast<pool_handler<Component> &>(*pools[index].pool);
        } else {
            sparse_set<entity_type> *candidate{nullptr};

            if(auto it = std::find_if(pools.begin(), pools.end(), [id = type_info<Component>::id()](const auto &pdata) { return id == pdata.type_id; }); it == pools.cend()) {
                candidate = pools.emplace_back(pool_data{
                    type_info<Component>::id(),
                    std::unique_ptr<sparse_set<entity_type>>{new pool_handler<Component>()},
                    [](sparse_set<entity_type> &cpool, basic_registry &owner, const entity_type entt) {
                        static_cast<pool_handler<Component> &>(cpool).remove(owner, entt);
                    }
                }).pool.get();
            } else {
                candidate = it->pool.get();
            }

            return static_cast<pool_handler<Component> &>(*candidate);
        }
    }

    template<typename Component>
    pool_handler<Component> & assure() {
        return const_cast<pool_handler<Component> &>(std::as_const(*this).template assure<Component>());
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    basic_registry() = default;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;

    /**
     * @brief Prepares a pool for the given type if required.
     * @tparam Component Type of component for which to prepare a pool.
     */
    template<typename Component>
    void prepare() {
        assure<Component>();
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const {
        return assure<Component>().size();
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
    size_type alive() const {
        auto sz = entities.size();
        auto curr = destroyed;

        for(; curr != null; --sz) {
            curr = entities[to_integral(curr) & traits_type::entity_mask];
        }

        return sz;
    }

    /**
     * @brief Increases the capacity of the registry or of the pools for the
     * given components.
     *
     * If no components are specified, the capacity of the registry is
     * increased, that is the number of entities it contains. Otherwise the
     * capacity of the pools for the given components is increased.<br/>
     * In both cases, if the new capacity is greater than the current capacity,
     * new storage is allocated, otherwise the method does nothing.
     *
     * @tparam Component Types of components for which to reserve storage.
     * @param cap Desired capacity.
     */
    template<typename... Component>
    void reserve(const size_type cap) {
        if constexpr(sizeof...(Component) == 0) {
            entities.reserve(cap);
        } else {
            (assure<Component>().reserve(cap), ...);
        }
    }

    /**
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    size_type capacity() const {
        return assure<Component>().capacity();
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
     * @brief Requests the removal of unused capacity for the given components.
     * @tparam Component Types of components for which to reclaim unused
     * capacity.
     */
    template<typename... Component>
    void shrink_to_fit() {
        (assure<Component>().shrink_to_fit(), ...);
    }

    /**
     * @brief Checks whether the registry or the pools of the given components
     * are empty.
     *
     * A registry is considered empty when it doesn't contain entities that are
     * still in use.
     *
     * @tparam Component Types of components in which one is interested.
     * @return True if the registry or the pools of the given components are
     * empty, false otherwise.
     */
    template<typename... Component>
    bool empty() const {
        if constexpr(sizeof...(Component) == 0) {
            return !alive();
        } else {
            return (assure<Component>().empty() && ...);
        }
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
     * Empty components aren't explicitly instantiated. Therefore, this function
     * isn't available for them. A compilation error will occur if invoked.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    const Component * raw() const {
        return assure<Component>().raw();
    }

    /*! @copydoc raw */
    template<typename Component>
    Component * raw() {
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
    const entity_type * data() const {
        return assure<Component>().data();
    }

    /**
     * @brief Direct access to the list of entities of a registry.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @warning
     * This list contains both valid and destroyed entities and isn't suitable
     * for direct use.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return entities.data();
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    bool valid(const entity_type entity) const {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integral(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(to_integral(entity) >> traits_type::entity_shift);
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
    version_type current(const entity_type entity) const {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        ENTT_ASSERT(pos < entities.size());
        return version_type(to_integral(entities[pos]) >> traits_type::entity_shift);
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of possible entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * @return A valid entity identifier.
     */
    entity_type create() {
        entity_type entt;

        if(destroyed == null) {
            entt = entities.emplace_back(entity_type(entities.size()));
            // traits_type::entity_mask is reserved to allow for null identifiers
            ENTT_ASSERT(to_integral(entt) < traits_type::entity_mask);
        } else {
            const auto curr = to_integral(destroyed);
            const auto version = to_integral(entities[curr]) & (traits_type::version_mask << traits_type::entity_shift);
            destroyed = entity_type{to_integral(entities[curr]) & traits_type::entity_mask};
            entt = entities[curr] = entity_type{curr | version};
        }

        return entt;
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * @sa create
     *
     * If the requested entity isn't in use, the suggested identifier is created
     * and returned. Otherwise, a new one will be generated for this purpose.
     *
     * @param hint A desired entity identifier.
     * @return A valid entity identifier.
     */
    entity_type create(const entity_type hint) {
        ENTT_ASSERT(hint != null);
        entity_type entt;

        if(const auto req = (to_integral(hint) & traits_type::entity_mask); !(req < entities.size())) {
            entities.reserve(req + 1);

            for(auto pos = entities.size(); pos < req; ++pos) {
                entities.emplace_back(destroyed);
                destroyed = entity_type(pos);
            }

            entt = entities.emplace_back(hint);
        } else if(const auto curr = (to_integral(entities[req]) & traits_type::entity_mask); req == curr) {
            entt = create();
        } else {
            auto *it = &destroyed;
            for(; (to_integral(*it) & traits_type::entity_mask) != req; it = &entities[to_integral(*it) & traits_type::entity_mask]);
            *it = entity_type{curr | (to_integral(*it) & (traits_type::version_mask << traits_type::entity_shift))};
            entt = entities[req] = hint;
        }

        return entt;
    }

    /**
     * @brief Assigns each element in a range an entity.
     *
     * @sa create
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void create(It first, It last) {
        std::generate(first, last, [this]() { return create(); });
    }

    /**
     * @brief Assigns entities to an empty registry.
     *
     * This function is intended for use in conjunction with `raw`.<br/>
     * Don't try to inject ranges of randomly generated entities. There is no
     * guarantee that a registry will continue to work properly in this case.
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode if all
     * pools aren't empty.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void assign(It first, It last) {
        ENTT_ASSERT(std::all_of(pools.cbegin(), pools.cend(), [](auto &&pdata) { return !pdata.pool || pdata.pool->empty(); }));
        entities.assign(first, last);
        destroyed = null;

        for(std::size_t pos{}, end = entities.size(); pos < end; ++pos) {
            if((to_integral(entities[pos]) & traits_type::entity_mask) != pos) {
                const auto version = to_integral(entities[pos]) & (traits_type::version_mask << traits_type::entity_shift);
                entities[pos] = entity_type{to_integral(destroyed) | version};
                destroyed = entity_type(pos);
            }
        }
    }

    /**
     * @brief Destroys an entity and lets the registry recycle the identifier.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        remove_all(entity);
        // lengthens the implicit list of destroyed entities
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        const auto version = ((to_integral(entity) >> traits_type::entity_shift) + 1) << traits_type::entity_shift;
        entities[entt] = entity_type{to_integral(destroyed) | version};
        destroyed = entity_type{entt};
    }

    /**
     * @brief Destroys all the entities in a range.
     *
     * @sa destroy
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void destroy(It first, It last) {
        while(first != last) { destroy(*(first++)); }
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
    decltype(auto) emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>().emplace(*this, entity, std::forward<Args>(args)...);
    }

    /*! @copydoc emplace */
    template<typename Component, typename... Args>
    [[deprecated("use ::emplace instead")]]
    decltype(auto) assign(const entity_type entity, Args &&... args) {
        return emplace<Component>(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns each entity in a range the given component.
     *
     * @sa emplace
     *
     * @tparam Component Type of component to create.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the component to assign.
     */
    template<typename Component, typename It>
    void insert(It first, It last, const Component &value = {}) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().insert(*this, first, last, value);
    }

    /*! @copydoc insert */
    template<typename Component, typename It>
    [[deprecated("use ::insert instead")]]
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    assign(It first, It last, const Component &value = {}) {
        return insert(std::move(first), std::move(last), value);
    }

    /**
     * @brief Assigns each entity in a range the given components.
     *
     * @sa emplace
     *
     * @tparam Component Type of component to create.
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of components.
     * @param to An iterator past the last element of the range of components.
     */
    template<typename Component, typename EIt, typename CIt>
    void insert(EIt first, EIt last, CIt from, CIt to) {
        static_assert(std::is_constructible_v<Component, typename std::iterator_traits<CIt>::value_type>);
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().insert(*this, first, last, from, to);
    }

    /*! @copydoc insert */
    template<typename Component, typename EIt, typename CIt>
    [[deprecated("use ::insert instead")]]
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<EIt>::value_type, entity_type>, void>
    assign(EIt first, EIt last, CIt value) {
        return insert<Component>(std::move(first), std::move(last), value, value + std::distance(first, last));
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.emplace<Component>(entity, args...);
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
    decltype(auto) emplace_or_replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();

        return cpool.contains(entity)
                ? cpool.replace(*this, entity, Component{std::forward<Args>(args)...})
                : cpool.emplace(*this, entity, std::forward<Args>(args)...);
    }

    /*! @copydoc emplace_or_replace */
    template<typename Component, typename... Args>
    [[deprecated("use ::emplace_or_replace instead")]]
    decltype(auto) assign_or_replace(const entity_type entity, Args &&... args) {
        return emplace_or_replace<Component>(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given component for an entity.
     *
     * The signature of the functions should be equivalent to the following:
     *
     * @code{.cpp}
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned. However, this function can be used to trigger an update signal
     * for them.
     *
     * @warning
     * Attempting to use an invalid entity or to patch a component of an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched component.
     */
    template<typename Component, typename... Func>
    decltype(auto) patch(const entity_type entity, Func &&... func) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>().patch(*this, entity, std::forward<Func>(func)...);
    }

    /*! @copydoc patch */
    template<typename Component, typename... Func>
    [[deprecated("use registry::patch instead")]]
    auto replace(const entity_type entity, Func &&... func)
    -> decltype((func(std::declval<Component &>()), ...), assign<Component>(entity)) {
        return patch<Component>(entity, std::forward<Func>(func)...);
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
     * @return A reference to the component being replaced.
     */
    template<typename Component, typename... Args>
    auto replace(const entity_type entity, Args &&... args)
    -> decltype(std::enable_if_t<sizeof...(Args) != 0>(), Component{std::forward<Args>(args)...}, assure<Component>().get(entity)) {
        return assure<Component>().replace(*this, entity, Component{std::forward<Args>(args)...});
    }

    /**
     * @brief Removes the given components from an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to remove a component from an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Types of components to remove.
     * @param entity A valid entity identifier.
     */
    template<typename... Component>
    void remove(const entity_type entity) {
        ENTT_ASSERT(valid(entity));
        (assure<Component>().remove(*this, entity), ...);
    }

    /**
     * @brief Removes the given components from all the entities in a range.
     *
     * @see remove
     *
     * @tparam Component Types of components to remove.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename... Component, typename It>
    void remove(It first, It last) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        (assure<Component>().remove(*this, first, last), ...);
    }

    /**
     * @brief Removes the given components from an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * if(registry.has<Component>(entity)) { registry.remove<Component>(entity) }
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Types of components to remove.
     * @param entity A valid entity identifier.
     */
    template<typename... Component>
    void remove_if_exists(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        ([this, entity](auto &&cpool) {
            if(cpool.contains(entity)) {
                cpool.remove(*this, entity);
            }
        }(assure<Component>()), ...);
    }

    /**
     * @brief Removes all the components from an entity and makes it orphaned.
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
    void remove_all(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool && pdata.pool->contains(entity)) {
                pdata.remove(*pdata.pool, *this, entity);
            }
        }

        // just a way to protect users from listeners that attach components
        ENTT_ASSERT(orphan(entity));
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
    bool has(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().contains(entity) && ...);
    }

    /**
     * @brief Checks if an entity has at least one of the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has at least one of the given components,
     * false otherwise.
     */
    template<typename... Component>
    bool any(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().contains(entity) || ...);
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
            return (assure<Component>().get(entity), ...);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().get(entity), ...);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
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
     * auto &component = registry.has<Component>(entity) ? registry.get<Component>(entity) : registry.emplace<Component>(entity, args...);
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
    decltype(auto) get_or_emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();
        return cpool.contains(entity) ? cpool.get(entity) : cpool.emplace(*this, entity, std::forward<Args>(args)...);
    }

    /*! @copydoc get_or_emplace */
    template<typename Component, typename... Args>
    [[deprecated("use ::get_or_emplace instead")]]
    decltype(auto) get_or_assign(const entity_type entity, Args &&... args) {
        return get_or_emplace<Component>(entity, std::forward<Args>(args)...);
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
    auto try_get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().try_get(entity), ...);
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    auto try_get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().try_get(entity), ...);
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /**
     * @brief Clears a whole registry or the pools for the given components.
     * @tparam Component Types of components to remove from their entities.
     */
    template<typename... Component>
    void clear() {
        if constexpr(sizeof...(Component) == 0) {
            // useless this-> used to suppress a warning with clang
            each([this](const auto entity) { this->destroy(entity); });
        } else {
            ([this](auto &&cpool) {
                cpool.remove(*this, cpool.sparse_set<entity_type>::begin(), cpool.sparse_set<entity_type>::end());
            }(assure<Component>()), ...);
        }
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

        if(destroyed == null) {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                if(const auto entt = entities[pos - 1]; (to_integral(entt) & traits_type::entity_mask) == (pos - 1)) {
                    func(entt);
                }
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
        return std::none_of(pools.cbegin(), pools.cend(), [entity](auto &&pdata) { return pdata.pool && pdata.pool->contains(entity); });
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

        each([this, &func](const auto entity) {
            if(orphan(entity)) {
                func(entity);
            }
        });
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
     * void(registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been assigned to the
     * entity.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_construct() {
        return assure<Component>().on_construct();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is explicitly updated.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been updated.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_update() {
        return assure<Component>().on_update();
    }

    /*! @copydoc on_update */
    template<typename Component>
    [[deprecated("use registry::on_update instead")]]
    auto on_replace() {
        return on_update<Component>();
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
     * entity.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_destroy() {
        return assure<Component>().on_destroy();
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
     * Pools of components owned by a group cannot be sorted.<br/>
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
        auto &cpool = assure<Component>();
        ENTT_ASSERT(!cpool.super);
        cpool.sort(cpool.begin(), cpool.end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
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
     * Pools of components owned by a group cannot be sorted.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        auto &cpool = assure<To>();
        ENTT_ASSERT(!cpool.super);
        cpool.respect(assure<From>());
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
     * @tparam Exclude Types of components used to filter the view.
     * @return A newly created view.
     */
    template<typename... Component, typename... Exclude>
    entt::basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) > 0);
        return { assure<std::decay_t<Component>>()..., assure<Exclude>()... };
    }

    /*! @copydoc view */
    template<typename... Component, typename... Exclude>
    entt::basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Component>...>);
        return const_cast<basic_registry *>(this)->view<Component...>(exclude<Exclude...>);
    }

    /**
     * @brief Checks whether the given components belong to any group.
     * @tparam Component Types of components in which one is interested.
     * @return True if the pools of the given components are sortable, false
     * otherwise.
     */
    template<typename... Component>
    bool sortable() const {
        return !(assure<Component>().super || ...);
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0);
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1);

        using handler_type = group_handler<exclude_t<Exclude...>, get_t<std::decay_t<Get>...>, std::decay_t<Owned>...>;

        const auto cpools = std::forward_as_tuple(assure<std::decay_t<Owned>>()..., assure<std::decay_t<Get>>()...);
        constexpr auto size = sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude);
        handler_type *handler = nullptr;

        if(auto it = std::find_if(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
            return gdata.size == size
                && (gdata.owned(type_info<std::decay_t<Owned>>::id()) && ...)
                && (gdata.get(type_info<std::decay_t<Get>>::id()) && ...)
                && (gdata.exclude(type_info<Exclude>::id()) && ...);
        }); it != groups.cend())
        {
            handler = static_cast<handler_type *>(it->group.get());
        }

        if(!handler) {
            group_data candidate = {
                size,
                { new handler_type{}, [](void *instance) { delete static_cast<handler_type *>(instance); } },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Owned>>::id()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Get>>::id()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<Exclude>::id()) || ...); },
            };

            handler = static_cast<handler_type *>(candidate.group.get());

            const void *maybe_valid_if = nullptr;
            const void *discard_if = nullptr;

            if constexpr(sizeof...(Owned) == 0) {
                groups.push_back(std::move(candidate));
            } else {
                ENTT_ASSERT(std::all_of(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    const auto overlapping = (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id()));
                    const auto sz = overlapping + (0u + ... + gdata.get(type_info<std::decay_t<Get>>::id())) + (0u + ... + gdata.exclude(type_info<Exclude>::id()));
                    return !overlapping || ((sz == size) || (sz == gdata.size));
                }));

                const auto next = std::find_if_not(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    return !(0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id())) || (size > (gdata.size));
                });

                const auto prev = std::find_if(std::make_reverse_iterator(next), groups.crend(), [](const auto &gdata) {
                    return (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id()));
                });

                maybe_valid_if = (next == groups.cend() ? maybe_valid_if : next->group.get());
                discard_if = (prev == groups.crend() ? discard_if : prev->group.get());
                groups.insert(next, std::move(candidate));
            }

            ((std::get<pool_handler<std::decay_t<Owned>> &>(cpools).super = std::max(std::get<pool_handler<std::decay_t<Owned>> &>(cpools).super, size)), ...);

            (on_construct<std::decay_t<Owned>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Owned>>>(*handler), ...);
            (on_construct<std::decay_t<Get>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Get>>>(*handler), ...);
            (on_destroy<Exclude>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<Exclude>>(*handler), ...);

            (on_destroy<std::decay_t<Owned>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_destroy<std::decay_t<Get>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_construct<Exclude>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);

            if constexpr(sizeof...(Owned) == 0) {
                for(const auto entity: view<Owned..., Get...>(entt::exclude<Exclude...>)) {
                    handler->current.emplace(entity);
                }
            } else {
                // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
                for(auto *first = std::get<0>(cpools).data(), *last = first + std::get<0>(cpools).size(); first != last; ++first) {
                    handler->template maybe_valid_if<std::tuple_element_t<0, std::tuple<std::decay_t<Owned>...>>>(*this, *first);
                }
            }
        }

        if constexpr(sizeof...(Owned) == 0) {
            return { handler->current, std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
        } else {
            return { std::get<0>(cpools).super, handler->current, std::get<pool_handler<std::decay_t<Owned>> &>(cpools)... , std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
        }
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>..., std::is_const<Get>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(entt::get<Get...>, exclude<Exclude...>);
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Exclude>
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) {
        return group<Owned...>(entt::get<>, exclude<Exclude...>);
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Exclude>
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) const {
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
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of components.
     * @param last An iterator past the last element of the range of components.
     * @return A newly created runtime view.
     */
    template<typename It>
    entt::basic_runtime_view<Entity> runtime_view(It first, It last) const {
        std::vector<const sparse_set<Entity> *> selected(std::distance(first, last));

        std::transform(first, last, selected.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.pool && pdata.type_id == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        return { std::move(selected) };
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
    entt::basic_snapshot<Entity> snapshot() const {
        using follow_fn_type = entity_type(const basic_registry &, const entity_type);

        const auto head = to_integral(destroyed);
        const entity_type seed = (destroyed == null) ? destroyed : entity_type{head | (to_integral(entities[head]) & (traits_type::version_mask << traits_type::entity_shift))};

        follow_fn_type *follow = [](const basic_registry &reg, const entity_type entity) -> entity_type {
            const auto &others = reg.entities;
            const auto entt = to_integral(entity) & traits_type::entity_mask;
            const auto curr = to_integral(others[entt]) & traits_type::entity_mask;
            return entity_type{curr | (to_integral(others[curr]) & (traits_type::version_mask << traits_type::entity_shift))};
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
    basic_snapshot_loader<Entity> loader() {
        using force_fn_type = void(basic_registry &, const entity_type, const bool);

        force_fn_type *force = [](basic_registry &reg, const entity_type entity, const bool drop) {
            const auto entt = to_integral(entity) & traits_type::entity_mask;
            auto &others = reg.entities;

            if(!(entt < others.size())) {
                auto curr = others.size();
                others.resize(entt + 1);
                std::generate(others.data() + curr, others.data() + entt, [&curr]() { return entity_type(curr++); });
            }

            others[entt] = entity;

            if(drop) {
                reg.destroy(entity);
                const auto version = to_integral(entity) & (traits_type::version_mask << traits_type::entity_shift);
                others[entt] = entity_type{(to_integral(others[entt]) & traits_type::entity_mask) | version};
            }
        };

        clear();
        entities.clear();
        destroyed = null;

        return { this, force };
    }

    /**
     * @brief Visits an entity and returns the types for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
     * @endcode
     *
     * Returned identifiers are those of the components owned by the entity.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a component attached to or removed from the
     * given entity during the visit is returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param entity A valid entity identifier.
     * @param func A valid function object.
     */
    template<typename Func>
    void visit(entity_type entity, Func func) const {
        for(auto pos = pools.size(); pos; --pos) {
            if(const auto &pdata = pools[pos-1]; pdata.pool && pdata.pool->contains(entity)) {
                func(pdata.type_id);
            }
        }
    }

    /**
     * @brief Visits a registry and returns the types for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
     * @endcode
     *
     * Returned identifiers are those of the components managed by the registry.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a component for which a pool is created during
     * the visit is returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void visit(Func func) const {
        for(auto pos = pools.size(); pos; --pos) {
            if(const auto &pdata = pools[pos-1]; pdata.pool) {
                func(pdata.type_id);
            }
        }
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
        unset<Type>();
        vars.push_back(variable_data{type_info<Type>::id(), { new Type{std::forward<Args>(args)...}, [](void *instance) { delete static_cast<Type *>(instance); } }});
        return *static_cast<Type *>(vars.back().value.get());
    }

    /**
     * @brief Unsets a context variable if it exists.
     * @tparam Type Type of object to set.
     */
    template<typename Type>
    void unset() {
        vars.erase(std::remove_if(vars.begin(), vars.end(), [](auto &&var) {
            return var.type_id == type_info<Type>::id();
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
     * @return A reference to the object in the context of the registry.
     */
    template<typename Type, typename... Args>
    Type & ctx_or_set(Args &&... args) {
        auto *value = try_ctx<Type>();
        return value ? *value : set<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns a pointer to an object in the context of the registry.
     * @tparam Type Type of object to get.
     * @return A pointer to the object if it exists in the context of the
     * registry, a null pointer otherwise.
     */
    template<typename Type>
    const Type * try_ctx() const {
        auto it = std::find_if(vars.cbegin(), vars.cend(), [](auto &&var) { return var.type_id == type_info<Type>::id(); });
        return it == vars.cend() ? nullptr : static_cast<const Type *>(it->value.get());
    }

    /*! @copydoc try_ctx */
    template<typename Type>
    Type * try_ctx() {
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
    const Type & ctx() const {
        const auto *instance = try_ctx<Type>();
        ENTT_ASSERT(instance);
        return *instance;
    }

    /*! @copydoc ctx */
    template<typename Type>
    Type & ctx() {
        return const_cast<Type &>(std::as_const(*this).template ctx<Type>());
    }

    /**
     * @brief Visits a registry and returns the types for its context variables.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
     * @endcode
     *
     * Returned identifiers are those of the context variables currently set.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a context variable created during the visit is
     * returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void ctx(Func func) const {
        for(auto pos = vars.size(); pos; --pos) {
            func(vars[pos-1].type_id);
        }
    }

private:
    std::vector<group_data> groups{};
    mutable std::vector<pool_data> pools{};
    std::vector<entity_type> entities{};
    std::vector<variable_data> vars{};
    entity_type destroyed{null};
};


}


#endif

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/**
 * @brief Dedicated to those who aren't confident with the
 * entity-component-system architecture.
 *
 * Tiny wrapper around a registry, for all those users that aren't confident
 * with entity-component-system architecture and prefer to iterate objects
 * directly.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct basic_actor {
    /*! @brief Type of registry used internally. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    basic_actor() ENTT_NOEXCEPT
        : entt{entt::null}, reg{nullptr}
    {}

    /**
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_actor(basic_actor &&other) ENTT_NOEXCEPT
        : entt{other.entt}, reg{other.reg}
    {
        other.entt = null;
    }

    /**
     * @brief Constructs an actor from a given registry.
     * @param ref An instance of the registry class.
     */
    explicit basic_actor(registry_type &ref)
        : entt{ref.create()}, reg{&ref}
    {}

    /**
     * @brief Constructs an actor from a given entity.
     * @param entity A valid entity identifier.
     * @param ref An instance of the registry class.
     */
    explicit basic_actor(entity_type entity, registry_type &ref) ENTT_NOEXCEPT
        : entt{entity}, reg{&ref}
    {
        ENTT_ASSERT(ref.valid(entity));
    }

    /*! @brief Default destructor. */
    virtual ~basic_actor() {
        if(*this) {
            reg->destroy(entt);
        }
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
    basic_actor & operator=(basic_actor &&other) ENTT_NOEXCEPT {
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
        return reg->template emplace_or_replace<Component>(entt, std::forward<Args>(args)...);
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
     * @brief Checks if an actor has the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the actor has all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const {
        return reg->template has<Component...>(entt);
    }

    /**
     * @brief Returns references to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the actor.
     */
    template<typename... Component>
    decltype(auto) get() const {
        return std::as_const(*reg).template get<Component...>(entt);
    }

    /*! @copydoc get */
    template<typename... Component>
    decltype(auto) get() {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the actor.
     */
    template<typename... Component>
    auto try_get() const {
        return std::as_const(*reg).template try_get<Component...>(entt);
    }

    /*! @copydoc try_get */
    template<typename... Component>
    auto try_get() {
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    const registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /*! @copydoc backend */
    registry_type & backend() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(std::as_const(*this).backend());
    }

    /**
     * @brief Returns the entity associated with an actor.
     * @return The entity associated with the actor.
     */
    entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

    /**
     * @brief Checks if an actor refers to a valid entity or not.
     * @return True if the actor refers to a valid entity, false otherwise.
     */
    explicit operator bool() const {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
};


}


#endif

// #include "entity/entity.hpp"

// #include "entity/group.hpp"

// #include "entity/helper.hpp"
#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP


#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "../signal/delegate.hpp"

// #include "registry.hpp"

// #include "fwd.hpp"



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
     * @tparam Exclude Types of components used to filter the view.
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename Exclude, typename... Component>
    operator entt::basic_view<Entity, Exclude, Component...>() const {
        return reg.template view<Component...>(Exclude{});
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
     * @tparam Exclude Types of components used to filter the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Owned Types of components owned by the group.
     * @return A newly created group.
     */
    template<typename Exclude, typename Get, typename... Owned>
    operator entt::basic_group<Entity, Exclude, Get, Owned...>() const {
        return reg.template group<Owned...>(Get{}, Exclude{});
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
 * @brief Helper to create a listener that directly invokes a member function.
 * @tparam Member Member function to invoke on a component of the given type.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param reg A registry that contains the given entity and its components.
 * @param entt Entity from which to get the component.
 */
template<auto Member, typename Entity = entity>
void invoke(basic_registry<Entity> &reg, const Entity entt) {
    static_assert(std::is_member_function_pointer_v<decltype(Member)>);
    delegate<void(basic_registry<Entity> &, const Entity)> func;
    func.template connect<Member>(reg.template get<member_class_t<decltype(Member)>>(entt));
    func(reg, entt);
}


}


#endif

// #include "entity/observer.hpp"
#ifndef ENTT_ENTITY_OBSERVER_HPP
#define ENTT_ENTITY_OBSERVER_HPP


#include <limits>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "registry.hpp"

// #include "storage.hpp"

// #include "utility.hpp"

// #include "entity.hpp"

// #include "fwd.hpp"



namespace entt {


/*! @brief Grouping matcher. */
template<typename...>
struct matcher {};


/**
 * @brief Collector.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
struct basic_collector;


/**
 * @brief Collector.
 *
 * A collector contains a set of rules (literally, matchers) to use to track
 * entities.<br/>
 * Its main purpose is to generate a descriptor that allows an observer to know
 * how to connect to a registry.
 */
template<>
struct basic_collector<> {
    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of components tracked by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        return basic_collector<matcher<type_list<>, type_list<>, type_list<NoneOf...>, AllOf...>>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        return basic_collector<matcher<type_list<>, type_list<>, AnyOf>>{};
    }
};

/**
 * @brief Collector.
 * @copydetails basic_collector<>
 * @tparam Reject Untracked types used to filter out entities.
 * @tparam Require Untracked types required by the matcher.
 * @tparam Rule Specific details of the current matcher.
 * @tparam Other Other matchers.
 */
template<typename... Reject, typename... Require, typename... Rule, typename... Other>
struct basic_collector<matcher<type_list<Reject...>, type_list<Require...>, Rule...>, Other...> {
    /*! @brief Current matcher. */
    using current_type = matcher<type_list<Reject...>, type_list<Require...>, Rule...>;

    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of components tracked by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        return basic_collector<matcher<type_list<>, type_list<>, type_list<NoneOf...>, AllOf...>, current_type, Other...>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        return basic_collector<matcher<type_list<>, type_list<>, AnyOf>, current_type, Other...>{};
    }

    /**
     * @brief Updates the filter of the last added matcher.
     * @tparam AllOf Types of components required by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto where(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        using extended_type = matcher<type_list<Reject..., NoneOf...>, type_list<Require..., AllOf...>, Rule...>;
        return basic_collector<extended_type, Other...>{};
    }
};


/*! @brief Variable template used to ease the definition of collectors. */
inline constexpr basic_collector<> collector{};


/**
 * @brief Observer.
 *
 * An observer returns all the entities and only the entities that fit the
 * requirements of at least one matcher. Moreover, it's guaranteed that the
 * entity list is tightly packed in memory for fast iterations.<br/>
 * In general, observers don't stay true to the order of any set of components.
 *
 * Observers work mainly with two types of matchers, provided through a
 * collector:
 *
 * * Observing matcher: an observer will return at least all the living entities
 *   for which one or more of the given components have been explicitly
 *   replaced and not yet destroyed.
 * * Grouping matcher: an observer will return at least all the living entities
 *   that would have entered the given group if it existed and that would have
 *   not yet left it.
 *
 * If an entity respects the requirements of multiple matchers, it will be
 * returned once and only once by the observer in any case.
 *
 * Matchers support also filtering by means of a _where_ clause that accepts
 * both a list of types and an exclusion list.<br/>
 * Whenever a matcher finds that an entity matches its requirements, the
 * condition of the filter is verified before to register the entity itself.
 * Moreover, a registered entity isn't returned by the observer if the condition
 * set by the filter is broken in the meantime.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @warning
 * Lifetime of an observer doesn't necessarily have to overcome that of the
 * registry to which it is connected. However, the observer must be disconnected
 * from the registry before being destroyed to avoid crashes due to dangling
 * pointers.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_observer {
    using payload_type = std::uint32_t;

    template<typename>
    struct matcher_handler;

    template<typename... Reject, typename... Require, typename AnyOf>
    struct matcher_handler<matcher<type_list<Reject...>, type_list<Require...>, AnyOf>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const basic_registry<Entity> &reg, const Entity entt) {
            if(reg.template has<Require...>(entt) && !reg.template any<Reject...>(entt)) {
                if(auto *comp = obs.view.try_get(entt); !comp) {
                    obs.view.emplace(entt);
                }

                obs.view.get(entt) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const basic_registry<Entity> &, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.erase(entt);
            }
        }

        template<std::size_t Index>
        static void connect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<Reject>().template connect<&discard_if<Index>>(obs), ...);
            reg.template on_update<AnyOf>().template connect<&maybe_valid_if<Index>>(obs);
            reg.template on_destroy<AnyOf>().template connect<&discard_if<Index>>(obs);
        }

        static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().disconnect(obs), ...);
            (reg.template on_construct<Reject>().disconnect(obs), ...);
            reg.template on_update<AnyOf>().disconnect(obs);
            reg.template on_destroy<AnyOf>().disconnect(obs);
        }
    };

    template<typename... Reject, typename... Require, typename... NoneOf, typename... AllOf>
    struct matcher_handler<matcher<type_list<Reject...>, type_list<Require...>, type_list<NoneOf...>, AllOf...>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const basic_registry<Entity> &reg, const Entity entt) {
            if(reg.template has<AllOf..., Require...>(entt) && !reg.template any<NoneOf..., Reject...>(entt)) {
                if(auto *comp = obs.view.try_get(entt); !comp) {
                    obs.view.emplace(entt);
                }

                obs.view.get(entt) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const basic_registry<Entity> &, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.erase(entt);
            }
        }

        template<std::size_t Index>
        static void connect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<Reject>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<AllOf>().template connect<&maybe_valid_if<Index>>(obs), ...);
            (reg.template on_destroy<NoneOf>().template connect<&maybe_valid_if<Index>>(obs), ...);
            (reg.template on_destroy<AllOf>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<NoneOf>().template connect<&discard_if<Index>>(obs), ...);
        }

        static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().disconnect(obs), ...);
            (reg.template on_construct<Reject>().disconnect(obs), ...);
            (reg.template on_construct<AllOf>().disconnect(obs), ...);
            (reg.template on_destroy<NoneOf>().disconnect(obs), ...);
            (reg.template on_destroy<AllOf>().disconnect(obs), ...);
            (reg.template on_construct<NoneOf>().disconnect(obs), ...);
        }
    };

    template<typename... Matcher>
    static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
        (matcher_handler<Matcher>::disconnect(obs, reg), ...);
    }

    template<typename... Matcher, std::size_t... Index>
    void connect(basic_registry<Entity> &reg, std::index_sequence<Index...>) {
        static_assert(sizeof...(Matcher) < std::numeric_limits<payload_type>::digits);
        (matcher_handler<Matcher>::template connect<Index>(*this, reg), ...);
        release = &basic_observer::disconnect<Matcher...>;
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = typename sparse_set<Entity>::iterator;

    /*! @brief Default constructor. */
    basic_observer()
        : target{}, release{}, view{}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_observer(const basic_observer &) = delete;
    /*! @brief Default move constructor, deleted on purpose. */
    basic_observer(basic_observer &&) = delete;

    /**
     * @brief Creates an observer and connects it to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     */
    template<typename... Matcher>
    basic_observer(basic_registry<entity_type> &reg, basic_collector<Matcher...>)
        : target{&reg},
          release{},
          view{}
    {
        connect<Matcher...>(reg, std::index_sequence_for<Matcher...>{});
    }

    /*! @brief Default destructor. */
    ~basic_observer() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer & operator=(const basic_observer &) = delete;

    /**
     * @brief Default move assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer & operator=(basic_observer &&) = delete;

    /**
     * @brief Connects an observer to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     */
    template<typename... Matcher>
    void connect(basic_registry<entity_type> &reg, basic_collector<Matcher...>) {
        disconnect();
        connect<Matcher...>(reg, std::index_sequence_for<Matcher...>{});
        target = &reg;
        view.clear();
    }

    /*! @brief Disconnects an observer from the registry it keeps track of. */
    void disconnect() {
        if(release) {
            release(*this, *target);
            release = nullptr;
        }
    }

    /**
     * @brief Returns the number of elements in an observer.
     * @return Number of elements.
     */
    size_type size() const ENTT_NOEXCEPT {
        return view.size();
    }

    /**
     * @brief Checks whether an observer is empty.
     * @return True if the observer is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return view.empty();
    }

    /**
     * @brief Direct access to the list of entities of the observer.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the observer in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return view.data();
    }

    /**
     * @brief Returns an iterator to the first entity of the observer.
     *
     * The returned iterator points to the first entity of the observer. If the
     * container is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the observer.
     */
    iterator begin() const ENTT_NOEXCEPT {
        return view.sparse_set<entity_type>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity of the observer.
     *
     * The returned iterator points to the entity following the last entity of
     * the observer. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * observer.
     */
    iterator end() const ENTT_NOEXCEPT {
        return view.sparse_set<entity_type>::end();
    }

    /*! @brief Clears the underlying container. */
    void clear() ENTT_NOEXCEPT {
        view.clear();
    }

    /**
     * @brief Iterates entities and applies the given function object to them.
     *
     * The function object is invoked for each entity.<br/>
     * The signature of the function must be equivalent to the following form:
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
        static_assert(std::is_invocable_v<Func, entity_type>);

        for(const auto entity: *this) {
            func(entity);
        }
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        std::as_const(*this).each(std::move(func));
        clear();
    }

private:
    basic_registry<entity_type> *target;
    void(* release)(basic_observer &, basic_registry<entity_type> &);
    storage<entity_type, payload_type> view;
};


}


#endif

// #include "entity/registry.hpp"

// #include "entity/runtime_view.hpp"

// #include "entity/snapshot.hpp"

// #include "entity/sparse_set.hpp"

// #include "entity/storage.hpp"

// #include "entity/utility.hpp"

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
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



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
    static bool empty() ENTT_NOEXCEPT {
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
    static std::weak_ptr<Service> get() ENTT_NOEXCEPT {
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
    static Service & ref() ENTT_NOEXCEPT {
        return *service;
    }

    /**
     * @brief Sets or replaces a service.
     * @tparam Impl Type of the new service to use.
     * @tparam Args Types of arguments to use to construct the service.
     * @param args Parameters to use to construct the service.
     */
    template<typename Impl = Service, typename... Args>
    static void set(Args &&... args) {
        service = std::make_shared<Impl>(std::forward<Args>(args)...);
    }

    /**
     * @brief Sets or replaces a service.
     * @param ptr Service to use to replace the current one.
     */
    static void set(std::shared_ptr<Service> ptr) {
        ENTT_ASSERT(static_cast<bool>(ptr));
        service = std::move(ptr);
    }

    /**
     * @brief Resets a service.
     *
     * The service is no longer valid after a reset.
     */
    static void reset() {
        service.reset();
    }

private:
    inline static std::shared_ptr<Service> service = nullptr;
};


}


#endif

// #include "meta/factory.hpp"
#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "../core/fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif

// #include "../core/type_info.hpp"
#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


// #include "../config/config.h"

// #include "../core/attribute.h"
#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

// #include "hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>
// #include "../config/config.h"

// #include "fwd.hpp"



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
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    using type = std::uint64_t;
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr id_type helper(const Char *curr) ENTT_NOEXCEPT {
        auto value = traits_type::offset;

        while(*curr != 0) {
            value = (value ^ static_cast<traits_type::type>(*(curr++))) * traits_type::prime;
        }

        return value;
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
    /*! @brief Unsigned integer type. */
    using hash_type = id_type;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        id_type partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * basic_hashed_string<char> hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const value_type * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /*! @copydoc data */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return data(); }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return value(); }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const basic_hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const value_type *str;
    hash_type hash;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the character type of the hashed string directly from a
 * human-readable identifer provided to the constructor.
 *
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifer.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) ENTT_NOEXCEPT
-> basic_hashed_string<Char>;


/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;


/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
constexpr entt::hashed_wstring operator"" ENTT_HWS_SUFFIX(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_wstring{str};
}


#endif

// #include "fwd.hpp"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_index {
    static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Type index.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_index::next();
        return value;
    }
};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * indexable, false otherwise.
 * @tparam Type Potentially indexable type.
 */
template<typename, typename = void>
struct has_type_index: std::false_type {};


/*! @brief has_type_index */
template<typename Type>
struct has_type_index<Type, std::void_t<decltype(type_index<Type>::value())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially indexable type.
 */
template<typename Type>
inline constexpr bool has_type_index_v = has_type_index<Type>::value;


/**
 * @brief Type info.
 * @tparam Type Type for which to generate information.
 */
template<typename Type, typename = void>
struct ENTT_API type_info {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    static constexpr id_type id() ENTT_NOEXCEPT {
        constexpr auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION_CONSTEXPR);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    static id_type id() ENTT_NOEXCEPT {
        static const auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    static id_type id() ENTT_NOEXCEPT {
        return type_index<Type>::value();
    }
#endif
};


}


#endif

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "hashed_string.hpp"

// #include "fwd.hpp"



namespace entt {


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
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>);

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


/**
 * @brief Defines an enum class to use for opaque identifiers and a dedicate
 * `to_integer` function to convert the identifiers to their underlying type.
 * @param clazz The name to use for the enum class.
 * @param type The underlying type for the enum class.
 */
#define ENTT_OPAQUE_TYPE(clazz, type)\
    enum class clazz: type {};\
    constexpr auto to_integral(const clazz id) ENTT_NOEXCEPT {\
        return static_cast<std::underlying_type_t<clazz>>(id);\
    }\
    static_assert(true)


#endif

// #include "../core/utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


#include <utility>
// #include "../config/config.h"



namespace entt {


/*! @brief Identity function object (waiting for C++20). */
struct identity {
    /**
     * @brief Returns its argument unchanged.
     * @tparam Type Type of the argument.
     * @param value The actual argument.
     * @return The submitted value as-is.
     */
    template<class Type>
    constexpr Type && operator()(Type &&value) const ENTT_NOEXCEPT {
        return std::forward<Type>(value);
    }
};


/**
 * @brief Constant utility to disambiguate overloaded members of a class.
 * @tparam Type Type of the desired overload.
 * @tparam Class Type of class to which the member belongs.
 * @param member A valid pointer to a member.
 * @return Pointer to the member.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Func Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Func>
constexpr auto overload(Func *func) ENTT_NOEXCEPT { return func; }


/**
 * @brief Helper type for visitors.
 * @tparam Func Types of function objects.
 */
template<class... Func>
struct overloaded: Func... {
    using Func::operator()...;
};


/**
 * @brief Deduction guide.
 * @tparam Func Types of function objects.
 */
template<class... Func>
overloaded(Func...) -> overloaded<Func...>;


/**
 * @brief Basic implementation of a y-combinator.
 * @tparam Func Type of a potentially recursive function.
 */
template<class Func>
struct y_combinator {
    /**
     * @brief Constructs a y-combinator from a given function.
     * @param recursive A potentially recursive function.
     */
    y_combinator(Func recursive):
        func{std::move(recursive)}
    {}

    /**
     * @brief Invokes a y-combinator and therefore its underlying function.
     * @tparam Args Types of arguments to use to invoke the underlying function.
     * @param args Parameters to use to invoke the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) const {
        return func(*this, std::forward<Args>(args)...);
    }

    /*! @copydoc operator()() */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};


}


#endif

// #include "meta.hpp"
#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
// #include "../config/config.h"

// #include "../core/fwd.hpp"

// #include "../core/type_info.hpp"

// #include "../core/type_traits.hpp"



namespace entt {


class meta_any;
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
};


struct meta_base_node {
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    void *(* const cast)(void *) ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    meta_any(* const conv)(const void *);
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
};


struct meta_dtor_node {
    meta_type_node * const parent;
    void(* const invoke)(void *);
};


struct meta_data_node {
    id_type id;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_any, meta_any, meta_any);
    meta_any(* const get)(meta_any, meta_any);
};


struct meta_func_node {
    using size_type = std::size_t;
    id_type id;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)() ENTT_NOEXCEPT;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any, meta_any *);
};


struct meta_type_node {
    using size_type = std::size_t;
    const id_type type_id;
    id_type id;
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
    const bool is_function_pointer;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const size_type extent;
    bool(* const compare)(const void *, const void *);
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_dtor_node *dtor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
};


template<typename Type, typename Op, typename Node>
void visit(Op &op, Node *node) {
    while(node) {
        op(Type{node});
        node = node->next;
    }
}


template<auto Member, typename Type, typename Op>
void visit(Op &op, const internal::meta_type_node *node) {
    if(node) {
        internal::visit<Type>(op, node->*Member);
        auto *next = node->base;

        while(next) {
            visit<Member, Type>(op, next->type());
            next = next->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(const Op &op, Node *node) {
    while(node && !op(node)) {
        node = node->next;
    }

    return node;
}


template<auto Member, typename Op>
auto find_if(const Op &op, const meta_type_node *node)
-> decltype(find_if(op, node->*Member)) {
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *next = node->base;

        while(next && !ret) {
            ret = find_if<Member>(op, next->type());
            next = next->next;
        }
    }

    return ret;
}


template<typename Type>
bool compare(const void *lhs, const void *rhs) {
    if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
        return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
    } else {
        return lhs == rhs;
    }
}


struct ENTT_API meta_context {
    inline static meta_type_node *local = nullptr;
    inline static meta_type_node **global = &local;

    static void detach(const meta_type_node *node) ENTT_NOEXCEPT {
        auto **it = global;

        while(*it && *it != node) {
            it = &(*it)->next;
        }

        if(*it) {
            *it = (*it)->next;
        }
    }
};


template<typename Type>
struct ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>);

    static meta_type_node * resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            type_info<Type>::id(),
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
            std::is_pointer_v<Type> && std::is_function_v<std::remove_pointer_t<Type>>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            std::extent_v<Type>,
            &compare<Type>, // workaround for an issue with VS2017
            &meta_node<std::remove_const_t<std::remove_pointer_t<Type>>>::resolve,
            &meta_node<std::remove_const_t<std::remove_extent_t<Type>>>::resolve
        };

        return &node;
    }
};


template<typename... Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>...> {};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Opaque container for a meta context. */
struct meta_ctx {
    /**
     * @brief Binds the meta system to the given context.
     * @param other A valid context to which to bind.
     */
    static void bind(meta_ctx other) ENTT_NOEXCEPT {
        internal::meta_context::global = other.ctx;
    }

private:
    internal::meta_type_node **ctx{&internal::meta_context::local};
};


/**
 * @brief Opaque container for values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to get rid
 * of memory allocations if possible. This should improve overall performance.
 */
class meta_any {
    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using copy_fn_type = void(meta_any &, const meta_any &);
    using steal_fn_type = void(meta_any &, meta_any &);
    using destroy_fn_type = void(meta_any &);

    template<typename Type, typename = std::void_t<>>
    struct type_traits {
        template<typename... Args>
        static void instance(meta_any &any, Args &&... args) {
            any.instance = new Type{std::forward<Args>(args)...};
            new (&any.storage) Type *{static_cast<Type *>(any.instance)};
        }

        static void destroy(meta_any &any) {
            const auto * const node = internal::meta_info<Type>::resolve();
            if(node->dtor) { node->dtor->invoke(any.instance); }
            delete static_cast<Type *>(any.instance);
        }

        static void copy(meta_any &to, const meta_any &from) {
            auto *instance = new Type{*static_cast<const Type *>(from.instance)};
            new (&to.storage) Type *{instance};
            to.instance = instance;
        }

        static void steal(meta_any &to, meta_any &from) {
            new (&to.storage) Type *{static_cast<Type *>(from.instance)};
            to.instance = from.instance;
        }
    };

    template<typename Type>
    struct type_traits<Type, std::enable_if_t<sizeof(Type) <= sizeof(void *) && std::is_nothrow_move_constructible_v<Type>>> {
        template<typename... Args>
        static void instance(meta_any &any, Args &&... args) {
            any.instance = new (&any.storage) Type{std::forward<Args>(args)...};
        }

        static void destroy(meta_any &any) {
            const auto * const node = internal::meta_info<Type>::resolve();
            if(node->dtor) { node->dtor->invoke(any.instance); }
            static_cast<Type *>(any.instance)->~Type();
        }

        static void copy(meta_any &to, const meta_any &from) {
            to.instance = new (&to.storage) Type{*static_cast<const Type *>(from.instance)};
        }

        static void steal(meta_any &to, meta_any &from) {
            to.instance = new (&to.storage) Type{std::move(*static_cast<Type *>(from.instance))};
            destroy(from);
        }
    };

    meta_any(const internal::meta_type_node *curr, void *ref) ENTT_NOEXCEPT
        : meta_any{}
    {
        node = curr;
        instance = ref;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{},
          node{},
          destroy_fn{},
          copy_fn{},
          steal_fn{}
    {}

    /**
     * @brief Constructs a meta any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the container.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : meta_any{}
    {
        node = internal::meta_info<Type>::resolve();

        if constexpr(!std::is_void_v<Type>) {
            static_assert(std::is_copy_constructible_v<Type>);
            using traits_type = type_traits<std::remove_cv_t<std::remove_reference_t<Type>>>;
            traits_type::instance(*this, std::forward<Args>(args)...);
            destroy_fn = &traits_type::destroy;
            copy_fn = &traits_type::copy;
            steal_fn = &traits_type::steal;
        }
    }

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type>
    meta_any(std::reference_wrapper<Type> value)
        : meta_any{internal::meta_info<Type>::resolve(), &value.get()}
    {}

    /**
     * @brief Constructs a meta any from a given value.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other)
        : meta_any{}
    {
        node = other.node;
        (other.copy_fn ? other.copy_fn : [](meta_any &to, const meta_any &from) { to.instance = from.instance; })(*this, other);
        destroy_fn = other.destroy_fn;
        copy_fn = other.copy_fn;
        steal_fn = other.steal_fn;
    }

    /**
     * @brief Move constructor.
     *
     * After move construction, instances that have been moved from are placed
     * in a valid but unspecified state.
     *
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other)
        : meta_any{}
    {
        swap(*this, other);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy_fn) {
            destroy_fn(*this);
        }
    }

    /**
     * @brief Assignment operator.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     * @return This meta any object.
     */
    template<typename Type>
    meta_any & operator=(Type &&value) {
        return (*this = meta_any{std::forward<Type>(value)});
    }

    /**
     * @brief Assignment operator.
     * @param other The instance to assign from.
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
    const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    /*! @copydoc data */
    void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Tries to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return A (possibly null) pointer to the contained instance.
     */
    template<typename Type>
    const Type * try_cast() const {
        void *ret = nullptr;

        if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node && node->type_id == type_id) {
            ret = instance;
        } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); base) {
            ret = base->cast(instance);
        }

        return static_cast<const Type *>(ret);
    }

    /*! @copydoc try_cast */
    template<typename Type>
    Type * try_cast() {
        return const_cast<Type *>(std::as_const(*this).try_cast<Type>());
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
    const Type & cast() const {
        auto * const actual = try_cast<Type>();
        ENTT_ASSERT(actual);
        return *actual;
    }

    /*! @copydoc cast */
    template<typename Type>
    Type & cast() {
        return const_cast<Type &>(std::as_const(*this).cast<Type>());
    }

    /**
     * @brief Tries to convert an instance to a given type and returns it.
     * @tparam Type Type to which to convert the instance.
     * @return A valid meta any object if the conversion is possible, an invalid
     * one otherwise.
     */
    template<typename Type>
    meta_any convert() const {
        meta_any any{};

        if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node && node->type_id == type_id) {
            any = *this;
        } else if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); conv) {
            any = conv->conv(instance);
        }

        return any;
    }

    /**
     * @brief Tries to convert an instance to a given type.
     * @tparam Type Type to which to convert the instance.
     * @return True if the conversion is possible, false otherwise.
     */
    template<typename Type>
    bool convert() {
        bool valid = (node && node->type_id == internal::meta_info<Type>::resolve()->type_id);

        if(!valid) {
            if(auto any = std::as_const(*this).convert<Type>(); any) {
                swap(any, *this);
                valid = true;
            }
        }

        return valid;
    }

    /**
     * @brief Replaces the contained object by initializing a new instance
     * directly.
     * @tparam Type Type of object to use to initialize the container.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = meta_any{std::in_place_type_t<Type>{}, std::forward<Args>(args)...};
    }

    /**
     * @brief Indirection operator for aliasing construction.
     * @return An alias to the contained object.
     */
    meta_any operator *() const ENTT_NOEXCEPT {
        return meta_any{node, instance};
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    bool operator==(const meta_any &other) const {
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id && node->compare(instance, other.instance));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) {
        if(lhs.steal_fn && rhs.steal_fn) {
            meta_any buffer{};
            lhs.steal_fn(buffer, lhs);
            rhs.steal_fn(lhs, rhs);
            lhs.steal_fn(rhs, buffer);
        } else if(lhs.steal_fn) {
            lhs.steal_fn(rhs, lhs);
        } else if(rhs.steal_fn) {
            rhs.steal_fn(lhs, rhs);
        } else {
            std::swap(lhs.instance, rhs.instance);
        }

        std::swap(lhs.node, rhs.node);
        std::swap(lhs.destroy_fn, rhs.destroy_fn);
        std::swap(lhs.copy_fn, rhs.copy_fn);
        std::swap(lhs.steal_fn, rhs.steal_fn);
    }

private:
    storage_type storage;
    void *instance;
    const internal::meta_type_node *node;
    destroy_fn_type *destroy_fn;
    copy_fn_type *copy_fn;
    steal_fn_type *steal_fn;
};


/**
 * @brief Opaque pointers to instances of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance.<br/>
 * Handles are used mainly to generate aliases for actual objects when needed.
 */
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle()
        : any{}
    {}

    /**
     * @brief Creates an alias for the actual object.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type>
    meta_handle(Type &&value) ENTT_NOEXCEPT
        : meta_handle{}
    {
        if constexpr(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>) {
            any = *value;
        } else {
            static_assert(std::is_lvalue_reference_v<Type>);
            any = std::ref(value);
        }
    }

    /*! @copydoc meta_any::operator* */
    meta_any operator *() const {
        return any;
    }

private:
    meta_any any;
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


/*! @brief Opaque container for meta properties of any type. */
struct meta_prop {
    /**
     * @brief Constructs an instance from a given node.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_prop(const internal::meta_prop_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    meta_any key() const {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    meta_any value() const {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_prop_node *node;
};


/*! @brief Opaque container for meta base classes. */
struct meta_base {
    /*! @copydoc meta_prop::meta_prop */
    meta_base(const internal::meta_base_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the meta type to which a meta object belongs.
     * @return The meta type to which the meta object belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Casts an instance from a parent type to a base type.
     * @param instance The instance to cast.
     * @return An opaque pointer to the base type.
     */
    void * cast(void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_base_node *node;
};


/*! @brief Opaque container for meta conversion functions. */
struct meta_conv {
    /*! @copydoc meta_prop::meta_prop */
    meta_conv(const internal::meta_conv_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Converts an instance to a given type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    meta_any convert(const void *instance) const {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_conv_node *node;
};


/*! @brief Opaque container for meta constructors. */
struct meta_ctor {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_ctor(const internal::meta_ctor_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta constructor.
     * @return The number of arguments accepted by the meta constructor.
     */
    size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta constructor.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta constructor, if any.
     */
    meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke([[maybe_unused]] Args &&... args) const {
        if constexpr(sizeof...(Args) == 0) {
            return sizeof...(Args) == size() ? node->invoke(nullptr) : meta_any{};
        } else {
            meta_any arguments[]{std::forward<Args>(args)...};
            return sizeof...(Args) == size() ? node->invoke(arguments) : meta_any{};
        }
    }

    /**
     * @brief Iterates all the properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(op, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_ctor_node *node;
};


/*! @brief Opaque container for meta data. */
struct meta_data {
    /*! @copydoc meta_prop::meta_prop */
    meta_data(const internal::meta_data_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::id */
    id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /*! @copydoc id */
    [[deprecated("use ::id instead")]]
    id_type alias() const ENTT_NOEXCEPT {
        return id();
    }

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Indicates whether a given meta data is constant or not.
     * @return True if the meta data is constant, false otherwise.
     */
    bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta data is static or not.
     * @return True if the meta data is static, false otherwise.
     */
    bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must be such that a cast or conversion to the type
     * of the variable is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param instance An opaque instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle instance, Type &&value) const {
        return node->set(*instance, {}, std::forward<Type>(value));
    }

    /**
     * @brief Sets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must be such that a cast or conversion to the array
     * type is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param instance An opaque instance of the underlying type.
     * @param index Position of the underlying element to set.
     * @param value Parameter to use to set the underlying element.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle instance, std::size_t index, Type &&value) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->set(*instance, index, std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    meta_any get(meta_handle instance) const {
        return node->get(*instance, {});
    }

    /**
     * @brief Gets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @param index Position of the underlying element to get.
     * @return A meta any containing the value of the underlying element.
     */
    meta_any get(meta_handle instance, std::size_t index) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->get(*instance, index);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(op, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_data_node *node;
};


/*! @brief Opaque container for meta functions. */
struct meta_func {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_func(const internal::meta_func_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::id */
    id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /*! @copydoc id */
    [[deprecated("use ::id instead")]]
    id_type alias() const ENTT_NOEXCEPT {
        return id();
    }

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta function.
     * @return The number of arguments accepted by the meta function.
     */
    size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Indicates whether a given meta function is constant or not.
     * @return True if the meta function is constant, false otherwise.
     */
    bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta function is static or not.
     * @return True if the meta function is static, false otherwise.
     */
    bool is_static() const ENTT_NOEXCEPT {
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
     * To invoke a meta function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&... args) const {
        meta_any arguments[]{*instance, std::forward<Args>(args)...};
        return sizeof...(Args) == size() ? node->invoke(arguments[0], &arguments[sizeof...(Args) != 0]) : meta_any{};
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(op, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_func_node *node;
};


/*! @brief Opaque container for meta types. */
class meta_type {
    template<typename... Args, std::size_t... Indexes>
    auto ctor(std::index_sequence<Indexes...>) const {
        return internal::find_if([](const auto *candidate) {
            return candidate->size == sizeof...(Args) && ([](auto *from, auto *to) {
                return (from->type_id == to->type_id)
                        || internal::find_if<&internal::meta_type_node::base>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from)
                        || internal::find_if<&internal::meta_type_node::conv>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from);
            }(internal::meta_info<Args>::resolve(), candidate->arg(Indexes)) && ...);
        }, node->ctor);
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_type(const internal::meta_type_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the type id of the underlying type.
     * @return The type id of the underlying type.
     */
    id_type type_id() const ENTT_NOEXCEPT {
        return node->type_id;
    }

    /**
     * @brief Returns the identifier assigned to a given meta object.
     * @return The identifier assigned to the meta object.
     */
    id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /*! @copydoc id */
    [[deprecated("use ::id instead")]]
    id_type alias() const ENTT_NOEXCEPT {
        return id();
    }

    /**
     * @brief Indicates whether a given meta type refers to void or not.
     * @return True if the underlying type is void, false otherwise.
     */
    bool is_void() const ENTT_NOEXCEPT {
        return node->is_void;
    }

    /**
     * @brief Indicates whether a given meta type refers to an integral type or
     * not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    bool is_integral() const ENTT_NOEXCEPT {
        return node->is_integral;
    }

    /**
     * @brief Indicates whether a given meta type refers to a floating-point
     * type or not.
     * @return True if the underlying type is a floating-point type, false
     * otherwise.
     */
    bool is_floating_point() const ENTT_NOEXCEPT {
        return node->is_floating_point;
    }

    /**
     * @brief Indicates whether a given meta type refers to an array type or
     * not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    bool is_array() const ENTT_NOEXCEPT {
        return node->is_array;
    }

    /**
     * @brief Indicates whether a given meta type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    bool is_enum() const ENTT_NOEXCEPT {
        return node->is_enum;
    }

    /**
     * @brief Indicates whether a given meta type refers to an union or not.
     * @return True if the underlying type is an union, false otherwise.
     */
    bool is_union() const ENTT_NOEXCEPT {
        return node->is_union;
    }

    /**
     * @brief Indicates whether a given meta type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    bool is_class() const ENTT_NOEXCEPT {
        return node->is_class;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    bool is_pointer() const ENTT_NOEXCEPT {
        return node->is_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a function pointer
     * or not.
     * @return True if the underlying type is a function pointer, false
     * otherwise.
     */
    bool is_function_pointer() const ENTT_NOEXCEPT {
        return node->is_function_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to data
     * member or not.
     * @return True if the underlying type is a pointer to data member, false
     * otherwise.
     */
    bool is_member_object_pointer() const ENTT_NOEXCEPT {
        return node->is_member_object_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to member
     * function or not.
     * @return True if the underlying type is a pointer to member function,
     * false otherwise.
     */
    bool is_member_function_pointer() const ENTT_NOEXCEPT {
        return node->is_member_function_pointer;
    }

    /**
     * @brief If a given meta type refers to an array type, provides the number
     * of elements of the array.
     * @return The number of elements of the array if the underlying type is an
     * array type, 0 otherwise.
     */
    size_type extent() const ENTT_NOEXCEPT {
        return node->extent;
    }

    /**
     * @brief Provides the meta type for which the pointer is defined.
     * @return The meta type for which the pointer is defined or this meta type
     * if it doesn't refer to a pointer type.
     */
    meta_type remove_pointer() const ENTT_NOEXCEPT {
        return node->remove_pointer();
    }

    /**
     * @brief Provides the meta type for which the array is defined.
     * @return The meta type for which the array is defined or this meta type
     * if it doesn't refer to an array type.
     */
    meta_type remove_extent() const ENTT_NOEXCEPT {
        return node->remove_extent();
    }

    /**
     * @brief Iterates all the meta bases of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_base>, void>
    base(Op op) const {
        internal::visit<&internal::meta_type_node::base, meta_base>(op, node);
    }

    /**
     * @brief Returns the meta base associated with a given identifier.
     * @param id Unique identifier.
     * @return The meta base associated with the given identifier, if any.
     */
    meta_base base(const id_type id) const {
        return internal::find_if<&internal::meta_type_node::base>([id](const auto *curr) {
            return curr->type()->id == id;
        }, node);
    }

    /**
     * @brief Iterates all the meta conversion functions of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void conv(Op op) const {
        internal::visit<&internal::meta_type_node::conv, meta_conv>(op, node);
    }

    /**
     * @brief Returns the meta conversion function associated with a given type.
     * @tparam Type The type to use to search for a meta conversion function.
     * @return The meta conversion function associated with the given type, if
     * any.
     */
    template<typename Type>
    meta_conv conv() const {
        return internal::find_if<&internal::meta_type_node::conv>([type_id = internal::meta_info<Type>::resolve()->type_id](const auto *curr) {
            return curr->type()->type_id == type_id;
        }, node);
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void ctor(Op op) const {
        internal::visit<meta_ctor>(op, node->ctor);
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    meta_ctor ctor() const {
        return ctor<Args...>(std::index_sequence_for<Args...>{});
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     *
     * The meta data of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_data>, void>
    data(Op op) const {
        internal::visit<&internal::meta_type_node::data, meta_data>(op, node);
    }

    /**
     * @brief Returns the meta data associated with a given identifier.
     *
     * The meta data of the base classes will also be visited, if any.
     *
     * @param id Unique identifier.
     * @return The meta data associated with the given identifier, if any.
     */
    meta_data data(const id_type id) const {
        return internal::find_if<&internal::meta_type_node::data>([id](const auto *curr) {
            return curr->id == id;
        }, node);
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     *
     * The meta functions of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_func>, void>
    func(Op op) const {
        internal::visit<&internal::meta_type_node::func, meta_func>(op, node);
    }

    /**
     * @brief Returns the meta function associated with a given identifier.
     *
     * The meta functions of the base classes will also be visited, if any.
     *
     * @param id Unique identifier.
     * @return The meta function associated with the given identifier, if any.
     */
    meta_func func(const id_type id) const {
        return internal::find_if<&internal::meta_type_node::func>([id](const auto *curr) {
            return curr->id == id;
        }, node);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any construct(Args &&... args) const {
        auto construct_if = [this](meta_any *params) {
            meta_any any{};

            internal::find_if<&internal::meta_type_node::ctor>([params, &any](const auto *curr) {
                return (curr->size == sizeof...(args)) && (any = curr->invoke(params));
            }, node);

            return any;
        };

        if constexpr(sizeof...(Args) == 0) {
            return construct_if(nullptr);
        } else {
            meta_any arguments[]{std::forward<Args>(args)...};
            return construct_if(arguments);
        }
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     *
     * The properties of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<&internal::meta_type_node::prop, meta_prop>(op, node);
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * The properties of the base classes will also be visited, if any.
     *
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if<&internal::meta_type_node::prop>([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two meta objects refer to the same type.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same type, false
     * otherwise.
     */
    bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id);
    }

    /*! @brief Removes a meta object from the list of searchable types. */
    void detach() ENTT_NOEXCEPT {
        internal::meta_context::detach(node);
    }

private:
    const internal::meta_type_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same type.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return False if the two meta objects refer to the same node, true otherwise.
 */
inline bool operator!=(const meta_type &lhs, const meta_type &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


inline meta_type meta_any::type() const ENTT_NOEXCEPT {
    return node;
}


inline meta_type meta_base::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_base::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_conv::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_conv::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


inline meta_type meta_data::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_data::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_func::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_func::ret() const ENTT_NOEXCEPT {
    return node->ret();
}


inline meta_type meta_func::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


}


#endif

// #include "policy.hpp"
#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP


namespace entt {


/*! @brief Empty class type used to request the _as alias_ policy. */
struct as_alias_t {};


/*! @brief Disambiguation tag. */
inline constexpr as_alias_t as_alias;


/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t {};


/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t {};


}


#endif



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct meta_function_helper;


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...)> {
    using return_type = std::remove_cv_t<std::remove_reference_t<Ret>>;
    using args_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

    static constexpr std::index_sequence_for<Args...> index_sequence{};
    static constexpr auto is_const = false;

    static auto arg(typename internal::meta_func_node::size_type index) ENTT_NOEXCEPT {
        return std::array<meta_type_node *, sizeof...(Args)>{{meta_info<Args>::resolve()...}}[index];
    }
};


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...) const>: meta_function_helper<Ret(Args...)> {
    static constexpr auto is_const = true;
};


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(Class:: *)(Args...));


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...) const>
to_meta_function_helper(Ret(Class:: *)(Args...) const);


template<typename Ret, typename... Args>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(*)(Args...));


constexpr void to_meta_function_helper(...);


template<typename Candidate>
using meta_function_helper_t = decltype(to_meta_function_helper(std::declval<Candidate>()));


template<typename Type, typename... Args, std::size_t... Indexes>
meta_any construct(meta_any * const args, std::index_sequence<Indexes...>) {
    [[maybe_unused]] auto direct = std::make_tuple((args+Indexes)->try_cast<Args>()...);
    meta_any any{};

    if(((std::get<Indexes>(direct) || (args+Indexes)->convert<Args>()) && ...)) {
        any = Type{(std::get<Indexes>(direct) ? *std::get<Indexes>(direct) : (args+Indexes)->cast<Args>())...};
    }

    return any;
}


template<bool Const, typename Type, auto Data>
bool setter([[maybe_unused]] meta_any instance, [[maybe_unused]] meta_any index, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(!Const) {
        if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using helper_type = meta_function_helper_t<decltype(Data)>;
            using data_type = std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>;
            static_assert(std::is_invocable_v<decltype(Data), Type &, data_type>);
            auto * const clazz = instance.try_cast<Type>();
            auto * const direct = value.try_cast<data_type>();

            if(clazz && (direct || value.convert<data_type>())) {
                std::invoke(Data, *clazz, direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
            static_assert(std::is_invocable_v<decltype(Data), Type *>);
            auto * const clazz = instance.try_cast<Type>();

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                auto * const direct = value.try_cast<underlying_type>();
                auto * const idx = index.try_cast<std::size_t>();

                if(clazz && idx && (direct || value.convert<underlying_type>())) {
                    std::invoke(Data, clazz)[*idx] = direct ? *direct : value.cast<underlying_type>();
                    accepted = true;
                }
            } else {
                auto * const direct = value.try_cast<data_type>();

                if(clazz && (direct || value.convert<data_type>())) {
                    std::invoke(Data, clazz) = (direct ? *direct : value.cast<data_type>());
                    accepted = true;
                }
            }
        } else {
            static_assert(std::is_pointer_v<decltype(Data)>);
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(*Data)>>;

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                auto * const direct = value.try_cast<underlying_type>();
                auto * const idx = index.try_cast<std::size_t>();

                if(idx && (direct || value.convert<underlying_type>())) {
                    (*Data)[*idx] = (direct ? *direct : value.cast<underlying_type>());
                    accepted = true;
                }
            } else {
                auto * const direct = value.try_cast<data_type>();

                if(direct || value.convert<data_type>()) {
                    *Data = (direct ? *direct : value.cast<data_type>());
                    accepted = true;
                }
            }
        }
    }

    return accepted;
}


template<typename Type, auto Data, typename Policy>
meta_any getter([[maybe_unused]] meta_any instance, [[maybe_unused]] meta_any index) {
    auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>, std::forward<decltype(value)>(value)};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::forward<decltype(value)>(value))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
        static_assert(std::is_invocable_v<decltype(Data), Type &>);
        auto * const clazz = instance.try_cast<Type>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
        static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto * const clazz = instance.try_cast<Type>();

        if constexpr(std::is_array_v<data_type>) {
            auto * const idx = index.try_cast<std::size_t>();
            return (clazz && idx) ? dispatch(std::invoke(Data, clazz)[*idx]) : meta_any{};
        } else {
            return clazz ? dispatch(std::invoke(Data, clazz)) : meta_any{};
        }
    } else {
        static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);

        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            auto * const idx = index.try_cast<std::size_t>();
            return idx ? dispatch((*Data)[*idx]) : meta_any{};
        } else {
            return dispatch(*Data);
        }
    }
}


template<typename Type, auto Candidate, typename Policy, std::size_t... Indexes>
meta_any invoke([[maybe_unused]] meta_any instance, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper_t<decltype(Candidate)>;

    auto dispatch = [](auto *... params) {
        if constexpr(std::is_void_v<typename helper_type::return_type> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, *params...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::invoke(Candidate, *params...))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::invoke(Candidate, *params...)};
        }
    };

    [[maybe_unused]] const auto direct = std::make_tuple([](meta_any *any, auto *value) {
        using arg_type = std::remove_reference_t<decltype(*value)>;

        if(!value && any->convert<arg_type>()) {
            value = any->try_cast<arg_type>();
        }

        return value;
    }(args+Indexes, (args+Indexes)->try_cast<std::tuple_element_t<Indexes, typename helper_type::args_type>>())...);

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Candidate)>>>) {
        return (std::get<Indexes>(direct) && ...) ? dispatch(std::get<Indexes>(direct)...) : meta_any{};
    } else {
        auto * const clazz = instance.try_cast<Type>();
        return (clazz && (std::get<Indexes>(direct) && ...)) ? dispatch(clazz, std::get<Indexes>(direct)...) : meta_any{};
    }
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Meta factory to be used for reflection purposes.
 *
 * The meta factory is an utility class used to reflect types, data members and
 * functions of all sorts. This class ensures that the underlying web of types
 * is built correctly and performs some checks in debug mode to ensure that
 * there are no subtle errors at runtime.
 */
template<typename...>
class meta_factory;


/**
 * @brief Extended meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 * @tparam Spec Property specialization pack used to disambiguate overloads.
 */
template<typename Type, typename... Spec>
class meta_factory<Type, Spec...>: public meta_factory<Type> {
    bool exists(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node && (node->key() == key || exists(key, node->next));
    }

    template<std::size_t Step = 0, std::size_t... Index, typename... Property, typename... Other>
    void unpack(std::index_sequence<Index...>, std::tuple<Property...> property, Other &&... other) {
        unroll<Step>(choice<3>, std::move(std::get<Index>(property))..., std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<3>, std::tuple<Property...> property, Other &&... other) {
        unpack<Step>(std::index_sequence_for<Property...>{}, std::move(property), std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<2>, std::pair<Property...> property, Other &&... other) {
        assign<Step>(std::move(property.first), std::move(property.second));
        unroll<Step+1>(choice<3>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename Property, typename... Other>
    std::enable_if_t<!std::is_invocable_v<Property>>
    unroll(choice_t<1>, Property &&property, Other &&... other) {
        assign<Step>(std::forward<Property>(property));
        unroll<Step+1>(choice<3>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename Func, typename... Other>
    void unroll(choice_t<0>, Func &&invocable, Other &&... other) {
        unroll<Step>(choice<3>, std::forward<Func>(invocable)(), std::forward<Other>(other)...);
    }

    template<std::size_t>
    void unroll(choice_t<0>) {}

    template<std::size_t = 0, typename Key, typename... Value>
    void assign(Key &&key, Value &&... value) {
        static const auto property{std::make_tuple(std::forward<Key>(key), std::forward<Value>(value)...)};

        static internal::meta_prop_node node{
            nullptr,
            []() -> meta_any {
                return std::get<0>(property);
            },
            []() -> meta_any {
                if constexpr(sizeof...(Value) == 0) {
                    return {};
                } else {
                    return std::get<1>(property);
                }
            }
        };

        ENTT_ASSERT(!exists(node.key(), *curr));
        node.next = *curr;
        *curr = &node;
    }

public:
    /**
     * @brief Constructs an extended factory from a given node.
     * @param target The underlying node to which to assign the properties.
     */
    meta_factory(entt::internal::meta_prop_node **target) ENTT_NOEXCEPT
        : curr{target}
    {}

    /**
     * @brief Assigns a property to the last meta object created.
     *
     * Both the key and the value (if any) must be at least copy constructible.
     *
     * @tparam PropertyOrKey Type of the property or property key.
     * @tparam Value Optional type of the property value.
     * @param property_or_key Property or property key.
     * @param value Optional property value.
     * @return A meta factory for the parent type.
     */
    template<typename PropertyOrKey, typename... Value>
    auto prop(PropertyOrKey &&property_or_key, Value &&... value) && {
        if constexpr(sizeof...(Value) == 0) {
            unroll(choice<3>, std::forward<PropertyOrKey>(property_or_key));
        } else {
            assign(std::forward<PropertyOrKey>(property_or_key), std::forward<Value>(value)...);
        }

        return meta_factory<Type, Spec..., PropertyOrKey, Value...>{curr};
    }

    /**
     * @brief Assigns properties to the last meta object created.
     *
     * Both the keys and the values (if any) must be at least copy
     * constructible.
     *
     * @tparam Property Types of the properties.
     * @param property Properties to assign to the last meta object created.
     * @return A meta factory for the parent type.
     */
    template <typename... Property>
    auto props(Property... property) && {
        unroll(choice<3>, std::forward<Property>(property)...);
        return meta_factory<Type, Spec..., Property...>{curr};
    }

private:
    entt::internal::meta_prop_node **curr;
};


/**
 * @brief Basic meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory<Type> {
    template<typename Node>
    bool exists(const Node *candidate, const Node *node) ENTT_NOEXCEPT {
        return node && (node == candidate || exists(candidate, node->next));
    }

    template<typename Node>
    bool exists(const id_type id, const Node *node) ENTT_NOEXCEPT {
        return node && (node->id == id || exists(id, node->next));
    }

public:
    /**
     * @brief Makes a meta type _searchable_.
     * @param id Optional unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto type(const id_type id = type_info<Type>::id()) {
        auto * const node = internal::meta_info<Type>::resolve();

        ENTT_ASSERT(!exists(id, *internal::meta_context::global));
        ENTT_ASSERT(!exists(node, *internal::meta_context::global));
        node->id = id;
        node->next = *internal::meta_context::global;
        *internal::meta_context::global = node;

        return meta_factory<Type, Type>{&node->prop};
    }

    /*! @copydoc type */
    [[deprecated("use ::type instead")]]
    auto alias(const id_type id) ENTT_NOEXCEPT {
        return type(id);
    }

    /**
     * @brief Assigns a meta base to a meta type.
     *
     * A reflected base class must be a real base class of the reflected type.
     *
     * @tparam Base Type of the base class to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename Base>
    auto base() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<Base, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](void *instance) ENTT_NOEXCEPT -> void * {
                return static_cast<Base *>(static_cast<Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->base));
        node.next = type->base;
        type->base = &node;

        return meta_factory<Type>{};
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
    auto conv() ENTT_NOEXCEPT {
        static_assert(std::is_convertible_v<Type, To>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            type,
            nullptr,
            &internal::meta_info<To>::resolve,
            [](const void *instance) -> meta_any {
                return static_cast<To>(*static_cast<const Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->conv));
        node.next = type->conv;
        type->conv = &node;

        return meta_factory<Type>{};
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * Conversion functions can be either free functions or member
     * functions.<br/>
     * In case of free functions, they must accept a const reference to an
     * instance of the parent type as an argument. In case of member functions,
     * they should have no arguments at all.
     *
     * @tparam Candidate The actual function to use for the conversion.
     * @return A meta factory for the parent type.
     */
    template<auto Candidate>
    auto conv() ENTT_NOEXCEPT {
        using conv_type = std::invoke_result_t<decltype(Candidate), Type &>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            type,
            nullptr,
            &internal::meta_info<conv_type>::resolve,
            [](const void *instance) -> meta_any {
                return std::invoke(Candidate, *static_cast<const Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->conv));
        node.next = type->conv;
        type->conv = &node;

        return meta_factory<Type>{};
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of constructors.
     * All that is required is that they return an instance of the underlying
     * type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or a free function.
     *
     * @tparam Func The actual function to use as a constructor.
     * @tparam Policy Optional policy (no policy set by default).
     * @return An extended meta factory for the parent type.
     */
    template<auto Func, typename Policy = as_is_t>
    auto ctor() ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Func)>;
        static_assert(std::is_same_v<typename helper_type::return_type, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func, Policy>({}, any, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

        return meta_factory<Type, std::integral_constant<decltype(Func), Func>>{&node.prop};
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * A meta constructor is uniquely identified by the types of its arguments
     * and is such that there exists an actual constructor of the underlying
     * type that can be invoked with parameters whose types are those given.
     *
     * @tparam Args Types of arguments to use to construct an instance.
     * @return An extended meta factory for the parent type.
     */
    template<typename... Args>
    auto ctor() ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<Type(*)(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, std::remove_cv_t<std::remove_reference_t<Args>>...>(any, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

        return meta_factory<Type, Type(Args...)>{&node.prop};
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of destructors.
     * The signature of the function should identical to the following:
     *
     * @code{.cpp}
     * void(Type &);
     * @endcode
     *
     * The purpose is to give users the ability to free up resources that
     * require special treatment before an object is actually destroyed.
     *
     * @tparam Func The actual function to use as a destructor.
     * @return A meta factory for the parent type.
     */
    template<auto Func>
    auto dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type &>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_dtor_node node{
            type,
            [](void *instance) {
                if(instance) {
                    std::invoke(Func, *static_cast<Type *>(instance));
                }
            }
        };

        ENTT_ASSERT(!type->dtor);
        type->dtor = &node;

        return meta_factory<Type>{};
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
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_is_t>
    auto data(const id_type id) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::resolve();
        internal::meta_data_node *curr = nullptr;

        if constexpr(std::is_same_v<Type, decltype(Data)>) {
            static_assert(std::is_same_v<Policy, as_is_t>);

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                true,
                true,
                &internal::meta_info<Type>::resolve,
                [](meta_any, meta_any, meta_any) { return false; },
                [](meta_any, meta_any) -> meta_any { return Data; }
            };

            curr = &node;
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_reference_t<decltype(std::declval<Type>().*Data)>;

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data, Policy>
            };

            curr = &node;
        } else {
            static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);
            using data_type = std::remove_pointer_t<std::decay_t<decltype(Data)>>;

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data, Policy>
            };

            curr = &node;
        }

        ENTT_ASSERT(!exists(id, type->data));
        ENTT_ASSERT(!exists(curr, type->data));
        curr->id = id;
        curr->next = type->data;
        type->data = curr;

        return meta_factory<Type, std::integral_constant<decltype(Data), Data>>{&curr->prop};
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     *
     * Setters and getters can be either free functions, member functions or a
     * mix of them.<br/>
     * In case of free functions, setters and getters must accept a reference to
     * an instance of the parent type as their first argument. A setter has then
     * an extra argument of a type convertible to that of the parameter to
     * set.<br/>
     * In case of member functions, getters have no arguments at all, while
     * setters has an argument of a type convertible to that of the parameter to
     * set.
     *
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_is_t>
    auto data(const id_type id) ENTT_NOEXCEPT {
        using underlying_type = std::invoke_result_t<decltype(Getter), Type &>;
        static_assert(std::is_invocable_v<decltype(Setter), Type &, underlying_type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_data_node node{
            {},
            type,
            nullptr,
            nullptr,
            false,
            false,
            &internal::meta_info<underlying_type>::resolve,
            &internal::setter<false, Type, Setter>,
            &internal::getter<Type, Getter, Policy>
        };

        ENTT_ASSERT(!exists(id, type->data));
        ENTT_ASSERT(!exists(&node, type->data));
        node.id = id;
        node.next = type->data;
        type->data = &node;

        return meta_factory<Type, std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
    }

    /**
     * @brief Assigns a meta funcion to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Candidate The actual function to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto func(const id_type id) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            {},
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            helper_type::is_const,
            !std::is_member_function_pointer_v<decltype(Candidate)>,
            &internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename helper_type::return_type>>::resolve,
            &helper_type::arg,
            [](meta_any instance, meta_any *args) {
                return internal::invoke<Type, Candidate, Policy>(std::move(instance), args, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(id, type->func));
        ENTT_ASSERT(!exists(&node, type->func));
        node.id = id;
        node.next = type->func;
        type->func = &node;

        return meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
    }

    /**
     * @brief Resets a meta type and all its parts.
     *
     * This function resets a meta type and all its data members, member
     * functions and properties, as well as its constructors, destructors and
     * conversion functions if any.<br/>
     * Base classes aren't reset but the link between the two types is removed.
     *
     * @return An extended meta factory for the given type.
     */
    auto reset() ENTT_NOEXCEPT {
        auto * const node = internal::meta_info<Type>::resolve();

        internal::meta_context::detach(node);

        const auto unregister_all = y_combinator{
            [](auto &&self, auto **curr, auto... member) {
                while(*curr) {
                    auto *prev = *curr;
                    (self(&(prev->*member)), ...);
                    *curr = prev->next;
                    prev->next = nullptr;
                }
            }
        };

        unregister_all(&node->prop);
        unregister_all(&node->base);
        unregister_all(&node->conv);
        unregister_all(&node->ctor, &internal::meta_ctor_node::prop);
        unregister_all(&node->data, &internal::meta_data_node::prop);
        unregister_all(&node->func, &internal::meta_func_node::prop);

        node->id = {};
        node->next = nullptr;
        node->dtor = nullptr;

        return meta_factory<Type, Type>{&node->prop};
    }
};


/**
 * @brief Utility function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach meta objects through a
 * dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @return An meta factory for the given type.
 */
template<typename Type>
inline meta_factory<Type> meta() ENTT_NOEXCEPT {
    auto * const node = internal::meta_info<Type>::resolve();
    // extended meta factory to allow assigning properties to opaque meta types
    return meta_factory<Type, Type>{&node->prop};
}


}


#endif

// #include "meta/meta.hpp"

// #include "meta/resolve.hpp"
#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP


#include <type_traits>
// #include "meta.hpp"



namespace entt {


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve();
}


/**
 * @brief Returns the first meta type that satisfies specific criteria, if any.
 * @tparam Func Type of the unary predicate to use to test the meta types.
 * @param func Unary predicate which returns ​true for the required element.
 * @return The first meta type satisfying the condition, if any.
 */
template<typename Func>
inline meta_type resolve_if(Func func) ENTT_NOEXCEPT {
    return internal::find_if([&func](const auto *curr) {
        return func(meta_type{curr});
    }, *internal::meta_context::global);
}


/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
inline meta_type resolve_id(const id_type id) ENTT_NOEXCEPT {
    return resolve_if([id](const auto type) { return type.id() == id; });
}


/*! @copydoc resolve_id */
[[deprecated("use entt::resolve_id instead")]]
inline meta_type resolve(const id_type id) ENTT_NOEXCEPT {
    return resolve_id(id);
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
inline std::enable_if_t<std::is_invocable_v<Op, meta_type>, void>
resolve(Op op) {
    internal::visit<meta_type>(op, *internal::meta_context::global);
}


}


#endif

// #include "meta/policy.hpp"

// #include "process/process.hpp"
#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP


#include <utility>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>
// #include "../config/config.h"

// #include "fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif



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
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    using type = std::uint64_t;
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr id_type helper(const Char *curr) ENTT_NOEXCEPT {
        auto value = traits_type::offset;

        while(*curr != 0) {
            value = (value ^ static_cast<traits_type::type>(*(curr++))) * traits_type::prime;
        }

        return value;
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
    /*! @brief Unsigned integer type. */
    using hash_type = id_type;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        id_type partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * basic_hashed_string<char> hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const value_type * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /*! @copydoc data */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return data(); }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return value(); }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const basic_hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const value_type *str;
    hash_type hash;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the character type of the hashed string directly from a
 * human-readable identifer provided to the constructor.
 *
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifer.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) ENTT_NOEXCEPT
-> basic_hashed_string<Char>;


/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;


/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
constexpr entt::hashed_wstring operator"" ENTT_HWS_SUFFIX(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_wstring{str};
}


#endif

// #include "fwd.hpp"



namespace entt {


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
struct is_equality_comparable<Type, std::void_t<decltype(std::declval<Type>() == std::declval<Type>())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially equality comparable type.
 */
template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;


/**
 * @brief Extracts the class of a non-static member object or function.
 * @tparam Member A pointer to a non-static member object or function.
 */
template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>);

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


/**
 * @brief Defines an enum class to use for opaque identifiers and a dedicate
 * `to_integer` function to convert the identifiers to their underlying type.
 * @param clazz The name to use for the enum class.
 * @param type The underlying type for the enum class.
 */
#define ENTT_OPAQUE_TYPE(clazz, type)\
    enum class clazz: type {};\
    constexpr auto to_integral(const clazz id) ENTT_NOEXCEPT {\
        return static_cast<std::underlying_type_t<clazz>>(id);\
    }\
    static_assert(true)


#endif



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

    template<typename Target = Derived>
    auto next(integral_constant<state::UNINITIALIZED>)
    -> decltype(std::declval<Target>().init()) {
        static_cast<Target *>(this)->init();
    }

    template<typename Target = Derived>
    auto next(integral_constant<state::RUNNING>, Delta delta, void *data)
    -> decltype(std::declval<Target>().update(delta, data)) {
        static_cast<Target *>(this)->update(delta, data);
    }

    template<typename Target = Derived>
    auto next(integral_constant<state::SUCCEEDED>)
    -> decltype(std::declval<Target>().succeeded()) {
        static_cast<Target *>(this)->succeeded();
    }

    template<typename Target = Derived>
    auto next(integral_constant<state::FAILED>)
    -> decltype(std::declval<Target>().failed()) {
        static_cast<Target *>(this)->failed();
    }

    template<typename Target = Derived>
    auto next(integral_constant<state::ABORTED>)
    -> decltype(std::declval<Target>().aborted()) {
        static_cast<Target *>(this)->aborted();
    }

    void next(...) const ENTT_NOEXCEPT {}

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
    virtual ~process() {
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
    void abort(const bool immediately = false) {
        if(alive()) {
            current = state::ABORTED;

            if(immediately) {
                tick({});
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
            next(integral_constant<state::UNINITIALIZED>{});
            current = state::RUNNING;
            break;
        case state::RUNNING:
            next(integral_constant<state::RUNNING>{}, delta, data);
            break;
        default:
            // suppress warnings
            break;
        }

        // if it's dead, it must be notified and removed immediately
        switch(current) {
        case state::SUCCEEDED:
            next(integral_constant<state::SUCCEEDED>{});
            current = state::FINISHED;
            break;
        case state::FAILED:
            next(integral_constant<state::FAILED>{});
            current = state::FINISHED;
            stopped = true;
            break;
        case state::ABORTED:
            next(integral_constant<state::ABORTED>{});
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


#endif

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
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    scheduler() = default;

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

        for(auto &&handler: exec) {
            handler.abort(handler, immediately);
        }

        std::move(handlers.begin(), handlers.end(), std::back_inserter(exec));
        handlers.swap(exec);
    }

private:
    std::vector<process_handler> handlers{};
};


}


#endif

// #include "resource/cache.hpp"
#ifndef ENTT_RESOURCE_CACHE_HPP
#define ENTT_RESOURCE_CACHE_HPP


#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif

// #include "../core/fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif

// #include "handle.hpp"
#ifndef ENTT_RESOURCE_HANDLE_HPP
#define ENTT_RESOURCE_HANDLE_HPP


#include <memory>
#include <utility>
// #include "../config/config.h"

// #include "fwd.hpp"
#ifndef ENTT_RESOURCE_FWD_HPP
#define ENTT_RESOURCE_FWD_HPP


namespace entt {


/*! @struct cache */
template<typename>
struct cache;

/*! @class handle */
template<typename>
class handle;

/*! @class loader */
template<typename, typename>
class loader;


}


#endif



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
class handle {
    /*! @brief Resource handles are friends of their caches. */
    friend struct cache<Resource>;

    handle(std::shared_ptr<Resource> res) ENTT_NOEXCEPT
        : resource{std::move(res)}
    {}

public:
    /*! @brief Default constructor. */
    handle() ENTT_NOEXCEPT = default;

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
    operator const Resource & () const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    operator Resource & () ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    const Resource & operator *() const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    Resource & operator *() ENTT_NOEXCEPT { return get(); }

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
    const Resource * operator->() const ENTT_NOEXCEPT {
        ENTT_ASSERT(static_cast<bool>(resource));
        return resource.get();
    }

    /*! @copydoc operator-> */
    Resource * operator->() ENTT_NOEXCEPT {
        return const_cast<Resource *>(std::as_const(*this).operator->());
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(resource);
    }

private:
    std::shared_ptr<Resource> resource;
};


}


#endif

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
 * struct my_loader: entt::loader<my_loader, my_resource> {
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
class loader {
    /*! @brief Resource loaders are friends of their caches. */
    friend struct cache<Resource>;

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


#endif

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
struct cache {
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Type of resources managed by a cache. */
    using resource_type = Resource;

    /*! @brief Default constructor. */
    cache() = default;

    /*! @brief Default move constructor. */
    cache(cache &&) = default;

    /*! @brief Default move assignment operator. @return This cache. */
    cache & operator=(cache &&) = default;

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
    entt::handle<Resource> load(const id_type id, Args &&... args) {
        static_assert(std::is_base_of_v<loader<Loader, Resource>, Loader>);
        entt::handle<Resource> resource{};

        if(auto it = resources.find(id); it == resources.cend()) {
            if(auto instance = Loader{}.get(std::forward<Args>(args)...); instance) {
                resources[id] = instance;
                resource = std::move(instance);
            }
        } else {
            resource = it->second;
        }

        return resource;
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
    entt::handle<Resource> reload(const id_type id, Args &&... args) {
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
    entt::handle<Resource> temp(Args &&... args) const {
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
     * @sa handle
     *
     * @param id Unique resource identifier.
     * @return A handle for the given resource.
     */
    entt::handle<Resource> handle(const id_type id) const {
        auto it = resources.find(id);
        return { it == resources.end() ? nullptr : it->second };
    }

    /**
     * @brief Checks if a cache contains a given identifier.
     * @param id Unique resource identifier.
     * @return True if the cache contains the resource, false otherwise.
     */
    bool contains(const id_type id) const {
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
    void discard(const id_type id) {
        if(auto it = resources.find(id); it != resources.end()) {
            resources.erase(it);
        }
    }

    /**
     * @brief Iterates all resources.
     *
     * The function object is invoked for each element. It is provided with
     * either the resource identifier, the resource handle or both of them.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const id_type);
     * void(handle<Resource>);
     * void(const id_type, handle<Resource>);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template <typename Func>
    void each(Func func) const {
        auto begin = resources.begin();
        auto end = resources.end();

        while(begin != end) {
            auto curr = begin++;

            if constexpr(std::is_invocable_v<Func, id_type>) {
                func(curr->first);
            } else if constexpr(std::is_invocable_v<Func, entt::handle<Resource>>) {
                func(entt::handle{ curr->second });
            } else {
                func(curr->first, entt::handle{ curr->second });
            }
        }
    }

private:
    std::unordered_map<id_type, std::shared_ptr<Resource>> resources;
};


}


#endif

// #include "resource/handle.hpp"

// #include "resource/loader.hpp"

// #include "signal/delegate.hpp"
#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <tuple>
#include <cstddef>
#include <utility>
#include <functional>
#include <type_traits>
// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Ret, typename... Args>
auto function_pointer(Ret(*)(Args...)) -> Ret(*)(Args...);


template<typename Ret, typename Type, typename... Args, typename Other>
auto function_pointer(Ret(*)(Type, Args...), Other &&) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args, typename... Other>
auto function_pointer(Ret(Class:: *)(Args...), Other &&...) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args, typename... Other>
auto function_pointer(Ret(Class:: *)(Args...) const, Other &&...) -> Ret(*)(Args...);


template<typename Class, typename Type, typename... Other>
auto function_pointer(Type Class:: *, Other &&...) -> Type(*)();


template<typename... Type>
using function_pointer_t = decltype(internal::function_pointer(std::declval<Type>()...));


template<typename... Class, typename Ret, typename... Args>
constexpr auto index_sequence_for(Ret(*)(Args...)) {
    return std::index_sequence_for<Class..., Args...>{};
}


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
inline constexpr connect_arg_t<Func> connect_arg{};


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
 * A delegate can be used as a general purpose invoker without memory overhead
 * for free functions possibly with payloads and bound or unbound members.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using proto_fn_type = Ret(const void *, Args...);

    template<auto Candidate, std::size_t... Index>
    auto wrap(std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            return Ret(std::invoke(Candidate, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    auto wrap(Type &, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *payload, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            return Ret(std::invoke(Candidate, *curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    auto wrap(Type *, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        return [](const void *payload, Args... args) -> Ret {
            const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            return Ret(std::invoke(Candidate, curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(arguments))...));
        };
    }

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : fn{nullptr}, data{nullptr}
    {}

    /**
     * @brief Constructs a delegate and connects a free function or an unbound
     * member.
     * @tparam Candidate Function or member to connect to the delegate.
     */
    template<auto Candidate>
    delegate(connect_arg_t<Candidate>) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>();
    }

    /**
     * @brief Constructs a delegate and connects a free function with payload or
     * a bound member.
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type &&value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(std::forward<Type>(value_or_instance));
    }

    /**
     * @brief Connects a free function or an unbound member to a delegate.
     * @tparam Candidate Function or member to connect to the delegate.
     */
    template<auto Candidate>
    void connect() ENTT_NOEXCEPT {
        data = nullptr;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>) {
            fn = [](const void *, Args... args) -> Ret {
                return Ret(std::invoke(Candidate, std::forward<Args>(args)...));
            };
        } else if constexpr(std::is_member_pointer_v<decltype(Candidate)>) {
            fn = wrap<Candidate>(internal::index_sequence_for<std::tuple_element_t<0, std::tuple<Args...>>>(internal::function_pointer_t<decltype(Candidate)>{}));
        } else {
            fn = wrap<Candidate>(internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate)>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) ENTT_NOEXCEPT {
        data = &value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, Args...>) {
            fn = [](const void *payload, Args... args) -> Ret {
                Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
                return Ret(std::invoke(Candidate, *curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * @sa connect(Type &)
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) ENTT_NOEXCEPT {
        data = value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>) {
            fn = [](const void *payload, Args... args) -> Ret {
                Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
                return Ret(std::invoke(Candidate, curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
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
     * @brief Returns the instance or the payload linked to a delegate, if any.
     * @return An opaque pointer to the underlying data.
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
        return fn(data, std::forward<Args>(args)...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also data
        return !(fn == nullptr);
    }

    /**
     * @brief Compares the contents of two delegates.
     * @param other Delegate with which to compare.
     * @return False if the two contents differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn && data == other.data;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Compares the contents of two delegates.
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the two contents differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 */
template<auto Candidate>
delegate(connect_arg_t<Candidate>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate)>>>;


/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &&) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate), Type>>>;


}


#endif

// #include "signal/dispatcher.hpp"
#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP


#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// #include "../config/config.h"

// #include "../core/fwd.hpp"
#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP


// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (false && std::is_empty_v<Type>)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined _MSC_VER
#      define ENTT_PRETTY_FUNCTION __FUNCSIG__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#      define ENTT_PRETTY_FUNCTION_CONSTEXPR ENTT_PRETTY_FUNCTION
#   elif defined __GNUC__
#      define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#   endif
#endif


#endif



namespace entt {


/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;


}


#endif

// #include "../core/type_info.hpp"
#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


// #include "../config/config.h"

// #include "../core/attribute.h"
#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

// #include "hashed_string.hpp"
#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>
// #include "../config/config.h"

// #include "fwd.hpp"



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
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};


template<>
struct fnv1a_traits<std::uint64_t> {
    using type = std::uint64_t;
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr id_type helper(const Char *curr) ENTT_NOEXCEPT {
        auto value = traits_type::offset;

        while(*curr != 0) {
            value = (value ^ static_cast<traits_type::type>(*(curr++))) * traits_type::prime;
        }

        return value;
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
    /*! @brief Unsigned integer type. */
    using hash_type = id_type;

    /**
     * @brief Returns directly the numeric representation of a string.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        id_type partial{traits_type::offset};
        while(size--) { partial = (partial^(str++)[0])*traits_type::prime; }
        return partial;
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : str{nullptr}, hash{}
    {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * basic_hashed_string<char> hs{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param curr Human-readable identifer.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&curr)[N]) ENTT_NOEXCEPT
        : str{curr}, hash{helper(curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(wrapper.str)}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr const value_type * data() const ENTT_NOEXCEPT {
        return str;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr hash_type value() const ENTT_NOEXCEPT {
        return hash;
    }

    /*! @copydoc data */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return data(); }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return value(); }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const basic_hashed_string &other) const ENTT_NOEXCEPT {
        return hash == other.hash;
    }

private:
    const value_type *str;
    hash_type hash;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the character type of the hashed string directly from a
 * human-readable identifer provided to the constructor.
 *
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifer.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) ENTT_NOEXCEPT
-> basic_hashed_string<Char>;


/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;


/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;


}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
constexpr entt::hashed_string operator"" ENTT_HS_SUFFIX(const char *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_string{str};
}


/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
constexpr entt::hashed_wstring operator"" ENTT_HWS_SUFFIX(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return entt::hashed_wstring{str};
}


#endif

// #include "fwd.hpp"



namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_index {
    static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Type index.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_index::next();
        return value;
    }
};


/**
 * @brief Provides the member constant `value` to true if a given type is
 * indexable, false otherwise.
 * @tparam Type Potentially indexable type.
 */
template<typename, typename = void>
struct has_type_index: std::false_type {};


/*! @brief has_type_index */
template<typename Type>
struct has_type_index<Type, std::void_t<decltype(type_index<Type>::value())>>: std::true_type {};


/**
 * @brief Helper variable template.
 * @tparam Type Potentially indexable type.
 */
template<typename Type>
inline constexpr bool has_type_index_v = has_type_index<Type>::value;


/**
 * @brief Type info.
 * @tparam Type Type for which to generate information.
 */
template<typename Type, typename = void>
struct ENTT_API type_info {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    static constexpr id_type id() ENTT_NOEXCEPT {
        constexpr auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION_CONSTEXPR);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    static id_type id() ENTT_NOEXCEPT {
        static const auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    static id_type id() ENTT_NOEXCEPT {
        return type_index<Type>::value();
    }
#endif
};


}


#endif

// #include "sigh.hpp"
#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>
// #include "../config/config.h"

// #include "delegate.hpp"

// #include "fwd.hpp"
#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP


namespace entt {


/*! @class delegate */
template<typename>
class delegate;

/*! @class dispatcher */
class dispatcher;

/*! @class emitter */
template<typename>
class emitter;

/*! @class connection */
class connection;

/*! @class scoped_connection */
struct scoped_connection;

/*! @class sink */
template<typename>
class sink;

/*! @class sigh */
template<typename>
class sigh;


}


#endif



namespace entt {


/**
 * @brief Sink class.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sigh;


/**
 * @brief Unmanaged signal handler.
 *
 * It works directly with references to classes and pointers to member functions
 * as well as pointers to free functions. Users of this class are in charge of
 * disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals to use later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sigh<Ret(Args...)> {
    /*! @brief A sink is allowed to modify a signal. */
    friend class sink<Ret(Args...)>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
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
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) const {
        for(auto &&call: std::as_const(calls)) {
            call(args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     *
     * The collector must expose a call operator with the following properties:
     *
     * * The return type is either `void` or such that it's convertible to
     *   `bool`. In the second case, a true value will stop the iteration.
     * * The list of parameters is empty if `Ret` is `void`, otherwise it
     *   contains a single element such that `Ret` is convertible to it.
     *
     * @tparam Func Type of collector to use, if any.
     * @param func A valid function object.
     * @param args Arguments to use to invoke listeners.
     */
    template<typename Func>
    void collect(Func func, Args... args) const {
        for(auto &&call: calls) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    call(args...);
                    if(func()) { break; }
                } else {
                    call(args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    if(func(call(args...))) { break; }
                } else {
                    func(call(args...));
                }
            }
        }
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


/**
 * @brief Connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.
 */
class connection {
    /*! @brief A sink is allowed to create connection objects. */
    template<typename>
    friend class sink;

    connection(delegate<void(void *)> fn, void *ref)
        : disconnect{fn}, signal{ref}
    {}

public:
    /*! @brief Default constructor. */
    connection() = default;

    /**
     * @brief Checks whether a connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(disconnect);
    }

    /*! @brief Breaks the connection. */
    void release() {
        if(disconnect) {
            disconnect(signal);
            disconnect.reset();
        }
    }

private:
    delegate<void(void *)> disconnect;
    void *signal{};
};


/**
 * @brief Scoped connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.<br/>
 * A scoped connection automatically breaks the link between the two objects
 * when it goes out of scope.
 */
struct scoped_connection {
    /*! @brief Default constructor. */
    scoped_connection() = default;

    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param other A valid connection object.
     */
    scoped_connection(const connection &other)
        : conn{other}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        conn.release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const scoped_connection &) = delete;

    /**
     * @brief Acquires a connection.
     * @param other The connection object to acquire.
     * @return This scoped connection.
     */
    scoped_connection & operator=(connection other) {
        conn = std::move(other);
        return *this;
    }

    /**
     * @brief Checks whether a scoped connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(conn);
    }

    /*! @brief Breaks the connection. */
    void release() {
        conn.release();
    }

private:
    connection conn;
};


/**
 * @brief Sink class.
 *
 * A sink is used to connect listeners to signals and to disconnect them.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of the class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    using signal_type = sigh<Ret(Args...)>;
    using difference_type = typename std::iterator_traits<typename decltype(signal_type::calls)::iterator>::difference_type;

    template<auto Candidate, typename Type>
    static void release(Type value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Candidate>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>();
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : offset{},
          signal{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return signal->calls.empty();
    }

    /**
     * @brief Returns a sink that connects before a given free function or an
     * unbound member.
     * @tparam Function A valid free function pointer.
     * @return A properly initialized sink object.
     */
    template<auto Function>
    sink before() {
        delegate<Ret(Args...)> call{};
        call.template connect<Function>();

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a free function with payload
     * or a bound member.
     * @tparam Candidate Member or free function to look for.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<auto Candidate, typename Type>
    sink before(Type &&value_or_instance) {
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(std::forward<Type>(value_or_instance));

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    sink before(Type &value_or_instance) {
        return before(&value_or_instance);
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    sink before(Type *value_or_instance) {
        sink other{*this};

        if(value_or_instance) {
            const auto &calls = signal->calls;
            const auto it = std::find_if(calls.cbegin(), calls.cend(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            });

            other.offset = std::distance(it, calls.cend());
        }

        return other;
    }

    /**
     * @brief Returns a sink that connects before anything else.
     * @return A properly initialized sink object.
     */
    sink before() {
        sink other{*this};
        other.offset = signal->calls.size();
        return other;
    }

    /**
     * @brief Connects a free function or an unbound member to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for the
     * same function.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @return A properly initialized connection object.
     */
    template<auto Candidate>
    connection connect() {
        disconnect<Candidate>();

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate>>();
        return { std::move(conn), signal };
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one of the signal. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the signal itself.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type &&value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(value_or_instance);
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type>>(value_or_instance);
        return { std::move(conn), signal };
    }

    /**
     * @brief Disconnects a free function or an unbound member from a signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     */
    template<auto Candidate>
    void disconnect() {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects a free function with payload or a bound member from a
     * signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type &&value_or_instance) {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(std::forward<Type>(value_or_instance));
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type &value_or_instance) {
        disconnect(&value_or_instance);
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type *value_or_instance) {
        if(value_or_instance) {
            auto &calls = signal->calls;
            calls.erase(std::remove_if(calls.begin(), calls.end(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            }), calls.end());
        }
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
    difference_type offset;
    signal_type *signal;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of a sink directly from the signal it
 * refers to.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
sink(sigh<Ret(Args...)> &) ENTT_NOEXCEPT -> sink<Ret(Args...)>;


}


#endif



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
 * The dispatcher creates instances of the `sigh` class internally. Refer to the
 * documentation of the latter for more details.
 */
class dispatcher {
    struct basic_pool {
        virtual ~basic_pool() = default;
        virtual void publish() = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
        virtual id_type type_id() const ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct pool_handler final: basic_pool {
        using signal_type = sigh<void(const Event &)>;
        using sink_type = typename signal_type::sink_type;

        void publish() override {
            const auto length = events.size();

            for(std::size_t pos{}; pos < length; ++pos) {
                signal.publish(events[pos]);
            }

            events.erase(events.cbegin(), events.cbegin()+length);
        }

        void clear() ENTT_NOEXCEPT override {
            events.clear();
        }

        sink_type sink() ENTT_NOEXCEPT {
            return entt::sink{signal};
        }

        template<typename... Args>
        void trigger(Args &&... args) {
            signal.publish(Event{std::forward<Args>(args)...});
        }

        template<typename... Args>
        void enqueue(Args &&... args) {
            events.emplace_back(std::forward<Args>(args)...);
        }

        id_type type_id() const ENTT_NOEXCEPT override {
            return type_info<Event>::id();
        }

    private:
        signal_type signal{};
        std::vector<Event> events;
    };

    template<typename Event>
    pool_handler<Event> & assure() {
        static_assert(std::is_same_v<Event, std::decay_t<Event>>);

        if constexpr(has_type_index_v<Event>) {
            const auto index = type_index<Event>::value();

            if(!(index < pools.size())) {
                pools.resize(index+1);
            }

            if(!pools[index]) {
                pools[index].reset(new pool_handler<Event>{});
            }

            return static_cast<pool_handler<Event> &>(*pools[index]);
        } else {
            auto it = std::find_if(pools.begin(), pools.end(), [id = type_info<Event>::id()](const auto &cpool) { return id == cpool->type_id(); });
            return static_cast<pool_handler<Event> &>(it == pools.cend() ? *pools.emplace_back(new pool_handler<Event>{}) : **it);
        }
    }

public:
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
    auto sink() {
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
    void trigger(Args &&... args) {
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
    void trigger(Event &&event) {
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
    void enqueue(Args &&... args) {
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
    void enqueue(Event &&event) {
        assure<std::decay_t<Event>>().enqueue(std::forward<Event>(event));
    }

    /**
     * @brief Discards all the events queued so far.
     *
     * If no types are provided, the dispatcher will clear all the existing
     * pools.
     *
     * @tparam Event Type of events to discard.
     */
    template<typename... Event>
    void clear() {
        if constexpr(sizeof...(Event) == 0) {
            for(auto &&cpool: pools) {
                if(cpool) {
                    cpool->clear();
                }
            }
        } else {
            (assure<Event>().clear(), ...);
        }
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
    void update() {
        assure<Event>().publish();
    }

    /**
     * @brief Delivers all the pending events.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     */
    void update() const {
        for(auto pos = pools.size(); pos; --pos) {
            if(auto &&cpool = pools[pos-1]; cpool) {
                cpool->publish();
            }
        }
    }

private:
    std::vector<std::unique_ptr<basic_pool>> pools;
};


}


#endif

// #include "signal/emitter.hpp"
#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP


#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// #include "../config/config.h"

// #include "../core/fwd.hpp"

// #include "../core/type_info.hpp"



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
 * Pools for the type of events are created internally on the fly. It's not
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
    struct basic_pool {
        virtual ~basic_pool() = default;
        virtual bool empty() const ENTT_NOEXCEPT = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
        virtual id_type type_id() const ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct pool_handler final: basic_pool {
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
                for(auto &&element: once_list) {
                    element.first = true;
                }

                for(auto &&element: on_list) {
                    element.first = true;
                }
            } else {
                once_list.clear();
                on_list.clear();
            }
        }

        connection_type once(listener_type listener) {
            return once_list.emplace(once_list.cend(), false, std::move(listener));
        }

        connection_type on(listener_type listener) {
            return on_list.emplace(on_list.cend(), false, std::move(listener));
        }

        void erase(connection_type conn) {
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

            publishing = true;

            for(auto &&element: on_list) {
                element.first ? void() : element.second(event, ref);
            }

            for(auto &&element: swap_list) {
                element.first ? void() : element.second(event, ref);
            }

            publishing = false;

            on_list.remove_if([](auto &&element) { return element.first; });
        }

        id_type type_id() const ENTT_NOEXCEPT override {
            return type_info<Event>::id();
        }

    private:
        bool publishing{false};
        container_type once_list{};
        container_type on_list{};
    };

    template<typename Event>
    const pool_handler<Event> & assure() const {
        static_assert(std::is_same_v<Event, std::decay_t<Event>>);

        if constexpr(has_type_index_v<Event>) {
            const auto index = type_index<Event>::value();

            if(!(index < pools.size())) {
                pools.resize(index+1);
            }

            if(!pools[index]) {
                pools[index].reset(new pool_handler<Event>{});
            }

            return static_cast<pool_handler<Event> &>(*pools[index]);
        } else {
            auto it = std::find_if(pools.begin(), pools.end(), [id = type_info<Event>::id()](const auto &cpool) { return id == cpool->type_id(); });
            return static_cast<pool_handler<Event> &>(it == pools.cend() ? *pools.emplace_back(new pool_handler<Event>{}) : **it);
        }
    }

    template<typename Event>
    pool_handler<Event> & assure() {
        return const_cast<pool_handler<Event> &>(std::as_const(*this).template assure<Event>());
    }

public:
    /** @brief Type of listeners accepted for the given event. */
    template<typename Event>
    using listener = typename pool_handler<Event>::listener_type;

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
    struct connection: private pool_handler<Event>::connection_type {
        /** @brief Event emitters are friend classes of connections. */
        friend class emitter;

        /*! @brief Default constructor. */
        connection() = default;

        /**
         * @brief Creates a connection that wraps its underlying instance.
         * @param conn A connection object to wrap.
         */
        connection(typename pool_handler<Event>::connection_type conn)
            : pool_handler<Event>::connection_type{std::move(conn)}
        {}
    };

    /*! @brief Default constructor. */
    emitter() = default;

    /*! @brief Default destructor. */
    virtual ~emitter() {
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
        assure<Event>().publish(Event{std::forward<Args>(args)...}, *static_cast<Derived *>(this));
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
        return assure<Event>().on(std::move(instance));
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
        return assure<Event>().once(std::move(instance));
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
    void erase(connection<Event> conn) {
        assure<Event>().erase(std::move(conn));
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
    void clear() {
        assure<Event>().clear();
    }

    /**
     * @brief Disconnects all the listeners.
     *
     * All the connections previously returned are invalidated. Using them
     * results in undefined behavior.
     */
    void clear() ENTT_NOEXCEPT {
        for(auto &&cpool: pools) {
            if(cpool) {
                cpool->clear();
            }
        }
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Event Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Event>
    bool empty() const {
        return assure<Event>().empty();
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return std::all_of(pools.cbegin(), pools.cend(), [](auto &&cpool) {
            return !cpool || cpool->empty();
        });
    }

private:
    mutable std::vector<std::unique_ptr<basic_pool>> pools{};
};


}


#endif

// #include "signal/sigh.hpp"

