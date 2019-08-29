// #include "core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
// #include "utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


// #include "../config/config.h"
#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
 * @brief Constant utility to disambiguate overloaded member functions.
 * @tparam Type Function type of the desired overload.
 * @tparam Class Type of class to which the member functions belong.
 * @param member A valid pointer to a member function.
 * @return Pointer to the member function.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Type Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Type>
constexpr auto overload(Type *func) ENTT_NOEXCEPT { return func; }


}


#endif // ENTT_CORE_UTILITY_HPP



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


#endif // ENTT_CORE_ALGORITHM_HPP

// #include "core/family.hpp"
#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
// #include "../config/config.h"



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
    inline static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier;

    template<typename...>
    // clang (since version 9) started to complain if auto is used instead of ENTT_ID_TYPE
    inline static const ENTT_ID_TYPE inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const Char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
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
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type to_value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
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
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
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

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

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
template<ENTT_ID_TYPE>
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
template<ENTT_ID_TYPE Value>
inline monostate<Value> monostate_v = {};


}


#endif // ENTT_CORE_MONOSTATE_HPP

// #include "core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../config/config.h"

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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const Char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
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
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type to_value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
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
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
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

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

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


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


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
    }


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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
    inline static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier;

    template<typename...>
    // clang (since version 9) started to complain if auto is used instead of ENTT_ID_TYPE
    inline static const ENTT_ID_TYPE inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

// #include "../core/algorithm.hpp"
#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
// #include "utility.hpp"
#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


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
 * @brief Constant utility to disambiguate overloaded member functions.
 * @tparam Type Function type of the desired overload.
 * @tparam Class Type of class to which the member functions belong.
 * @param member A valid pointer to a member function.
 * @return Pointer to the member function.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Type Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Type>
constexpr auto overload(Type *func) ENTT_NOEXCEPT { return func; }


}


#endif // ENTT_CORE_UTILITY_HPP



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


#endif // ENTT_CORE_ALGORITHM_HPP

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../config/config.h"

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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const Char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
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
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type to_value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
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
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
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

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

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


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


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
    }


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

// #include "../signal/delegate.hpp"
#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <tuple>
#include <cstring>
#include <utility>
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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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


template<typename Ret, typename... Args, typename Type, typename Payload, typename = std::enable_if_t<std::is_convertible_v<Payload &, Type &>>>
auto to_function_pointer(Ret(*)(Type &, Args...), Payload &) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), const Class &) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, const Class &) -> Ret(*)(Args...);


template<typename Class, typename Type>
auto to_function_pointer(Type Class:: *, const Class &) -> Type(*)();


template<typename>
struct function_extent;


template<typename Ret, typename... Args>
struct function_extent<Ret(*)(Args...)> {
    static constexpr auto value = sizeof...(Args);
};


template<typename Func>
constexpr auto function_extent_v = function_extent<Func>::value;


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
    using proto_fn_type = Ret(const void *, std::tuple<Args &&...>);

    template<auto Function, std::size_t... Index>
    void connect(std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = nullptr;

        fn = [](const void *, std::tuple<Args &&...> args) -> Ret {
            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Function, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    void connect(Type &value_or_instance, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = &value_or_instance;

        fn = [](const void *payload, std::tuple<Args &&...> args) -> Ret {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(payload);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(payload));
            }

            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Candidate, *curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type &value_or_instance) ENTT_NOEXCEPT
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
        constexpr auto extent = internal::function_extent_v<decltype(internal::to_function_pointer(std::declval<decltype(Function)>()))>;
        connect<Function>(std::make_index_sequence<extent>{});
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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) ENTT_NOEXCEPT {
        constexpr auto extent = internal::function_extent_v<decltype(internal::to_function_pointer(std::declval<decltype(Candidate)>(), value_or_instance))>;
        connect<Candidate>(value_or_instance, std::make_index_sequence<extent>{});
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
        return fn(data, std::forward_as_tuple(std::forward<Args>(args)...));
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
 * @param value_or_instance A valid reference that fits the purpose.
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &value_or_instance) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Candidate, value_or_instance))>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP

// #include "../signal/sigh.hpp"
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
template<typename>
class sigh;


}


