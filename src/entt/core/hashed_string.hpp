#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include "../config/config.h"


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
