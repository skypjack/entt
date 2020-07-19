#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


#include <string_view>
#include "../config/config.h"
#include "../core/attribute.h"
#include "hashed_string.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_index {
    [[nodiscard]] static id_type next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};


template<typename Type>
[[nodiscard]] constexpr auto type_name() ENTT_NOEXCEPT {
#if defined ENTT_PRETTY_FUNCTION
    std::string_view pretty_function{ENTT_PRETTY_FUNCTION};
    auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ENTT_PRETTY_FUNCTION_PREFIX)+1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(ENTT_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
#else
    return std::string_view{};
#endif
}


}


/**
 * Internal details not to be documented.
 * @endcond
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
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
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
struct type_info {
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    [[nodiscard]] static constexpr id_type id() ENTT_NOEXCEPT {
        constexpr auto value = hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static id_type id() ENTT_NOEXCEPT {
        static const auto value = hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    [[nodiscard]] static id_type id() ENTT_NOEXCEPT {
        return type_index<Type>::value();
    }
#endif

    /**
     * @brief Returns the name of a given type.
     * @return The name of the given type.
     */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    [[nodiscard]] static constexpr std::string_view name() ENTT_NOEXCEPT {
        constexpr auto value = internal::type_name<Type>();
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static std::string_view name() ENTT_NOEXCEPT {
        static const auto value = internal::type_name<Type>();
        return value;
    }
#else
    [[nodiscard]] static constexpr std::string_view name() ENTT_NOEXCEPT {
        return internal::type_name<Type>();
    }
#endif
};


}


#endif