#endif // ENTT_SIGNAL_FWD_HPP



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
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
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
        for(auto pos = calls.size(); pos; --pos) {
            calls[pos-1](args...);
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
        bool stop = false;

        for(auto pos = calls.size(); pos && !stop; --pos) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    calls[pos-1](args...);
                    stop = func();
                } else {
                    calls[pos-1](args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    stop = func(calls[pos-1](args...));
                } else {
                    func(calls[pos-1](args...));
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
    /*! Default constructor. */
    connection() = default;

    /*! @brief Default copy constructor. */
    connection(const connection &) = default;

    /**
     * @brief Default move constructor.
     * @param other The instance to move from.
     */
    connection(connection &&other)
        : connection{}
    {
        std::swap(disconnect, other.disconnect);
        std::swap(signal, other.signal);
    }

    /*! @brief Default copy assignment operator. @return This connection. */
    connection & operator=(const connection &) = default;

    /**
     * @brief Default move assignment operator.
     * @param other The instance to move from.
     * @return This connection.
     */
    connection & operator=(connection &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            disconnect = tmp.disconnect;
            signal = tmp.signal;
        }

        return *this;
    }

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
struct scoped_connection: private connection {
    /*! Default constructor. */
    scoped_connection() = default;

    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param conn A valid connection object.
     */
    scoped_connection(const connection &conn)
        : connection{conn}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /*! @brief Default move constructor. */
    scoped_connection(scoped_connection &&) = default;

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        connection::release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const scoped_connection &) = delete;

    /**
     * @brief Default move assignment operator.
     * @return This scoped connection.
     */
    scoped_connection & operator=(scoped_connection &&) = default;

    /**
     * @brief Copies a connection.
     * @param other The connection object to copy.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const connection &other) {
        static_cast<connection &>(*this) = other;
        return *this;
    }

    /**
     * @brief Moves a connection.
     * @param other The connection object to move.
     * @return This scoped connection.
     */
    scoped_connection & operator=(connection &&other) {
        static_cast<connection &>(*this) = std::move(other);
        return *this;
    }

    using connection::operator bool;
    using connection::release;
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

    template<auto Candidate, typename Type>
    static void release(Type &value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Function>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Function>();
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : signal{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return signal->calls.empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     * @return A properly initialized connection object.
     */
    template<auto Function>
    connection connect() {
        disconnect<Function>();
        delegate<void(void *)> conn{};
        conn.template connect<&release<Function>>();
        signal->calls.emplace_back(delegate<Ret(Args...)>{connect_arg<Function>});
        return { std::move(conn), signal };
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
     * @tparam Candidate Member or free function to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type &value_or_instance) {
        disconnect<Candidate>(value_or_instance);
        delegate<void(void *)> conn{};
        conn.template connect<&sink::release<Candidate, Type>>(value_or_instance);
        signal->calls.emplace_back(delegate<Ret(Args...)>{connect_arg<Candidate>, value_or_instance});
        return { std::move(conn), signal };
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls.erase(std::remove(calls.begin(), calls.end(), delegate), calls.end());
    }

    /**
     * @brief Disconnects a member function or a free function with payload from
     * a signal.
     * @tparam Candidate Member or free function to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type &value_or_instance) {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls.erase(std::remove(calls.begin(), calls.end(), delegate), calls.end());
    }

    /**
     * @brief Disconnects member functions or free functions based on an
     * instance or specific payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<typename Type>
    void disconnect(const Type &value_or_instance) {
        auto &calls = signal->calls;
        calls.erase(std::remove_if(calls.begin(), calls.end(), [&value_or_instance](const auto &delegate) {
            return delegate.instance() == &value_or_instance;
        }), calls.end());
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
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


#endif // ENTT_SIGNAL_SIGH_HPP

// #include "runtime_view.hpp"
#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP


#include <iterator>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>
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
#include <numeric>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/algorithm.hpp"

// #include "entity.hpp"
#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include <cstdint>
#include <type_traits>
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
        return (to_integer(entity) & traits_type<Entity>::entity_mask) == to_integer(static_cast<Entity>(*this));
    }

    template<typename Entity>
    constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
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
 * @brief Compile-time constant for null entities.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
constexpr auto null = internal::null{};


}


#endif // ENTT_ENTITY_ENTITY_HPP

// #include "fwd.hpp"
#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP


#include <cstdint>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"



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

/*! @class basic_observer */
template<typename>
class basic_observer;

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
ENTT_OPAQUE_TYPE(entity, ENTT_ID_TYPE)

/*! @brief Alias declaration for the most common use case. */
ENTT_OPAQUE_TYPE(component, ENTT_ID_TYPE)

/*! @brief Alias declaration for the most common use case. */
using registry = basic_registry<entity>;

/*! @brief Alias declaration for the most common use case. */
using observer = basic_observer<entity>;

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

        iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
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

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*direct)[pos];
        }

        reference operator*() const ENTT_NOEXCEPT {
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

        if(!reverse[page]) {
            reverse[page] = std::make_unique<entity_type[]>(entt_per_page);
            // null is safe in all cases for our purposes
            std::fill_n(reverse[page].get(), entt_per_page, null);
        }
    }

    auto map(const Entity entt) const ENTT_NOEXCEPT {
        const auto identifier = to_integer(entt) & traits_type::entity_mask;
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
            if(other.reverse[pos]) {
                assure(pos);
                std::copy_n(other.reverse[pos].get(), entt_per_page, reverse[pos].get());
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
     * Random access iterators stay true to the order imposed by a call to
     * `respect`.
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
        return has(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entt) const ENTT_NOEXCEPT {
        auto [page, offset] = map(entt);
        // testing against null permits to avoid accessing the direct vector
        return (page < reverse.size() && reverse[page] && reverse[page][offset] != null);
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
    size_type index(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(has(entt));
        auto [page, offset] = map(entt);
        return size_type(reverse[page][offset]);
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
        auto [page, offset] = map(entt);
        assure(page);
        reverse[page][offset] = entity_type(direct.size());
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
        std::for_each(std::make_reverse_iterator(last), std::make_reverse_iterator(first), [this, next = direct.size()](const auto entt) mutable {
            ENTT_ASSERT(!has(entt));
            auto [page, offset] = map(entt);
            assure(page);
            reverse[page][offset] = entity_type(next++);
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
    void destroy(const entity_type entt) {
        ENTT_ASSERT(has(entt));
        auto [from_page, from_offset] = map(entt);
        auto [to_page, to_offset] = map(direct.back());
        direct[size_type(reverse[from_page][from_offset])] = entity_type(direct.back());
        reverse[to_page][to_offset] = reverse[from_page][from_offset];
        reverse[from_page][from_offset] = null;
        direct.pop_back();
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
     * @param lhs A valid position within the sparse set.
     * @param rhs A valid position within the sparse set.
     */
    virtual void swap(const size_type lhs, const size_type rhs) ENTT_NOEXCEPT {
        ENTT_ASSERT(lhs < direct.size());
        ENTT_ASSERT(rhs < direct.size());
        auto [src_page, src_offset] = map(direct[lhs]);
        auto [dst_page, dst_offset] = map(direct[rhs]);
        std::swap(reverse[src_page][src_offset], reverse[dst_page][dst_offset]);
        std::swap(direct[lhs], direct[rhs]);
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
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator_type first, iterator_type last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(first > last));

        std::vector<size_type> copy(last - first);
        const auto offset = std::distance(last, end());
        std::iota(copy.begin(), copy.end(), size_type{});

        algo(copy.rbegin(), copy.rend(), [this, offset, compare = std::move(compare)](const auto lhs, const auto rhs) {
            return compare(std::as_const(direct[lhs+offset]), std::as_const(direct[rhs+offset]));
        }, std::forward<Args>(args)...);

        for(size_type pos{}, length = copy.size(); pos < length; ++pos) {
            auto curr = pos;
            auto next = copy[curr];

            while(curr != next) {
                swap(copy[curr] + offset, copy[next] + offset);
                copy[curr] = curr;
                curr = next;
                next = copy[curr];
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
     * Attempting to iterate elements using the raw pointer returned by `data`
     * gives no guarantees on the order, even though `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set &other) ENTT_NOEXCEPT {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = direct.size() - 1;

        while(pos && from != to) {
            if(has(*from)) {
                if(*from != direct[pos]) {
                    swap(pos, index(*from));
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Resets a sparse set.
     */
    void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<std::unique_ptr<entity_type[]>> reverse;
    std::vector<entity_type> direct;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP

// #include "entity.hpp"

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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    class iterator {
        friend class basic_runtime_view<Entity>;

        iterator(underlying_iterator_type first, underlying_iterator_type last, const sparse_set<Entity> * const *others, const sparse_set<Entity> * const *length) ENTT_NOEXCEPT
            : begin{first},
              end{last},
              from{others},
              to{length}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        bool valid() const ENTT_NOEXCEPT {
            return std::all_of(from, to, [entt = *begin](const auto *view) {
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

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        underlying_iterator_type begin;
        underlying_iterator_type end;
        const sparse_set<Entity> * const *from;
        const sparse_set<Entity> * const *to;
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

    bool valid() const ENTT_NOEXCEPT {
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
            it = { pool.begin(), pool.end(), data + 1, data + pools.size() };
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
            it = { pool.end(), pool.end(), nullptr, nullptr };
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
        if constexpr(sizeof...(Component) == 1) {
            const auto sz = reg->template size<Component...>();
            const auto *entities = reg->template data<Component...>();

            archive(typename traits_type::entity_type(sz));

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
                reg->template assign<Type>(args..., entt);
            } else {
                Type instance{};
                archive(entt, instance);
                force(*reg, entt, discard);
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
                reg->template reset<Component>(local);
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
    std::unordered_map<entity_type, std::pair<entity_type, bool>> remloc;
    basic_registry<entity_type> *reg;
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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

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

        iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
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

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
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
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator_type{&instances, pos};
    }

    /*! @copydoc cbegin */
    const_iterator_type begin() const ENTT_NOEXCEPT {
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
     * Random access iterators stay true to the order imposed by a call to
     * either `sort` or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return const_iterator_type{&instances, {}};
    }

    /*! @copydoc cend */
    const_iterator_type end() const ENTT_NOEXCEPT {
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
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    object_type & get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<object_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entt A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entt) const ENTT_NOEXCEPT {
        return underlying_type::has(entt) ? (instances.data() + underlying_type::index(entt)) : nullptr;
    }

    /*! @copydoc try_get */
    object_type * try_get(const entity_type entt) ENTT_NOEXCEPT {
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
     * @brief Assigns one or more entities to a storage and default constructs
     * their objects.
     *
     * The object type must be at least move and default insertable.
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
     * @return An iterator to the list of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    iterator_type batch(It first, It last) {
        instances.resize(instances.size() + std::distance(first, last));
        // entity goes after component in case constructor throws
        underlying_type::batch(first, last);
        return begin();
    }

    /**
     * @brief Assigns one or more entities to a storage and copy constructs
     * their objects.
     *
     * The object type must be at least move and copy insertable.
     *
     * @sa batch
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value The value to initialize the new objects with.
     * @return An iterator to the list of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    iterator_type batch(It first, It last, const object_type &value) {
        instances.resize(instances.size() + std::distance(first, last), value);
        // entity goes after component in case constructor throws
        underlying_type::batch(first, last);
        return begin();
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
    void destroy(const entity_type entt) {
        auto other = std::move(instances.back());
        instances[underlying_type::index(entt)] = std::move(other);
        instances.pop_back();
        underlying_type::destroy(entt);
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
     * @param lhs A valid position within the sparse set.
     * @param rhs A valid position within the sparse set.
     */
    void swap(const size_type lhs, const size_type rhs) ENTT_NOEXCEPT override {
        ENTT_ASSERT(lhs < instances.size());
        ENTT_ASSERT(rhs < instances.size());
        std::swap(instances[lhs], instances[rhs]);
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
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator_type first, iterator_type last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(first > last));

        const auto from = underlying_type::begin() + std::distance(begin(), first);
        const auto to = from + std::distance(first, last);

        if constexpr(std::is_invocable_v<Compare, const object_type &, const object_type &>) {
            static_assert(!std::is_empty_v<object_type>);

            underlying_type::sort(from, to, [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[underlying_type::index(lhs)]), std::as_const(instances[underlying_type::index(rhs)]));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            underlying_type::sort(from, to, std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
    }

    /*! @brief Resets a storage. */
    void reset() {
        underlying_type::reset();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


/*! @copydoc basic_storage */
template<typename Entity, typename Type>
class basic_storage<Entity, Type, std::enable_if_t<std::is_empty_v<Type>>>: public sparse_set<Entity> {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;
    using underlying_type = sparse_set<Entity>;

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

        iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
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

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return nullptr;
        }

        reference operator*() const ENTT_NOEXCEPT {
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
    iterator_type begin() const ENTT_NOEXCEPT {
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
    iterator_type end() const ENTT_NOEXCEPT {
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

    /**
     * @brief Assigns one or more entities to a storage.
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
     * @return An iterator to the list of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    iterator_type batch(It first, It last, const object_type & = {}) {
        underlying_type::batch(first, last);
        return begin();
    }
};

/*! @copydoc basic_storage */
template<typename Entity, typename Type>
struct storage: basic_storage<Entity, Type> {};


}


#endif // ENTT_ENTITY_STORAGE_HPP

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
constexpr exclude_t<Type...> exclude{};


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
constexpr get_t<Type...> get{};


}


#endif // ENTT_ENTITY_UTILITY_HPP

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

// #include "utility.hpp"

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
 * * The entity currently pointed is destroyed.
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
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Type of component observed by the group.
 * @tparam Other Other types of components observed by the group.
 */
template<typename Entity, typename... Exclude, typename Get, typename... Other>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get, Other...>> {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    // we could use pool_type<Type> *..., but vs complains about it and refuses to compile for unknown reasons (most likely a bug)
    basic_group(sparse_set<Entity> *ref, storage<Entity, std::remove_const_t<Get>> *get, storage<Entity, std::remove_const_t<Other>> *... other) ENTT_NOEXCEPT
        : handler{ref},
          pools{get, other...}
    {}

    template<typename Func, typename... Weak>
    void traverse(Func func, type_list<Weak...>) const {
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
     * void(const entity_type, Get &, Other &...);
     * void(Get &, Other &...);
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
    void each(Func func) const {
        traverse(std::move(func), type_list<Get, Other...>{});
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
    void less(Func func) const {
        using get_type_list = std::conditional_t<std::is_empty_v<Get>, type_list<>, type_list<Get>>;
        using other_type_list = type_list_cat_t<std::conditional_t<std::is_empty_v<Other>, type_list<>, type_list<Other>>...>;
        traverse(std::move(func), type_list_cat_t<get_type_list, other_type_list>{});
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
        if constexpr(sizeof...(Component) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>);
            handler->sort(handler->begin(), handler->end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            handler->sort(handler->begin(), handler->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                // useless this-> used to suppress a warning with clang
                return compare(this->get<Component...>(lhs), this->get<Component...>(rhs));
            }, std::move(algo), std::forward<Args>(args)...);
        }
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
    const std::tuple<pool_type<Get> *, pool_type<Other> *...> pools;
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
 * * The entity currently pointed is destroyed.
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
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Types of components observed by the group.
 * @tparam Owned Type of component owned by the group.
 * @tparam Other Other types of components owned by the group.
 */
template<typename Entity, typename... Exclude, typename... Get, typename Owned, typename... Other>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned, Other...> {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    template<typename Component>
    using component_iterator_type = decltype(std::declval<pool_type<Component>>().begin());

    // we could use pool_type<Type> *..., but vs complains about it and refuses to compile for unknown reasons (most likely a bug)
    basic_group(const typename basic_registry<Entity>::size_type *sz, storage<Entity, std::remove_const_t<Owned>> *owned, storage<Entity, std::remove_const_t<Other>> *... other, storage<Entity, std::remove_const_t<Get>> *... get) ENTT_NOEXCEPT
        : length{sz},
          pools{owned, other..., get...}
    {}

    template<typename Func, typename... Strong, typename... Weak>
    void traverse(Func func, type_list<Strong...>, type_list<Weak...>) const {
        [[maybe_unused]] auto raw = std::make_tuple((std::get<pool_type<Strong> *>(pools)->end() - *length)...);
        [[maybe_unused]] auto data = std::get<pool_type<Owned> *>(pools)->sparse_set<entity_type>::end() - *length;

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
        return std::get<pool_type<Owned> *>(pools)->data();
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
        return std::get<pool_type<Owned> *>(pools)->sparse_set<entity_type>::end() - *length;
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
        return std::get<pool_type<Owned> *>(pools)->sparse_set<entity_type>::end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = std::get<pool_type<Owned> *>(pools)->find(entt);
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
     * void(const entity_type, Owned &, Other &..., Get &...);
     * void(Owned &, Other &..., Get &...);
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
    void each(Func func) const {
        traverse(std::move(func), type_list<Owned, Other...>{}, type_list<Get...>{});
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
    void less(Func func) const {
        using owned_type_list = std::conditional_t<std::is_empty_v<Owned>, type_list<>, type_list<Owned>>;
        using other_type_list = type_list_cat_t<std::conditional_t<std::is_empty_v<Other>, type_list<>, type_list<Other>>...>;
        using get_type_list = type_list_cat_t<std::conditional_t<std::is_empty_v<Get>, type_list<>, type_list<Get>>...>;
        traverse(std::move(func), type_list_cat_t<owned_type_list, other_type_list>{}, get_type_list{});
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
        auto *cpool = std::get<pool_type<Owned> *>(pools);

        if constexpr(sizeof...(Component) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>);
            cpool->sort(cpool->end()-*length, cpool->end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool->sort(cpool->end()-*length, cpool->end(), [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                // useless this-> used to suppress a warning with clang
                return compare(this->get<Component...>(lhs), this->get<Component...>(rhs));
            }, std::move(algo), std::forward<Args>(args)...);
        }

        for(auto next = *length; next; --next) {
            ([next = next-1, curr = cpool->data()[next-1]](auto *cpool) {
                const auto pos = cpool->index(curr);

                if(pos != next) {
                    cpool->swap(next, cpool->index(curr));
                }
            }(std::get<pool_type<Other> *>(pools)), ...);
        }
    }

private:
    const typename basic_registry<Entity>::size_type *length;
    const std::tuple<pool_type<Owned> *, pool_type<Other> *..., pool_type<Get> *...> pools;
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
 * * The entity currently pointed is destroyed.
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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    class iterator {
        friend class basic_view<Entity, Component...>;

        iterator(unchecked_type other, underlying_iterator_type first, underlying_iterator_type last) ENTT_NOEXCEPT
            : unchecked{other},
              begin{first},
              end{last}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        bool valid() const ENTT_NOEXCEPT {
            return std::all_of(unchecked.cbegin(), unchecked.cend(), [this](const sparse_set<Entity> *view) {
                return view->has(*begin);
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

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        unchecked_type unchecked;
        underlying_iterator_type begin;
        underlying_iterator_type end;
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
    decltype(auto) get([[maybe_unused]] component_iterator_type<Comp> it, [[maybe_unused]] pool_type<Other> *cpool, [[maybe_unused]] const Entity entt) const ENTT_NOEXCEPT {
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
            std::for_each(begin, end, [this, raw = std::get<pool_type<Comp> *>(pools)->begin(), &func](const auto entity) mutable {
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
            std::for_each(begin, end, [this, &func](const auto entity) mutable {
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
    void each(Func func) const {
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
    void each(Func func) const {
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
    void less(Func func) const {
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
    void less(Func func) const {
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
 * * The entity currently pointed is destroyed.
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
    void each(Func func) const {
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
    void less(Func func) const {
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
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    struct group_type {
        std::size_t owned{};
    };

    template<typename Component>
    struct pool_handler: storage<Entity, Component> {
        group_type *group{};

        pool_handler() ENTT_NOEXCEPT = default;

        pool_handler(const storage<Entity, Component> &other)
            : storage<Entity, Component>{other}
        {}

        auto on_construct() ENTT_NOEXCEPT {
            return sink{construction};
        }

        auto on_replace() ENTT_NOEXCEPT {
            return sink{update};
        }

        auto on_destroy() ENTT_NOEXCEPT {
            return sink{destruction};
        }

        template<typename... Args>
        decltype(auto) assign(basic_registry &registry, const Entity entt, Args &&... args) {
            if constexpr(std::is_empty_v<Component>) {
                storage<Entity, Component>::construct(entt);
                construction.publish(entt, registry, Component{});
                return Component{std::forward<Args>(args)...};
            } else {
                auto &component = storage<Entity, Component>::construct(entt, std::forward<Args>(args)...);
                construction.publish(entt, registry, component);
                return component;
            }
        }

        template<typename It, typename... Comp>
        auto batch(basic_registry &registry, It first, It last, const Comp &... value) {
            auto it = storage<Entity, Component>::batch(first, last, value...);

            if(!construction.empty()) {
                std::for_each(first, last, [this, &registry, it](const auto entt) mutable {
                    construction.publish(entt, registry, *(it++));
                });
            }

            return it;
        }

        void remove(basic_registry &registry, const Entity entt) {
            destruction.publish(entt, registry);
            storage<Entity, Component>::destroy(entt);
        }

        template<typename... Args>
        decltype(auto) replace(basic_registry &registry, const Entity entt, Args &&... args) {
            if constexpr(std::is_empty_v<Component>) {
                ENTT_ASSERT((storage<Entity, Component>::has(entt)));
                update.publish(entt, registry, Component{});
                return Component{std::forward<Args>(args)...};
            } else {
                Component component{std::forward<Args>(args)...};
                update.publish(entt, registry, component);
                return (storage<Entity, Component>::get(entt) = std::move(component));
            }
        }

    private:
        using reference_type = std::conditional_t<std::is_empty_v<Component>, const Component &, Component &>;
        sigh<void(const Entity, basic_registry &, reference_type)> construction{};
        sigh<void(const Entity, basic_registry &, reference_type)> update{};
        sigh<void(const Entity, basic_registry &)> destruction{};
    };

    template<typename Component>
    using pool_type = pool_handler<std::decay_t<Component>>;

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>>: sparse_set<Entity> {
        std::tuple<pool_type<Get> *..., pool_type<Exclude> *...> cpools{};

        template<typename Component>
        void maybe_valid_if(const Entity entt) {
            if constexpr(std::disjunction_v<std::is_same<Get, Component>...>) {
                if(((std::is_same_v<Component, Get> || std::get<pool_type<Get> *>(cpools)->has(entt)) && ...)
                        && !(std::get<pool_type<Exclude> *>(cpools)->has(entt) || ...))
                {
                    this->construct(entt);
                }
            } else if constexpr(std::disjunction_v<std::is_same<Exclude, Component>...>) {
                if((std::get<pool_type<Get> *>(cpools)->has(entt) && ...)
                        && ((std::is_same_v<Exclude, Component> || !std::get<pool_type<Exclude> *>(cpools)->has(entt)) && ...)) {
                    this->construct(entt);
                }
            }
        }

        void discard_if(const Entity entt) {
            if(this->has(entt)) {
                this->destroy(entt);
            }
        }
    };

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...>: group_type {
        std::tuple<pool_type<Owned> *..., pool_type<Get> *..., pool_type<Exclude> *...> cpools{};

        template<typename Component>
        void maybe_valid_if(const Entity entt) {
            if constexpr(std::disjunction_v<std::is_same<Owned, Component>..., std::is_same<Get, Component>...>) {
                if(((std::is_same_v<Component, Owned> || std::get<pool_type<Owned> *>(cpools)->has(entt)) && ...)
                        && ((std::is_same_v<Component, Get> || std::get<pool_type<Get> *>(cpools)->has(entt)) && ...)
                        && !(std::get<pool_type<Exclude> *>(cpools)->has(entt) || ...))
                {
                    const auto pos = this->owned++;
                    (std::get<pool_type<Owned> *>(cpools)->swap(std::get<pool_type<Owned> *>(cpools)->index(entt), pos), ...);
                }
            } else if constexpr(std::disjunction_v<std::is_same<Exclude, Component>...>) {
                if((std::get<pool_type<Owned> *>(cpools)->has(entt) && ...)
                        && (std::get<pool_type<Get> *>(cpools)->has(entt) && ...)
                        && ((std::is_same_v<Exclude, Component> || !std::get<pool_type<Exclude> *>(cpools)->has(entt)) && ...))
                {
                    const auto pos = this->owned++;
                    (std::get<pool_type<Owned> *>(cpools)->swap(std::get<pool_type<Owned> *>(cpools)->index(entt), pos), ...);
                }
            }
        }

        void discard_if(const Entity entt) {
            if(std::get<0>(cpools)->has(entt) && std::get<0>(cpools)->index(entt) < this->owned) {
                const auto pos = --this->owned;
                (std::get<pool_type<Owned> *>(cpools)->swap(std::get<pool_type<Owned> *>(cpools)->index(entt), pos), ...);
            }
        }
    };

    struct pool_data {
        std::unique_ptr<sparse_set<Entity>> pool;
        void(* remove)(sparse_set<Entity> &, basic_registry &, const Entity);
        std::unique_ptr<sparse_set<Entity>>(* clone)(const sparse_set<Entity> &);
        void(* stomp)(const sparse_set<Entity> &, const Entity, basic_registry &, const Entity);
        ENTT_ID_TYPE runtime_type;
    };

    struct group_data {
        const std::size_t extent[3];
        std::unique_ptr<void, void(*)(void *)> group;
        bool(* const is_same)(const component *);
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
        const auto entt = to_integer(entity) & traits_type::entity_mask;
        const auto version = ((to_integer(entity) >> traits_type::entity_shift) + 1) << traits_type::entity_shift;
        const auto node = to_integer(destroyed) | version;
        entities[entt] = Entity{node};
        destroyed = Entity{entt};
    }

    template<typename Component>
    const pool_type<Component> * pool() const ENTT_NOEXCEPT {
        const auto ctype = to_integer(type<Component>());

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
    pool_type<Component> * pool() ENTT_NOEXCEPT {
        return const_cast<pool_type<Component> *>(std::as_const(*this).template pool<Component>());
    }

    template<typename Component>
    pool_type<Component> * assure() {
        const auto ctype = to_integer(type<Component>());
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

            pdata->remove = [](sparse_set<Entity> &cpool, basic_registry &registry, const Entity entt) {
                static_cast<pool_type<Component> &>(cpool).remove(registry, entt);
            };

            if constexpr(std::is_copy_constructible_v<std::decay_t<Component>>) {
                pdata->clone = [](const sparse_set<Entity> &cpool) -> std::unique_ptr<sparse_set<Entity>> {
                    return std::make_unique<pool_type<Component>>(static_cast<const pool_type<Component> &>(cpool));
                };

                pdata->stomp = [](const sparse_set<Entity> &cpool, const Entity from, basic_registry &other, const Entity to) {
                    other.assign_or_replace<Component>(to, static_cast<const pool_type<Component> &>(cpool).get(from));
                };
            } else {
                pdata->clone = nullptr;
                pdata->stomp = nullptr;
            }
        }

        return static_cast<pool_type<Component> *>(pdata->pool.get());
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;

    /*! @brief Default constructor. */
    basic_registry() ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;

    /**
     * @brief Returns the opaque identifier of a component.
     *
     * The given component doesn't need to be necessarily in use.<br/>
     * Do not use this functionality to generate numeric identifiers for types
     * at runtime. They aren't guaranteed to be stable between different runs.
     *
     * @tparam Component Type of component to query.
     * @return Runtime the opaque identifier of the given type of component.
     */
    template<typename Component>
    static component type() ENTT_NOEXCEPT {
        return component{runtime_type<Component, component_family>()};
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
        auto curr = destroyed;
        size_type cnt{};

        while(curr != null) {
            curr = entities[to_integer(curr) & traits_type::entity_mask];
            ++cnt;
        }

        return entities.size() - cnt;
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
        return !alive();
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
    Component * raw() ENTT_NOEXCEPT {
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
        const auto pos = size_type(to_integer(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integer(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(to_integer(entity) >> traits_type::entity_shift);
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
        const auto pos = size_type(to_integer(entity) & traits_type::entity_mask);
        ENTT_ASSERT(pos < entities.size());
        return version_type(to_integer(entities[pos]) >> traits_type::entity_shift);
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
     * been destroyed and potentially recycled.<br/>
     * The returned entity has assigned the given components, if any.
     *
     * The components must be at least default constructible. A compilation
     * error will occur otherwhise.
     *
     * @tparam Component Types of components to assign to the entity.
     * @return A valid entity identifier if the component list is empty, a tuple
     * containing the entity identifier and the references to the components
     * just created otherwise.
     */
    template<typename... Component>
    auto create() {
        entity_type entities[1]{};

        if constexpr(sizeof...(Component) == 0) {
            create<Component...>(std::begin(entities), std::end(entities));
            return entities[0];
        } else {
            auto it = create<Component...>(std::begin(entities), std::end(entities));
            return std::tuple<entity_type, decltype(assign<Component>(entities[0]))...>{entities[0], *std::get<typename pool_type<Component>::iterator_type>(it)...};
        }
    }

    /**
     * @brief Assigns each element in a range an entity.
     *
     * @sa create
     *
     * The components must be at least move and default insertable. A
     * compilation error will occur otherwhise.
     *
     * @tparam Component Types of components to assign to the entity.
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     * @return No return value if the component list is empty, a tuple
     * containing the iterators to the lists of components just created and
     * sorted the same of the entities otherwise.
     */
    template<typename... Component, typename It>
    auto create(It first, It last) {
        static_assert(std::is_convertible_v<entity_type, typename std::iterator_traits<It>::value_type>);

        std::generate(first, last, [this]() {
            entity_type curr;

            if(destroyed == null) {
                curr = entities.emplace_back(entity_type(entities.size()));
                // traits_type::entity_mask is reserved to allow for null identifiers
                ENTT_ASSERT(to_integer(curr) < traits_type::entity_mask);
            } else {
                const auto entt = to_integer(destroyed);
                const auto version = to_integer(entities[entt]) & (traits_type::version_mask << traits_type::entity_shift);
                destroyed = entity_type{to_integer(entities[entt]) & traits_type::entity_mask};
                curr = entity_type{entt | version};
                entities[entt] = curr;
            }

            return curr;
        });

        if constexpr(sizeof...(Component) > 0) {
            return std::make_tuple(assure<Component>()->batch(*this, first, last)...);
        }
    }

    /**
     * @brief Creates a new entity from a prototype entity.
     *
     * @sa create
     *
     * @tparam Component Types of components to copy.
     * @tparam Exclude Types of components not to be copied.
     * @param src A valid entity identifier to be copied.
     * @param other The registry that owns the source entity.
     * @return A valid entity identifier.
     */
    template<typename... Component, typename... Exclude>
    auto create(entity_type src, basic_registry &other, exclude_t<Exclude...> = {}) {
        entity_type entities[1]{};
        create<Component...>(std::begin(entities), std::end(entities), src, other, exclude<Exclude...>);
        return entities[0];
    }

    /**
     * @brief Assigns each element in a range an entity from a prototype entity.
     *
     * @sa create
     *
     * @tparam Component Types of components to copy.
     * @tparam Exclude Types of components not to be copied.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     * @param src A valid entity identifier to be copied.
     * @param other The registry that owns the source entity.
     */
    template<typename... Component, typename It, typename... Exclude>
    void create(It first, It last, entity_type src, basic_registry &other, exclude_t<Exclude...> = {}) {
        create(first, last);

        if constexpr(sizeof...(Component) == 0) {
            stomp<Component...>(first, last, src, other, exclude<Exclude...>);
        } else {
            static_assert(sizeof...(Component) == 0 || sizeof...(Exclude) == 0);
            (assure<Component>()->batch(*this, first, last, other.get<Component>(src)), ...);
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
                pdata.remove(*pdata.pool, *this, entity);
            }
        };

        // just a way to protect users from listeners that attach components
        ENTT_ASSERT(orphan(entity));
        release(entity);
    }

    /**
     * @brief Destroys all the entities in a range.
     *
     * @sa destroy
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void destroy(It first, It last) {
        // useless this-> used to suppress a warning with clang
        std::for_each(first, last, [this](const auto entity) { this->destroy(entity); });
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
    decltype(auto) get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
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
    auto try_get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
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
     * void(Entity, registry<Entity> &, Component &);
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
        return assure<Component>()->on_construct();
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
     * void(Entity, registry<Entity> &, Component &);
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
        return assure<Component>()->on_replace();
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
     * void(Entity, registry<Entity> &);
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
        return assure<Component>()->on_destroy();
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
     * Pools of components owned by a group are only partially sorted.<br/>
     * In other words, only the elements that aren't part of the group are
     * sorted by this function. Use the `sort` member function of a group to
     * sort the other half of the pool.
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
        if(auto *cpool = assure<Component>(); cpool->group) {
            const auto last = cpool->end() - cpool->group->owned;
            cpool->sort(cpool->begin(), last, std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool->sort(cpool->begin(), cpool->end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
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
     * Pools of components owned by a group cannot be sorted this way.<br/>
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
        if(auto *cpool = assure<Component>(); cpool->on_destroy().empty()) {
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

        if(destroyed == null) {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                const auto curr = entity_type(pos - 1);
                const auto entity = entities[to_integer(curr)];
                const auto entt = entity_type{to_integer(entity) & traits_type::entity_mask};

                if(curr == entt) {
                    func(entity);
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

        each([this, &func](const auto entity) {
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
    entt::basic_view<Entity, Component...> view() const {
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0);
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1);

        using handler_type = group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...>;

        const std::size_t extent[] = { sizeof...(Owned), sizeof...(Get), sizeof...(Exclude) };
        const component types[] = { type<Owned>()..., type<Get>()..., type<Exclude>()... };
        handler_type *curr = nullptr;

        if(auto it = std::find_if(groups.begin(), groups.end(), [&extent, &types](auto &&gdata) {
            return std::equal(std::begin(extent), std::end(extent), gdata.extent) && gdata.is_same(types);
        }); it != groups.cend())
        {
            curr = static_cast<handler_type *>(it->group.get());
        }

        if(!curr) {
            groups.push_back(group_data{
                { sizeof...(Owned), sizeof...(Get), sizeof...(Exclude) },
                decltype(group_data::group){new handler_type{}, [](void *gptr) { delete static_cast<handler_type *>(gptr); }},
                [](const component *other) {
                    const component ctypes[] = { type<Owned>()..., type<Get>()..., type<Exclude>()... };
                    return std::equal(std::begin(ctypes), std::end(ctypes), other);
                }
            });

            curr = static_cast<handler_type *>(groups.back().group.get());

            ((std::get<pool_type<Owned> *>(curr->cpools) = assure<Owned>()), ...);
            ((std::get<pool_type<Get> *>(curr->cpools) = assure<Get>()), ...);
            ((std::get<pool_type<Exclude> *>(curr->cpools) = assure<Exclude>()), ...);

            ENTT_ASSERT((!std::get<pool_type<Owned> *>(curr->cpools)->group && ...));

            ((std::get<pool_type<Owned> *>(curr->cpools)->group = curr), ...);
            (std::get<pool_type<Owned> *>(curr->cpools)->on_construct().template connect<&handler_type::template maybe_valid_if<Owned>>(*curr), ...);
            (std::get<pool_type<Owned> *>(curr->cpools)->on_destroy().template connect<&handler_type::discard_if>(*curr), ...);

            (std::get<pool_type<Get> *>(curr->cpools)->on_construct().template connect<&handler_type::template maybe_valid_if<Get>>(*curr), ...);
            (std::get<pool_type<Get> *>(curr->cpools)->on_destroy().template connect<&handler_type::discard_if>(*curr), ...);

            (std::get<pool_type<Exclude> *>(curr->cpools)->on_destroy().template connect<&handler_type::template maybe_valid_if<Exclude>>(*curr), ...);
            (std::get<pool_type<Exclude> *>(curr->cpools)->on_construct().template connect<&handler_type::discard_if>(*curr), ...);

            const auto *cpool = std::min({
                static_cast<sparse_set<Entity> *>(std::get<pool_type<Owned> *>(curr->cpools))...,
                static_cast<sparse_set<Entity> *>(std::get<pool_type<Get> *>(curr->cpools))...
            }, [](const auto *lhs, const auto *rhs) {
                return lhs->size() < rhs->size();
            });

            // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
            std::for_each(cpool->data(), cpool->data() + cpool->size(), [curr](const auto entity) {
                if((std::get<pool_type<Owned> *>(curr->cpools)->has(entity) && ...)
                        && (std::get<pool_type<Get> *>(curr->cpools)->has(entity) && ...)
                        && !(std::get<pool_type<Exclude> *>(curr->cpools)->has(entity) || ...))
                {
                    if constexpr(sizeof...(Owned) == 0) {
                        curr->construct(entity);
                    } else {
                        const auto pos = curr->owned++;
                        // useless this-> used to suppress a warning with clang
                        (std::get<pool_type<Owned> *>(curr->cpools)->swap(std::get<pool_type<Owned> *>(curr->cpools)->index(entity), pos), ...);
                    }
                }
            });
        }

        if constexpr(sizeof...(Owned) == 0) {
            return { static_cast<sparse_set<Entity> *>(curr), std::get<pool_type<Get> *>(curr->cpools)... };
        } else {
            return { &curr->owned, std::get<pool_type<Owned> *>(curr->cpools)... , std::get<pool_type<Get> *>(curr->cpools)... };
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
        static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type, component>);
        std::vector<const sparse_set<Entity> *> set(std::distance(first, last));

        std::transform(first, last, set.begin(), [this](const component ctype) {
            auto it = std::find_if(pools.begin(), pools.end(), [ctype = to_integer(ctype)](const auto &pdata) {
                return pdata.pool && pdata.runtime_type == ctype;
            });

            return it != pools.cend() && it->pool ? it->pool.get() : nullptr;
        });

        return { std::move(set) };
    }

    /**
     * @brief Returns a full or partial copy of a registry.
     *
     * The components must be copyable for obvious reasons. The entities
     * maintain their versions once copied.<br/>
     * If no components are provided, the registry will try to clone all the
     * existing pools. The ones for non-copyable types won't be cloned.
     *
     * This feature supports exclusion lists. The excluded types have higher
     * priority than those indicated for cloning. An excluded type will never be
     * cloned.
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
     * @tparam Exclude Types of components not to be cloned.
     * @return A fresh copy of the registry.
     */
    template<typename... Component, typename... Exclude>
    basic_registry clone(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_copy_constructible<Component>...>);
        basic_registry other;

        other.pools.resize(pools.size());

        for(auto pos = pools.size(); pos; --pos) {
            const auto &pdata = pools[pos-1];
            ENTT_ASSERT(!sizeof...(Component) || !pdata.pool || pdata.clone);

            if(pdata.pool && pdata.clone
                    && (!sizeof...(Component) || ... || (pdata.runtime_type == to_integer(type<Component>())))
                    && ((pdata.runtime_type != to_integer(type<Exclude>())) && ...))
            {
                auto &curr = other.pools[pos-1];
                curr.remove = pdata.remove;
                curr.clone = pdata.clone;
                curr.stomp = pdata.stomp;
                curr.pool = pdata.clone ? pdata.clone(*pdata.pool) : nullptr;
                curr.runtime_type = pdata.runtime_type;
            }
        }

        other.skip_family_pools = skip_family_pools;
        other.destroyed = destroyed;
        other.entities = entities;

        other.pools.erase(std::remove_if(other.pools.begin()+skip_family_pools, other.pools.end(), [](const auto &pdata) {
            return !pdata.pool;
        }), other.pools.end());

        return other;
    }

    /**
     * @brief Stomps an entity and its components.
     *
     * The components must be copyable for obvious reasons. The entities
     * must be both valid.<br/>
     * If no components are provided, the registry will try to copy all the
     * existing types. The non-copyable ones will be ignored.
     *
     * This feature supports exclusion lists as an alternative to component
     * lists. An excluded type will never be copied.
     *
     * @warning
     * Attempting to copy components that aren't copyable results in unexpected
     * behaviors.<br/>
     * A static assertion will abort the compilation when the components
     * provided aren't copy constructible. Otherwise, an assertion will abort
     * the execution at runtime in debug mode in case one or more types cannot
     * be copied.
     *
     * @warning
     * Attempting to use invalid entities results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entities.
     *
     * @tparam Component Types of components to copy.
     * @tparam Exclude Types of components not to be copied.
     * @param dst A valid entity identifier to copy to.
     * @param src A valid entity identifier to be copied.
     * @param other The registry that owns the source entity.
     */
    template<typename... Component, typename... Exclude>
    void stomp(const entity_type dst, const entity_type src, basic_registry &other, exclude_t<Exclude...> = {}) {
        const entity_type entities[1]{dst};
        stomp<Component...>(std::begin(entities), std::end(entities), src, other, exclude<Exclude...>);
    }

    /**
     * @brief Stomps the entities in a range and their components.
     *
     * @sa stomp
     *
     * @tparam Component Types of components to copy.
     * @tparam Exclude Types of components not to be copied.
     * @param first An iterator to the first element of the range to stomp.
     * @param last An iterator past the last element of the range to stomp.
     * @param src A valid entity identifier to be copied.
     * @param other The registry that owns the source entity.
     */
    template<typename... Component, typename It, typename... Exclude>
    void stomp(It first, It last, const entity_type src, basic_registry &other, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) == 0 || sizeof...(Exclude) == 0);
        static_assert(std::conjunction_v<std::is_copy_constructible<Component>...>);

        for(auto pos = other.pools.size(); pos; --pos) {
            const auto &pdata = other.pools[pos-1];
            ENTT_ASSERT(!sizeof...(Component) || !pdata.pool || pdata.stomp);

            if(pdata.pool && pdata.stomp
                    && (!sizeof...(Component) || ... || (pdata.runtime_type == to_integer(type<Component>())))
                    && ((pdata.runtime_type != to_integer(type<Exclude>())) && ...)
                    && pdata.pool->has(src))
            {
                std::for_each(first, last, [this, &pdata, src](const auto entity) {
                    pdata.stomp(*pdata.pool, src, *this, entity);
                });
            }
        }
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

        const auto head = to_integer(destroyed);
        const entity_type seed = (destroyed == null) ? destroyed : entity_type{head | (to_integer(entities[head]) & (traits_type::version_mask << traits_type::entity_shift))};

        follow_fn_type *follow = [](const basic_registry &reg, const entity_type entity) -> entity_type {
            const auto &others = reg.entities;
            const auto entt = to_integer(entity) & traits_type::entity_mask;
            const auto curr = to_integer(others[entt]) & traits_type::entity_mask;
            return entity_type{curr | (to_integer(others[curr]) & (traits_type::version_mask << traits_type::entity_shift))};
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

        force_fn_type *force = [](basic_registry &reg, const entity_type entity, const bool discard) {
            const auto entt = to_integer(entity) & traits_type::entity_mask;
            auto &others = reg.entities;

            if(!(entt < others.size())) {
                auto curr = others.size();
                others.resize(entt + 1);

                std::generate(others.data() + curr, others.data() + entt, [curr]() mutable {
                    return entity_type(curr++);
                });
            }

            others[entt] = entity;

            if(discard) {
                reg.destroy(entity);
                const auto version = to_integer(entity) & (traits_type::version_mask << traits_type::entity_shift);
                others[entt] = entity_type{(to_integer(others[entt]) & traits_type::entity_mask) | version};
            }
        };

        reset();
        entities.clear();
        destroyed = null;

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
                decltype(ctx_variable::value){new Type{std::forward<Args>(args)...}, [](void *ptr) { delete static_cast<Type *>(ptr); }},
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
    Type * try_ctx() ENTT_NOEXCEPT {
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
    Type & ctx() ENTT_NOEXCEPT {
        return const_cast<Type &>(std::as_const(*this).template ctx<Type>());
    }

private:
    std::size_t skip_family_pools{};
    std::vector<pool_data> pools{};
    std::vector<group_data> groups{};
    std::vector<ctx_variable> vars{};
    std::vector<entity_type> entities{};
    entity_type destroyed{null};
};


}


#endif // ENTT_ENTITY_REGISTRY_HPP

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
    explicit basic_actor(entity_type entity, registry_type &ref)
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
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_actor(basic_actor &&other)
        : entt{other.entt}, reg{other.reg}
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
     * @brief Checks if an actor has the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the actor has all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const ENTT_NOEXCEPT {
        return (reg->template has<Component>(entt) && ...);
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
    explicit operator bool() const ENTT_NOEXCEPT {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
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
    operator entt::basic_view<Entity, Component...>() const {
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
template<ENTT_ID_TYPE Value>
using tag = std::integral_constant<ENTT_ID_TYPE, Value>;


}


#endif // ENTT_ENTITY_HELPER_HPP

// #include "entity/observer.hpp"
#ifndef ENTT_ENTITY_OBSERVER_HPP
#define ENTT_ENTITY_OBSERVER_HPP


#include <limits>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <type_traits>
// #include "../config/config.h"

// #include "../core/type_traits.hpp"

// #include "registry.hpp"

// #include "storage.hpp"

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
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, type_list<NoneOf...>, type_list<AllOf...>>>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, AnyOf>>{};
    }
};

/**
 * @brief Collector.
 * @copydetails basic_collector<>
 * @tparam AnyOf Types of components for which changes should be detected.
 * @tparam Matcher Types of grouping matchers.
 */
template<typename... Reject, typename... Require, typename... Rule, typename... Other>
struct basic_collector<matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>, Other...> {
    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of components tracked by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        using first = matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>;
        return basic_collector<first, Other..., matcher<matcher<type_list<>, type_list<>>, type_list<NoneOf...>, type_list<AllOf...>>>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        using first = matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>;
        return basic_collector<first, Other..., matcher<matcher<type_list<>, type_list<>>, AnyOf>>{};
    }

    /**
     * @brief Updates the filter of the last added matcher.
     * @tparam AllOf Types of components required by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto where(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        return basic_collector<matcher<matcher<type_list<Reject..., NoneOf...>, type_list<Require..., AllOf...>>, Rule...>, Other...>{};
    }
};


/*! @brief Variable template used to ease the definition of collectors. */
constexpr basic_collector<> collector{};


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
 * Lifetime of an observer doesn't necessarily have to overcome the one of the
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
    struct matcher_handler<matcher<matcher<type_list<Reject...>, type_list<Require...>>, AnyOf>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const Entity entt, const basic_registry<Entity> &reg) {
            if(reg.template has<Require...>(entt) && !(reg.template has<Reject>(entt) || ...)) {
                auto *comp = obs.view.try_get(entt);
                (comp ? *comp : obs.view.construct(entt)) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.destroy(entt);
            }
        }

        template<std::size_t Index>
        static void connect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<Reject>().template connect<&discard_if<Index>>(obs), ...);
            reg.template on_replace<AnyOf>().template connect<&maybe_valid_if<Index>>(obs);
            reg.template on_destroy<AnyOf>().template connect<&discard_if<Index>>(obs);
        }

        static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().disconnect(obs), ...);
            (reg.template on_construct<Reject>().disconnect(obs), ...);
            reg.template on_replace<AnyOf>().disconnect(obs);
            reg.template on_destroy<AnyOf>().disconnect(obs);
        }
    };

    template<typename... Reject, typename... Require, typename... NoneOf, typename... AllOf>
    struct matcher_handler<matcher<matcher<type_list<Reject...>, type_list<Require...>>, type_list<NoneOf...>, type_list<AllOf...>>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const Entity entt, const basic_registry<Entity> &reg) {
            if(reg.template has<AllOf...>(entt) && !(reg.template has<NoneOf>(entt) || ...)
                    && reg.template has<Require...>(entt) && !(reg.template has<Reject>(entt) || ...))
            {
                auto *comp = obs.view.try_get(entt);
                (comp ? *comp : obs.view.construct(entt)) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.destroy(entt);
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
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /*! @brief Default constructor. */
    basic_observer() ENTT_NOEXCEPT
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
    basic_observer(basic_registry<entity_type> &reg, basic_collector<Matcher...>) ENTT_NOEXCEPT
        : target{&reg},
          release{},
          view{}
    {
        connect<Matcher...>(reg, std::make_index_sequence<sizeof...(Matcher)>{});
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
        connect<Matcher...>(reg, std::make_index_sequence<sizeof...(Matcher)>{});
        target = &reg;
        view.reset();
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
    iterator_type begin() const ENTT_NOEXCEPT {
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
    iterator_type end() const ENTT_NOEXCEPT {
        return view.sparse_set<entity_type>::end();
    }

    /*! @brief Resets the underlying container. */
    void clear() {
        view.reset();
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
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
        std::for_each(begin(), end(), std::move(func));
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
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


#endif // ENTT_ENTITY_OBSERVER_HPP

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
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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


#endif // ENTT_LOCATOR_LOCATOR_HPP

// #include "meta/factory.hpp"
#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <tuple>
#include <array>
#include <cstddef>
#include <utility>
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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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

// #include "policy.hpp"
#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP


namespace entt {


/*! @brief Empty class type used to request the _as alias_ policy. */
struct as_alias_t {};


/*! @brief Disambiguation tag. */
constexpr as_alias_t as_alias;


/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t {};


/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t {};


}


#endif // ENTT_META_POLICY_HPP

// #include "meta.hpp"
#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <array>
#include <memory>
#include <cstring>
#include <cstddef>
#include <utility>
#include <type_traits>
// #include "../config/config.h"

// #include "policy.hpp"



namespace entt {


class meta_any;
struct meta_handle;
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
    meta_prop(* const meta)() ENTT_NOEXCEPT;
};


struct meta_base_node {
    meta_base_node ** const underlying;
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    void *(* const cast)(void *) ENTT_NOEXCEPT;
    meta_base(* const meta)() ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_conv_node ** const underlying;
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    meta_any(* const conv)(const void *);
    meta_conv(* const meta)() ENTT_NOEXCEPT;
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node ** const underlying;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
    meta_ctor(* const meta)() ENTT_NOEXCEPT;
};


struct meta_dtor_node {
    meta_dtor_node ** const underlying;
    meta_type_node * const parent;
    bool(* const invoke)(meta_handle);
    meta_dtor(* const meta)() ENTT_NOEXCEPT;
};


struct meta_data_node {
    meta_data_node ** const underlying;
    ENTT_ID_TYPE identifier;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_handle, meta_any, meta_any);
    meta_any(* const get)(meta_handle, meta_any);
    meta_data(* const meta)() ENTT_NOEXCEPT;
};


struct meta_func_node {
    using size_type = std::size_t;
    meta_func_node ** const underlying;
    ENTT_ID_TYPE identifier;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)() ENTT_NOEXCEPT;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_handle, meta_any *);
    meta_func(* const meta)() ENTT_NOEXCEPT;
};


struct meta_type_node {
    using size_type = std::size_t;
    ENTT_ID_TYPE identifier;
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
    meta_type(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type(* const meta)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_dtor_node *dtor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
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
    friend struct meta_handle;

    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using compare_fn_type = bool(const void *, const void *);
    using copy_fn_type = void *(storage_type &, const void *);
    using destroy_fn_type = void(void *);
    using steal_fn_type = void *(storage_type &, void *, destroy_fn_type *);

    template<typename Type, typename = std::void_t<>>
    struct type_traits {
        template<typename... Args>
        static void * instance(storage_type &storage, Args &&... args) {
            auto instance = std::make_unique<Type>(std::forward<Args>(args)...);
            new (&storage) Type *{instance.get()};
            return instance.release();
        }

        static void destroy(void *instance) {
            auto *node = internal::meta_info<Type>::resolve();
            auto *actual = static_cast<Type *>(instance);
            [[maybe_unused]] const bool destroyed = node->meta().destroy(*actual);
            ENTT_ASSERT(destroyed);
            delete actual;
        }

        static void * copy(storage_type &storage, const void *other) {
            auto instance = std::make_unique<Type>(*static_cast<const Type *>(other));
            new (&storage) Type *{instance.get()};
            return instance.release();
        }

        static void * steal(storage_type &to, void *from, destroy_fn_type *) {
            auto *instance = static_cast<Type *>(from);
            new (&to) Type *{instance};
            return instance;
        }
    };

    template<typename Type>
    struct type_traits<Type, std::enable_if_t<sizeof(Type) <= sizeof(void *) && std::is_nothrow_move_constructible_v<Type>>> {
        template<typename... Args>
        static void * instance(storage_type &storage, Args &&... args) {
            return new (&storage) Type{std::forward<Args>(args)...};
        }

        static void destroy(void *instance) {
            auto *node = internal::meta_info<Type>::resolve();
            auto *actual = static_cast<Type *>(instance);
            [[maybe_unused]] const bool destroyed = node->meta().destroy(*actual);
            ENTT_ASSERT(destroyed);
            actual->~Type();
        }

        static void * copy(storage_type &storage, const void *instance) {
            return new (&storage) Type{*static_cast<const Type *>(instance)};
        }

        static void * steal(storage_type &to, void *from, destroy_fn_type *destroy_fn) {
            void *instance = new (&to) Type{std::move(*static_cast<Type *>(from))};
            destroy_fn(from);
            return instance;
        }
    };

    template<typename Type>
    static auto compare(int, const Type &lhs, const Type &rhs)
    -> decltype(lhs == rhs, bool{}) {
        return lhs == rhs;
    }

    template<typename Type>
    static bool compare(char, const Type &lhs, const Type &rhs) {
        return &lhs == &rhs;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{nullptr},
          node{nullptr},
          destroy_fn{nullptr},
          compare_fn{nullptr},
          copy_fn{nullptr},
          steal_fn{nullptr}
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
            using traits_type = type_traits<std::remove_cv_t<std::remove_reference_t<Type>>>;
            instance = traits_type::instance(storage, std::forward<Args>(args)...);
            destroy_fn = &traits_type::destroy;
            copy_fn = &traits_type::copy;
            steal_fn = &traits_type::steal;

            compare_fn = [](const void *lhs, const void *rhs) {
                return compare(0, *static_cast<const Type *>(lhs), *static_cast<const Type *>(rhs));
            };
        }
    }

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type>
    explicit meta_any(as_alias_t, Type &type)
        : meta_any{}
    {
        node = internal::meta_info<Type>::resolve();
        instance = &type;

        compare_fn = [](const void *lhs, const void *rhs) {
            return compare(0, *static_cast<const Type *>(lhs), *static_cast<const Type *>(rhs));
        };
    }

    /**
     * @brief Constructs a meta any from a given value.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&type)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(type)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other)
        : meta_any{}
    {
        node = other.node;
        instance = other.copy_fn ? other.copy_fn(storage, other.instance) : other.instance;
        destroy_fn = other.destroy_fn;
        compare_fn = other.compare_fn;
        copy_fn = other.copy_fn;
        steal_fn = other.steal_fn;
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
            destroy_fn(instance);
        }
    }

    /**
     * @brief Assignment operator.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     * @return This meta any object.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any & operator=(Type &&type) {
        return (*this = meta_any{std::forward<Type>(type)});
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to assign.
     * @return This meta any object.
     */
    meta_any & operator=(const meta_any &other) {
        return (*this = meta_any{other});
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to assign.
     * @return This meta any object.
     */
    meta_any & operator=(meta_any &&other) ENTT_NOEXCEPT {
        meta_any any{std::move(other)};
        swap(any, *this);
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
    const Type * try_cast() const ENTT_NOEXCEPT {
        return internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc try_cast */
    template<typename Type>
    Type * try_cast() ENTT_NOEXCEPT {
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
    const Type & cast() const ENTT_NOEXCEPT {
        auto *actual = try_cast<Type>();
        ENTT_ASSERT(actual);
        return *actual;
    }

    /*! @copydoc cast */
    template<typename Type>
    Type & cast() ENTT_NOEXCEPT {
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

        if(const auto *type = internal::meta_info<Type>::resolve(); node == type) {
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
    bool convert() {
        bool valid = (node == internal::meta_info<Type>::resolve());

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
    void emplace(Args&& ... args) {
        *this = meta_any{std::in_place_type_t<Type>{}, std::forward<Args>(args)...};
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    bool operator==(const meta_any &other) const ENTT_NOEXCEPT {
        return node == other.node && ((!compare_fn && !other.compare_fn) || compare_fn(instance, other.instance));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) ENTT_NOEXCEPT {
        if(lhs.steal_fn && rhs.steal_fn) {
            storage_type buffer;
            auto *temp = lhs.steal_fn(buffer, lhs.instance, lhs.destroy_fn);
            lhs.instance = rhs.steal_fn(lhs.storage, rhs.instance, rhs.destroy_fn);
            rhs.instance = lhs.steal_fn(rhs.storage, temp, lhs.destroy_fn);
        } else if(lhs.steal_fn) {
            lhs.instance = lhs.steal_fn(rhs.storage, lhs.instance, lhs.destroy_fn);
            std::swap(rhs.instance, lhs.instance);
        } else if(rhs.steal_fn) {
            rhs.instance = rhs.steal_fn(lhs.storage, rhs.instance, rhs.destroy_fn);
            std::swap(rhs.instance, lhs.instance);
        } else {
            std::swap(lhs.instance, rhs.instance);
        }

        std::swap(lhs.node, rhs.node);
        std::swap(lhs.destroy_fn, rhs.destroy_fn);
        std::swap(lhs.compare_fn, rhs.compare_fn);
        std::swap(lhs.copy_fn, rhs.copy_fn);
        std::swap(lhs.steal_fn, rhs.steal_fn);
    }

private:
    storage_type storage;
    void *instance;
    internal::meta_type_node *node;
    destroy_fn_type *destroy_fn;
    compare_fn_type *compare_fn;
    copy_fn_type *copy_fn;
    steal_fn_type *steal_fn;
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
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle() ENTT_NOEXCEPT
        : node{nullptr},
          instance{nullptr}
    {}

    /**
     * @brief Constructs a meta handle from a meta any object.
     * @param any A reference to an object to use to initialize the handle.
     */
    meta_handle(meta_any &any) ENTT_NOEXCEPT
        : node{any.node},
          instance{any.instance}
    {}

    /**
     * @brief Constructs a meta handle from a given instance.
     * @tparam Type Type of object to use to initialize the handle.
     * @param obj A reference to an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_handle>>>
    meta_handle(Type &obj) ENTT_NOEXCEPT
        : node{internal::meta_info<Type>::resolve()},
          instance{&obj}
    {}

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
     * @return A (possibly null) pointer to the underlying object.
     */
    template<typename Type>
    const Type * data() const ENTT_NOEXCEPT {
        return internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc data */
    template<typename Type>
    Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(std::as_const(*this).data<Type>());
    }

    /**
     * @brief Returns false if a handle is empty, true otherwise.
     * @return False if the handle is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
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

    meta_prop(const internal::meta_prop_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_prop() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    meta_any key() const ENTT_NOEXCEPT {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    meta_any value() const ENTT_NOEXCEPT {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_prop &other) const ENTT_NOEXCEPT {
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

    meta_base(const internal::meta_base_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_base() ENTT_NOEXCEPT
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
    void * cast(void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_base &other) const ENTT_NOEXCEPT {
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

    meta_conv(const internal::meta_conv_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_conv() ENTT_NOEXCEPT
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
    meta_any convert(const void *instance) const ENTT_NOEXCEPT {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_conv &other) const ENTT_NOEXCEPT {
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

    meta_ctor(const internal::meta_ctor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /*! @brief Default constructor. */
    meta_ctor() ENTT_NOEXCEPT
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
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
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
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
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
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_ctor &other) const ENTT_NOEXCEPT {
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

    meta_dtor(const internal::meta_dtor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_dtor() ENTT_NOEXCEPT
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
    bool invoke(meta_handle handle) const {
        return node->invoke(handle);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_dtor &other) const ENTT_NOEXCEPT {
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

    meta_data(const internal::meta_data_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_data() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta data.
     * @return The identifier assigned to the meta data.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
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
    bool is_const() const ENTT_NOEXCEPT {
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
    bool is_static() const ENTT_NOEXCEPT {
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
    bool set(meta_handle handle, Type &&value) const {
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
    bool set(meta_handle handle, std::size_t index, Type &&value) const {
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
    meta_any get(meta_handle handle) const ENTT_NOEXCEPT {
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
    meta_any get(meta_handle handle, std::size_t index) const ENTT_NOEXCEPT {
        ENTT_ASSERT(index < node->type()->extent);
        return node->get(handle, index);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
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
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
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
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_data &other) const ENTT_NOEXCEPT {
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

    meta_func(const internal::meta_func_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @brief Default constructor. */
    meta_func() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta function.
     * @return The identifier assigned to the meta function.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
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
     *
     * A static meta function is such that it can be invoked using a null
     * pointer as an instance.
     *
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
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
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
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
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
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_func &other) const ENTT_NOEXCEPT {
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
    /*! @brief A meta node is allowed to create meta objects. */
    template<typename...> friend struct internal::meta_node;

    meta_type(const internal::meta_type_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @brief Default constructor. */
    meta_type() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta type.
     * @return The identifier assigned to the meta type.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
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
     * @brief Indicates whether a given meta type refers to a function type or
     * not.
     * @return True if the underlying type is a function, false otherwise.
     */
    bool is_function() const ENTT_NOEXCEPT {
        return node->is_function;
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
     * @brief Iterates all the meta base of a meta type.
     *
     * Iteratively returns **all** the base classes of the given type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_base>, void>
    base(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::base>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta base associated with a given identifier.
     *
     * Searches recursively among **all** the base classes of the given type.
     *
     * @param identifier Unique identifier.
     * @return The meta base associated with the given identifier, if any.
     */
    meta_base base(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::base>([identifier](auto *candidate) {
            return candidate->type()->identifier == identifier;
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
    void conv(Op op) const ENTT_NOEXCEPT {
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
    meta_conv conv() const ENTT_NOEXCEPT {
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
    void ctor(Op op) const ENTT_NOEXCEPT {
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
    meta_ctor ctor() const ENTT_NOEXCEPT {
        const auto *curr = internal::ctor<Args...>(std::make_index_sequence<sizeof...(Args)>{}, node);
        return curr ? curr->meta() : meta_ctor{};
    }

    /**
     * @brief Returns the meta destructor associated with a given type.
     * @return The meta destructor associated with the given type, if any.
     */
    meta_dtor dtor() const ENTT_NOEXCEPT {
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
    std::enable_if_t<std::is_invocable_v<Op, meta_data>, void>
    data(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::data>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta data associated with a given identifier.
     *
     * Searches recursively among **all** the meta data of the given type. This
     * means that the meta data of the base classes will also be inspected, if
     * any.
     *
     * @param identifier Unique identifier.
     * @return The meta data associated with the given identifier, if any.
     */
    meta_data data(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::data>([identifier](auto *candidate) {
            return candidate->identifier == identifier;
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
    std::enable_if_t<std::is_invocable_v<Op, meta_func>, void>
    func(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::func>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta function associated with a given identifier.
     *
     * Searches recursively among **all** the meta functions of the given type.
     * This means that the meta functions of the base classes will also be
     * inspected, if any.
     *
     * @param identifier Unique identifier.
     * @return The meta function associated with the given identifier, if any.
     */
    meta_func func(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::func>([identifier](auto *candidate) {
            return candidate->identifier == identifier;
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

        internal::find_if<&internal::meta_type_node::ctor>([data = arguments.data(), &any](auto *curr) -> bool {
            if(curr->size == sizeof...(args)) {
                any = curr->invoke(data);
            }

            return static_cast<bool>(any);
        }, node);

        return any;
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the underlying type.
     * Otherwise, invoking the meta destructor results in an undefined
     * behavior.<br/>
     * If no destructor has been set, this function returns true without doing
     * anything.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return True in case of success, false otherwise.
     */
    bool destroy(meta_handle handle) const {
        return (handle.type() == node->meta()) && (!node->dtor || node->dtor->invoke(handle));
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
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
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
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
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
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
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


template<typename Type>
inline meta_type_node * meta_node<Type>::resolve() ENTT_NOEXCEPT {
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
            []() ENTT_NOEXCEPT -> meta_type {
                return internal::meta_info<std::remove_pointer_t<Type>>::resolve();
            },
            []() ENTT_NOEXCEPT -> meta_type {
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

    static constexpr auto size = sizeof...(Args);
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
bool setter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(!Const) {
        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using helper_type = meta_function_helper_t<decltype(Data)>;
            using data_type = std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>;
            static_assert(std::is_invocable_v<decltype(Data), Type &, data_type>);
            auto *direct = value.try_cast<data_type>();
            auto *clazz = handle.data<Type>();

            if(clazz && (direct || value.convert<data_type>())) {
                std::invoke(Data, *clazz, direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
            static_assert(std::is_invocable_v<decltype(Data), Type *>);
            auto *clazz = handle.data<Type>();

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                auto *direct = value.try_cast<underlying_type>();
                auto *idx = index.try_cast<std::size_t>();

                if(clazz && idx && (direct || value.convert<underlying_type>())) {
                    std::invoke(Data, clazz)[*idx] = direct ? *direct : value.cast<underlying_type>();
                    accepted = true;
                }
            } else {
                auto *direct = value.try_cast<data_type>();

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
                auto *direct = value.try_cast<underlying_type>();
                auto *idx = index.try_cast<std::size_t>();

                if(idx && (direct || value.convert<underlying_type>())) {
                    (*Data)[*idx] = (direct ? *direct : value.cast<underlying_type>());
                    accepted = true;
                }
            } else {
                auto *direct = value.try_cast<data_type>();

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
meta_any getter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index) {
    auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{as_alias, std::forward<decltype(value)>(value)};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
        static_assert(std::is_invocable_v<decltype(Data), Type &>);
        auto *clazz = handle.data<Type>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
        static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto *clazz = handle.data<Type>();

        if constexpr(std::is_array_v<data_type>) {
            auto *idx = index.try_cast<std::size_t>();
            return (clazz && idx) ? dispatch(std::invoke(Data, clazz)[*idx]) : meta_any{};
        } else {
            return clazz ? dispatch(std::invoke(Data, clazz)) : meta_any{};
        }
    } else {
        static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);

        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            auto *idx = index.try_cast<std::size_t>();
            return idx ? dispatch((*Data)[*idx]) : meta_any{};
        } else {
            return dispatch(*Data);
        }
    }
}


template<typename Type, auto Candidate, typename Policy, std::size_t... Indexes>
meta_any invoke([[maybe_unused]] meta_handle handle, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper_t<decltype(Candidate)>;

    auto dispatch = [](auto *... args) {
        if constexpr(std::is_void_v<typename helper_type::return_type> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, *args...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{as_alias, std::invoke(Candidate, *args...)};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::invoke(Candidate, *args...)};
        }
    };

    [[maybe_unused]] const auto direct = std::make_tuple([](meta_any *any, auto *instance) {
        using arg_type = std::remove_reference_t<decltype(*instance)>;

        if(!instance && any->convert<arg_type>()) {
            instance = any->try_cast<arg_type>();
        }

        return instance;
    }(args+Indexes, (args+Indexes)->try_cast<std::tuple_element_t<Indexes, typename helper_type::args_type>>())...);

    if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Candidate)>>) {
        return (std::get<Indexes>(direct) && ...) ? dispatch(std::get<Indexes>(direct)...) : meta_any{};
    } else {
        auto *clazz = handle.data<Type>();
        return (clazz && (std::get<Indexes>(direct) && ...)) ? dispatch(clazz, std::get<Indexes>(direct)...) : meta_any{};
    }
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


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
    static_assert(std::is_same_v<Type, std::decay_t<Type>>);

    template<typename Node>
    bool duplicate(const ENTT_ID_TYPE identifier, const Node *node) ENTT_NOEXCEPT {
        return node ? node->identifier == identifier || duplicate(identifier, node->next) : false;
    }

    bool duplicate(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
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
                return std::as_const(std::get<0>(prop));
            },
            []() -> meta_any {
                return std::as_const(std::get<1>(prop));
            },
            []() ENTT_NOEXCEPT -> meta_prop {
                return &node;
            }
        };

        prop = std::forward<Property>(property);
        node.next = properties<Owner>(std::forward<Other>(other)...);
        ENTT_ASSERT(!duplicate(meta_any{std::get<0>(prop)}, node.next));
        return &node;
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

    meta_factory() ENTT_NOEXCEPT = default;

public:
    /**
     * @brief Extends a meta type by assigning it an identifier and properties.
     * @tparam Property Types of properties to assign to the meta type.
     * @param identifier Unique identifier.
     * @param property Properties to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename... Property>
    meta_factory type(const ENTT_ID_TYPE identifier, Property &&... property) ENTT_NOEXCEPT {
        ENTT_ASSERT(!internal::meta_info<Type>::type);
        auto *node = internal::meta_info<Type>::resolve();
        node->identifier = identifier;
        node->next = internal::meta_info<>::type;
        node->prop = properties<Type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(identifier, node->next));
        internal::meta_info<Type>::type = node;
        internal::meta_info<>::type = node;

        return *this;
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
    meta_factory base() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<Base, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            &internal::meta_info<Type>::template base<Base>,
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](void *instance) ENTT_NOEXCEPT -> void * {
                return static_cast<Base *>(static_cast<Type *>(instance));
            },
            []() ENTT_NOEXCEPT -> meta_base {
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
            [](const void *instance) -> meta_any {
                return static_cast<To>(*static_cast<const Type *>(instance));
            },
            []() ENTT_NOEXCEPT -> meta_conv {
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
    meta_factory conv() ENTT_NOEXCEPT {
        using conv_type = std::invoke_result_t<decltype(Candidate), Type &>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            &internal::meta_info<Type>::template conv<conv_type>,
            type,
            nullptr,
            &internal::meta_info<conv_type>::resolve,
            [](const void *instance) -> meta_any {
                return std::invoke(Candidate, *static_cast<const Type *>(instance));
            },
            []() ENTT_NOEXCEPT -> meta_conv {
                return &node;
            }
        };

        node.next = type->conv;
        ENTT_ASSERT((!internal::meta_info<Type>::template conv<conv_type>));
        internal::meta_info<Type>::template conv<conv_type> = &node;
        type->conv = &node;

        return *this;
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
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename Policy = as_is_t, typename... Property>
    meta_factory ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Func)>;
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
                return internal::invoke<Type, Func, Policy>({}, any, std::make_index_sequence<helper_type::size>{});
            },
            []() ENTT_NOEXCEPT -> meta_ctor {
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
        using helper_type = internal::meta_function_helper_t<Type(*)(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            &internal::meta_info<Type>::template ctor<typename helper_type::args_type>,
            type,
            nullptr,
            nullptr,
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, std::remove_cv_t<std::remove_reference_t<Args>>...>(any, std::make_index_sequence<helper_type::size>{});
            },
            []() ENTT_NOEXCEPT -> meta_ctor {
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
    meta_factory dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type &>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_dtor_node node{
            &internal::meta_info<Type>::template dtor<Func>,
            type,
            [](meta_handle handle) {
                return handle.type() == internal::meta_info<Type>::resolve()->meta()
                        ? (std::invoke(Func, *handle.data<Type>()), true)
                        : false;
            },
            []() ENTT_NOEXCEPT -> meta_dtor {
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
     * @tparam Policy Optional policy (no policy set by default).
     * @tparam Property Types of properties to assign to the meta data.
     * @param identifier Unique identifier.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_is_t, typename... Property>
    meta_factory data(const ENTT_ID_TYPE identifier, Property &&... property) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::resolve();
        internal::meta_data_node *curr = nullptr;

        if constexpr(std::is_same_v<Type, decltype(Data)>) {
            static_assert(std::is_same_v<Policy, as_is_t>);

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
                []() ENTT_NOEXCEPT -> meta_data {
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
                &internal::getter<Type, Data, Policy>,
                []() ENTT_NOEXCEPT -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...);
            curr = &node;
        } else {
            static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);
            using data_type = std::remove_pointer_t<std::decay_t<decltype(Data)>>;

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
                &internal::getter<Type, Data, Policy>,
                []() ENTT_NOEXCEPT -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...);
            curr = &node;
        }

        curr->identifier = identifier;
        curr->next = type->data;
        ENTT_ASSERT(!duplicate(identifier, curr->next));
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
     * @tparam Property Types of properties to assign to the meta data.
     * @param identifier Unique identifier.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_is_t, typename... Property>
    meta_factory data(const ENTT_ID_TYPE identifier, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::tuple<std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>;
        using underlying_type = std::invoke_result_t<decltype(Getter), Type &>;
        static_assert(std::is_invocable_v<decltype(Setter), Type &, underlying_type>);
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
            &internal::getter<Type, Getter, Policy>,
            []() ENTT_NOEXCEPT -> meta_data {
                return &node;
            }
        };

        node.identifier = identifier;
        node.next = type->data;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(identifier, node.next));
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
     * @tparam Candidate The actual function to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @tparam Property Types of properties to assign to the meta function.
     * @param identifier Unique identifier.
     * @param property Properties to assign to the meta function.
     * @return A meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t, typename... Property>
    meta_factory func(const ENTT_ID_TYPE identifier, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::integral_constant<decltype(Candidate), Candidate>;
        using helper_type = internal::meta_function_helper_t<decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            &internal::meta_info<Type>::template func<Candidate>,
            {},
            type,
            nullptr,
            nullptr,
            helper_type::size,
            helper_type::is_const,
            !std::is_member_function_pointer_v<decltype(Candidate)>,
            &internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename helper_type::return_type>>::resolve,
            &helper_type::arg,
            [](meta_handle handle, meta_any *any) {
                return internal::invoke<Type, Candidate, Policy>(handle, any, std::make_index_sequence<helper_type::size>{});
            },
            []() ENTT_NOEXCEPT -> meta_func {
                return &node;
            }
        };

        node.identifier = identifier;
        node.next = type->func;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(identifier, node.next));
        ENTT_ASSERT((!internal::meta_info<Type>::template func<Candidate>));
        internal::meta_info<Type>::template func<Candidate> = &node;
        type->func = &node;

        return *this;
    }

    /**
     * @brief Unregisters a meta type and all its parts.
     *
     * This function unregisters a meta type and all its data members, member
     * functions and properties, as well as its constructors, destructors and
     * conversion functions if any.<br/>
     * Base classes aren't unregistered but the link between the two types is
     * removed.
     *
     * @return True if the meta type exists, false otherwise.
     */
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

            internal::meta_info<Type>::type->identifier = {};
            internal::meta_info<Type>::type->next = nullptr;
            internal::meta_info<Type>::type = nullptr;
        }

        return registered;
    }
};


/**
 * @brief Utility function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @tparam Property Types of properties to assign to the reflected type.
 * @param identifier Unique identifier.
 * @param property Properties to assign to the reflected type.
 * @return A meta factory for the given type.
 */
template<typename Type, typename... Property>
inline meta_factory<Type> reflect(const ENTT_ID_TYPE identifier, Property &&... property) ENTT_NOEXCEPT {
    return meta_factory<Type>{}.type(identifier, std::forward<Property>(property)...);
}


/**
 * @brief Utility function to use for reflection.
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
 * @brief Utility function to unregister a type.
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
    return meta_factory<Type>{}.unregister();
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
 * @brief Returns the meta type associated with a given identifier.
 * @param identifier Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
inline meta_type resolve(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
    const auto *curr = internal::find_if([identifier](auto *node) {
        return node->identifier == identifier;
    }, internal::meta_info<>::type);

    return curr ? curr->meta() : meta_type{};
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
inline std::enable_if_t<std::is_invocable_v<Op, meta_type>, void>
resolve(Op op) ENTT_NOEXCEPT {
    internal::iterate([op = std::move(op)](auto *node) {
        op(node->meta());
    }, internal::meta_info<>::type);
}


}


#endif // ENTT_META_FACTORY_HPP

// #include "meta/meta.hpp"

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
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
    using container_type = std::unordered_map<ENTT_ID_TYPE, std::shared_ptr<Resource>>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename container_type::size_type;
    /*! @brief Type of resources managed by a cache. */
    using resource_type = ENTT_ID_TYPE;

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

    /**
     * @brief Iterates all resources.
     *
     * The function object is invoked for each element. It is provided with
     * either the resource identifier, the resource handle or both of them.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const resource_type);
     * void(resource_handle<Resource>);
     * void(const resource_type, resource_handle<Resource>);
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

            if constexpr(std::is_invocable_v<Func, resource_type>) {
                func(curr->first);
            } else if constexpr(std::is_invocable_v<Func, resource_handle<Resource>>) {
                func(resource_handle{ curr->second });
            } else {
                func(curr->first, resource_handle{ curr->second });
            }
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


#include <tuple>
#include <cstring>
#include <utility>
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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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


template<typename Ret, typename... Args, typename Type, typename Payload, typename = std::enable_if_t<std::is_convertible_v<Payload &, Type &>>>
auto to_function_pointer(Ret(*)(Type &, Args...), Payload &) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), const Class &) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, const Class &) -> Ret(*)(Args...);


template<typename Class, typename Type>
auto to_function_pointer(Type Class:: *, const Class &) -> Type(*)();


template<typename>
struct function_extent;


template<typename Ret, typename... Args>
struct function_extent<Ret(*)(Args...)> {
    static constexpr auto value = sizeof...(Args);
};


template<typename Func>
constexpr auto function_extent_v = function_extent<Func>::value;


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
    using proto_fn_type = Ret(const void *, std::tuple<Args &&...>);

    template<auto Function, std::size_t... Index>
    void connect(std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = nullptr;

        fn = [](const void *, std::tuple<Args &&...> args) -> Ret {
            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Function, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    void connect(Type &value_or_instance, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = &value_or_instance;

        fn = [](const void *payload, std::tuple<Args &&...> args) -> Ret {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(payload);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(payload));
            }

            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Candidate, *curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type &value_or_instance) ENTT_NOEXCEPT
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
        constexpr auto extent = internal::function_extent_v<decltype(internal::to_function_pointer(std::declval<decltype(Function)>()))>;
        connect<Function>(std::make_index_sequence<extent>{});
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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) ENTT_NOEXCEPT {
        constexpr auto extent = internal::function_extent_v<decltype(internal::to_function_pointer(std::declval<decltype(Candidate)>(), value_or_instance))>;
        connect<Candidate>(value_or_instance, std::make_index_sequence<extent>{});
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
        return fn(data, std::forward_as_tuple(std::forward<Args>(args)...));
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
 * @param value_or_instance A valid reference that fits the purpose.
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &value_or_instance) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Candidate, value_or_instance))>>;


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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
    inline static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier;

    template<typename...>
    // clang (since version 9) started to complain if auto is used instead of ENTT_ID_TYPE
    inline static const ENTT_ID_TYPE inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

// #include "../core/type_traits.hpp"
#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


#include <type_traits>
// #include "../config/config.h"

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


#ifndef ENTT_HWS_SUFFIX
#define ENTT_HWS_SUFFIX _hws
#endif // ENTT_HWS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
#define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else // ENTT_NO_ATOMIC
#define ENTT_MAYBE_ATOMIC(Type) Type
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
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string {
    using traits_type = internal::fnv1a_traits<ENTT_ID_TYPE>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *curr) ENTT_NOEXCEPT: str{curr} {}
        const Char *str;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr ENTT_ID_TYPE helper(ENTT_ID_TYPE partial, const Char *curr) ENTT_NOEXCEPT {
        return curr[0] == 0 ? partial : helper((partial^curr[0])*traits_type::prime, curr+1);
    }

public:
    /*! @brief Character type. */
    using value_type = Char;
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
     * const auto value = basic_hashed_string<char>::to_value("my.png");
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    static constexpr hash_type to_value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return helper(traits_type::offset, str);
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return helper(traits_type::offset, wrapper.str);
    }

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifer.
     * @param size Length of the string to hash.
     * @return The numeric representation of the string.
     */
    static hash_type to_value(const value_type *str, std::size_t size) ENTT_NOEXCEPT {
        ENTT_ID_TYPE partial{traits_type::offset};
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
        : str{curr}, hash{helper(traits_type::offset, curr)}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : str{wrapper.str}, hash{helper(traits_type::offset, wrapper.str)}
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

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const value_type *() const ENTT_NOEXCEPT { return str; }

    /*! @copydoc value */
    constexpr operator hash_type() const ENTT_NOEXCEPT { return hash; }

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


#endif // ENTT_CORE_HASHED_STRING_HPP



namespace entt {


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
    }


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
template<typename>
class sigh;


}


#endif // ENTT_SIGNAL_FWD_HPP



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
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
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
        for(auto pos = calls.size(); pos; --pos) {
            calls[pos-1](args...);
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
        bool stop = false;

        for(auto pos = calls.size(); pos && !stop; --pos) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    calls[pos-1](args...);
                    stop = func();
                } else {
                    calls[pos-1](args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    stop = func(calls[pos-1](args...));
                } else {
                    func(calls[pos-1](args...));
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
    /*! Default constructor. */
    connection() = default;

    /*! @brief Default copy constructor. */
    connection(const connection &) = default;

    /**
     * @brief Default move constructor.
     * @param other The instance to move from.
     */
    connection(connection &&other)
        : connection{}
    {
        std::swap(disconnect, other.disconnect);
        std::swap(signal, other.signal);
    }

    /*! @brief Default copy assignment operator. @return This connection. */
    connection & operator=(const connection &) = default;

    /**
     * @brief Default move assignment operator.
     * @param other The instance to move from.
     * @return This connection.
     */
    connection & operator=(connection &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            disconnect = tmp.disconnect;
            signal = tmp.signal;
        }

        return *this;
    }

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
struct scoped_connection: private connection {
    /*! Default constructor. */
    scoped_connection() = default;

    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param conn A valid connection object.
     */
    scoped_connection(const connection &conn)
        : connection{conn}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /*! @brief Default move constructor. */
    scoped_connection(scoped_connection &&) = default;

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        connection::release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const scoped_connection &) = delete;

    /**
     * @brief Default move assignment operator.
     * @return This scoped connection.
     */
    scoped_connection & operator=(scoped_connection &&) = default;

    /**
     * @brief Copies a connection.
     * @param other The connection object to copy.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const connection &other) {
        static_cast<connection &>(*this) = other;
        return *this;
    }

    /**
     * @brief Moves a connection.
     * @param other The connection object to move.
     * @return This scoped connection.
     */
    scoped_connection & operator=(connection &&other) {
        static_cast<connection &>(*this) = std::move(other);
        return *this;
    }

    using connection::operator bool;
    using connection::release;
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

    template<auto Candidate, typename Type>
    static void release(Type &value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Function>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Function>();
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : signal{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return signal->calls.empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     * @return A properly initialized connection object.
     */
    template<auto Function>
    connection connect() {
        disconnect<Function>();
        delegate<void(void *)> conn{};
        conn.template connect<&release<Function>>();
        signal->calls.emplace_back(delegate<Ret(Args...)>{connect_arg<Function>});
        return { std::move(conn), signal };
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
     * @tparam Candidate Member or free function to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type &value_or_instance) {
        disconnect<Candidate>(value_or_instance);
        delegate<void(void *)> conn{};
        conn.template connect<&sink::release<Candidate, Type>>(value_or_instance);
        signal->calls.emplace_back(delegate<Ret(Args...)>{connect_arg<Candidate>, value_or_instance});
        return { std::move(conn), signal };
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls.erase(std::remove(calls.begin(), calls.end(), delegate), calls.end());
    }

    /**
     * @brief Disconnects a member function or a free function with payload from
     * a signal.
     * @tparam Candidate Member or free function to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type &value_or_instance) {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls.erase(std::remove(calls.begin(), calls.end(), delegate), calls.end());
    }

    /**
     * @brief Disconnects member functions or free functions based on an
     * instance or specific payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<typename Type>
    void disconnect(const Type &value_or_instance) {
        auto &calls = signal->calls;
        calls.erase(std::remove_if(calls.begin(), calls.end(), [&value_or_instance](const auto &delegate) {
            return delegate.instance() == &value_or_instance;
        }), calls.end());
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
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
 * The types of the instances are `Class &`. Users must guarantee that the
 * lifetimes of the objects overcome the one of the dispatcher itself to avoid
 * crashes.
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

        sink_type sink() ENTT_NOEXCEPT {
            return entt::sink{signal};
        }

        template<typename... Args>
        void trigger(Args &&... args) {
            signal.publish({ std::forward<Args>(args)... });
        }

        template<typename... Args>
        void enqueue(Args &&... args) {
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
    sink_type<Event> sink() ENTT_NOEXCEPT {
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

        connection_type once(listener_type listener) {
            return once_list.emplace(once_list.cend(), false, std::move(listener));
        }

        connection_type on(listener_type listener) {
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

