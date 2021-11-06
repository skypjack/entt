#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP

#include <string_view>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/attribute.h"
#include "fwd.hpp"
#include "hashed_string.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

struct ENTT_API type_index final {
    [[nodiscard]] static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};

template<typename Type>
[[nodiscard]] constexpr auto stripped_type_name() ENTT_NOEXCEPT {
#if defined ENTT_PRETTY_FUNCTION
    std::string_view pretty_function{ENTT_PRETTY_FUNCTION};
    auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ENTT_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(ENTT_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
#else
    return std::string_view{""};
#endif
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] static constexpr std::string_view type_name(int) ENTT_NOEXCEPT {
    constexpr auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type>
[[nodiscard]] static std::string_view type_name(char) ENTT_NOEXCEPT {
    static const auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] static constexpr id_type type_hash(int) ENTT_NOEXCEPT {
    constexpr auto stripped = stripped_type_name<Type>();
    constexpr auto value = hashed_string::value(stripped.data(), stripped.size());
    return value;
}

template<typename Type>
[[nodiscard]] static id_type type_hash(char) ENTT_NOEXCEPT {
    static const auto value = [](const auto stripped) {
        return hashed_string::value(stripped.data(), stripped.size());
    }(stripped_type_name<Type>());
    return value;
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Type sequential identifier.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct ENTT_API type_index final {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_index::next();
        return value;
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator id_type() const ENTT_NOEXCEPT {
        return value();
    }
};

/**
 * @brief Type hash.
 * @tparam Type Type for which to generate a hash value.
 */
template<typename Type, typename = void>
struct type_hash final {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static constexpr id_type value() ENTT_NOEXCEPT {
        return internal::type_hash<Type>(0);
#else
    [[nodiscard]] static constexpr id_type value() ENTT_NOEXCEPT {
        return type_index<Type>::value();
#endif
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator id_type() const ENTT_NOEXCEPT {
        return value();
    }
};

/**
 * @brief Type name.
 * @tparam Type Type for which to generate a name.
 */
template<typename Type, typename = void>
struct type_name final {
    /**
     * @brief Returns the name of a given type.
     * @return The name of the given type.
     */
    [[nodiscard]] static constexpr std::string_view value() ENTT_NOEXCEPT {
        return internal::type_name<Type>(0);
    }

    /*! @copydoc value */
    [[nodiscard]] constexpr operator std::string_view() const ENTT_NOEXCEPT {
        return value();
    }
};

/*! @brief Implementation specific information about a type. */
class type_info final {
    template<typename Type>
    friend const type_info &type_id() ENTT_NOEXCEPT;

    template<typename Type>
    constexpr type_info(std::in_place_type_t<Type>) ENTT_NOEXCEPT
        : seq{type_index<std::remove_reference_t<std::remove_const_t<Type>>>::value()},
          identifier{type_hash<std::remove_reference_t<std::remove_const_t<Type>>>::value()},
          alias{type_name<std::remove_reference_t<std::remove_const_t<Type>>>::value()} {}

public:
    /**
     * @brief Type index.
     * @return Type index.
     */
    [[nodiscard]] constexpr id_type index() const ENTT_NOEXCEPT {
        return seq;
    }

    /**
     * @brief Type hash.
     * @return Type hash.
     */
    [[nodiscard]] constexpr id_type hash() const ENTT_NOEXCEPT {
        return identifier;
    }

    /**
     * @brief Type name.
     * @return Type name.
     */
    [[nodiscard]] constexpr std::string_view name() const ENTT_NOEXCEPT {
        return alias;
    }

private:
    id_type seq;
    id_type identifier;
    std::string_view alias;
};

/**
 * @brief Compares the contents of two type info objects.
 * @param lhs A type info object.
 * @param rhs A type info object.
 * @return True if the two type info objects are identical, false otherwise.
 */
[[nodiscard]] inline constexpr bool operator==(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return lhs.hash() == rhs.hash();
}

/**
 * @brief Compares the contents of two type info objects.
 * @param lhs A type info object.
 * @param rhs A type info object.
 * @return True if the two type info objects differ, false otherwise.
 */
[[nodiscard]] inline constexpr bool operator!=(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is less than the second, false otherwise.
 */
[[nodiscard]] constexpr bool operator<(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return lhs.index() < rhs.index();
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is less than or equal to the second, false
 * otherwise.
 */
[[nodiscard]] constexpr bool operator<=(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return !(rhs < lhs);
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is greater than the second, false
 * otherwise.
 */
[[nodiscard]] constexpr bool operator>(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return rhs < lhs;
}

/**
 * @brief Compares two type info objects.
 * @param lhs A valid type info object.
 * @param rhs A valid type info object.
 * @return True if the first element is greater than or equal to the second,
 * false otherwise.
 */
[[nodiscard]] constexpr bool operator>=(const type_info &lhs, const type_info &rhs) ENTT_NOEXCEPT {
    return !(lhs < rhs);
}

/**
 * @brief Returns the type info object associated to a given type.
 *
 * The type doesn't need to be a complete type. If the type is a reference, the
 * result refers to the referenced type. In all cases, top-level cv-qualifiers
 * are ignored.
 *
 * @tparam Type Type for which to generate a type info object.
 * @return A properly initialized type info object.
 */
template<typename Type>
[[nodiscard]] const type_info &type_id() ENTT_NOEXCEPT {
    if constexpr(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>) {
        static type_info instance{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>};
        return instance;
    } else {
        return type_id<std::remove_cv_t<std::remove_reference_t<Type>>>();
    }
}

} // namespace entt

#endif
