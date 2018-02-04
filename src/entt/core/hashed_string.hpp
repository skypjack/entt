#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP


#include <cstddef>
#include <cstdint>


namespace entt {


/**
 * @brief Zero overhead resource identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 */
class HashedString final {
    struct ConstCharWrapper final {
        // non-explicit constructor on purpose
        constexpr ConstCharWrapper(const char *str) noexcept: str{str} {}
        const char *str;
    };

    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;

    // Fowler–Noll–Vo hash function v. 1a - the good
    static constexpr std::uint64_t helper(std::uint64_t partial, const char *str) noexcept {
        return str[0] == 0 ? partial : helper((partial^str[0])*prime, str+1);
    }

public:
    /*! @brief Unsigned integer type. */
    using hash_type = std::uint64_t;

    /**
     * @brief Constructs a hashed string from an array of const chars.
     *
     * Forcing template resolution avoids implicit conversions. An
     * human-readable identifier can be anything but a plain, old bunch of
     * characters.<br/>
     * Example of use:
     * @code{.cpp}
     * HashedString sh{"my.png"};
     * @endcode
     *
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifer.
     */
    template <std::size_t N>
    constexpr HashedString(const char (&str)[N]) noexcept
        : hash{helper(offset, str)}, str{str}
    {}

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const char *`.
     *
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr HashedString(ConstCharWrapper wrapper) noexcept
        : hash{helper(offset, wrapper.str)}, str{wrapper.str}
    {}

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the instance.
     */
    constexpr operator const char *() const noexcept { return str; }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the instance.
     */
    constexpr operator hash_type() const noexcept { return hash; }

    /**
     * @brief Compares two hashed strings.
     * @param other Hashed string with which to compare.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    constexpr bool operator==(const HashedString &other) const noexcept {
        return hash == other.hash;
    }

private:
    const hash_type hash;
    const char *str;
};


/**
 * @brief Compares two hashed strings.
 * @param lhs A valid hashed string.
 * @param rhs A valid hashed string.
 * @return True if the two hashed strings are identical, false otherwise.
 */
constexpr bool operator!=(const HashedString &lhs, const HashedString &rhs) noexcept {
    return !(lhs == rhs);
}


}


#endif // ENTT_CORE_HASHED_STRING_HPP
