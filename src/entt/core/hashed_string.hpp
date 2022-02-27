#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP

#include <cstddef>
#include <cstdint>
#include "../config/config.h"
#include "fwd.hpp"

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

template<typename Char>
struct basic_hashed_string {
    using value_type = Char;
    using size_type = std::size_t;
    using hash_type = id_type;

    const value_type *repr;
    size_type length;
    hash_type hash;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifiers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 *
 * @warning
 * This class doesn't take ownership of user-supplied strings nor does it make a
 * copy of them.
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string: internal::basic_hashed_string<Char> {
    using base_type = internal::basic_hashed_string<Char>;
    using hs_traits = internal::fnv1a_traits<id_type>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const Char *str) ENTT_NOEXCEPT: repr{str} {}
        const Char *repr;
    };

    // Fowler–Noll–Vo hash function v. 1a - the good
    [[nodiscard]] static constexpr auto helper(const Char *str) ENTT_NOEXCEPT {
        base_type base{str, 0u, hs_traits::offset};

        for(; str[base.length]; ++base.length) {
            base.hash = (base.hash ^ static_cast<hs_traits::type>(str[base.length])) * hs_traits::prime;
        }

        return base;
    }

    // Fowler–Noll–Vo hash function v. 1a - the good
    [[nodiscard]] static constexpr auto helper(const Char *str, const std::size_t len) ENTT_NOEXCEPT {
        base_type base{str, len, hs_traits::offset};

        for(size_type pos{}; pos < len; ++pos) {
            base.hash = (base.hash ^ static_cast<hs_traits::type>(str[pos])) * hs_traits::prime;
        }

        return base;
    }

public:
    /*! @brief Character type. */
    using value_type = typename base_type::value_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;
    /*! @brief Unsigned integer type. */
    using hash_type = typename base_type::hash_type;

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifier.
     * @param len Length of the string to hash.
     * @return The numeric representation of the string.
     */
    [[nodiscard]] static constexpr hash_type value(const value_type *str, const size_type len) ENTT_NOEXCEPT {
        return basic_hashed_string{str, len};
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifier.
     * @return The numeric representation of the string.
     */
    template<std::size_t N>
    [[nodiscard]] static constexpr hash_type value(const value_type (&str)[N]) ENTT_NOEXCEPT {
        return basic_hashed_string{str};
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    [[nodiscard]] static constexpr hash_type value(const_wrapper wrapper) ENTT_NOEXCEPT {
        return basic_hashed_string{wrapper};
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() ENTT_NOEXCEPT
        : base_type{} {}

    /**
     * @brief Constructs a hashed string from a string view.
     * @param str Human-readable identifier.
     * @param len Length of the string to hash.
     */
    constexpr basic_hashed_string(const value_type *str, const size_type len) ENTT_NOEXCEPT
        : base_type{helper(str, len)} {}

    /**
     * @brief Constructs a hashed string from an array of const characters.
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifier.
     */
    template<std::size_t N>
    constexpr basic_hashed_string(const value_type (&str)[N]) ENTT_NOEXCEPT
        : base_type{helper(str)} {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     *
     * @warning
     * The lifetime of the string is not extended nor is it copied.
     *
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) ENTT_NOEXCEPT
        : base_type{helper(wrapper.repr)} {}

    /**
     * @brief Returns the size a hashed string.
     * @return The size of the hashed string.
     */
    [[nodiscard]] constexpr size_type size() const ENTT_NOEXCEPT {
        return base_type::length;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the hashed string.
     */
    [[nodiscard]] constexpr const value_type *data() const ENTT_NOEXCEPT {
        return base_type::repr;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the hashed string.
     */
    [[nodiscard]] constexpr hash_type value() const ENTT_NOEXCEPT {
        return base_type::hash;
    }

    /*! @copydoc data */
    [[nodiscard]] constexpr operator const value_type *() const ENTT_NOEXCEPT {
        return data();
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the hashed string.
     */
    [[nodiscard]] constexpr operator hash_type() const ENTT_NOEXCEPT {
        return value();
    }
};

/**
 * @brief Deduction guide.
 * @tparam Char Character type.
 * @param str Human-readable identifier.
 * @param len Length of the string to hash.
 */
template<typename Char>
basic_hashed_string(const Char *str, const std::size_t len) -> basic_hashed_string<Char>;

/**
 * @brief Deduction guide.
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifier.
 */
template<typename Char, std::size_t N>
basic_hashed_string(const Char (&str)[N]) -> basic_hashed_string<Char>;

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator==(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return lhs.value() == rhs.value();
}

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings differ, false otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the first element is less than the second, false otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator<(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return lhs.value() < rhs.value();
}

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the first element is less than or equal to the second, false
 * otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator<=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(rhs < lhs);
}

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the first element is greater than the second, false
 * otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator>(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return rhs < lhs;
}

/**
 * @brief Compares two hashed strings.
 * @tparam Char Character type.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the first element is greater than or equal to the second,
 * false otherwise.
 */
template<typename Char>
[[nodiscard]] constexpr bool operator>=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) ENTT_NOEXCEPT {
    return !(lhs < rhs);
}

/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;

/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;

inline namespace literals {

/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
[[nodiscard]] constexpr hashed_string operator"" _hs(const char *str, std::size_t) ENTT_NOEXCEPT {
    return hashed_string{str};
}

/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
[[nodiscard]] constexpr hashed_wstring operator"" _hws(const wchar_t *str, std::size_t) ENTT_NOEXCEPT {
    return hashed_wstring{str};
}

} // namespace literals

} // namespace entt

#endif
