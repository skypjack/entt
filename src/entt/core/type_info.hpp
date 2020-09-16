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


struct ENTT_API type_seq {
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
struct ENTT_API type_seq {
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const id_type value = internal::type_seq::next();
        return value;
    }
};


/**
* @brief Type hash.
* @tparam Type Type for which to generate a hash value.
*/
template<typename Type, typename = void>
struct type_hash {
    /**
    * @brief Returns the numeric representation of a given type.
    * @return The numeric representation of the given type.
    */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    [[nodiscard]] static constexpr id_type value() ENTT_NOEXCEPT {
        constexpr auto value = hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const auto value = hashed_string::value(ENTT_PRETTY_FUNCTION);
        return value;
    }
#else
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        return type_seq<Type>::value();
    }
#endif
};


/**
* @brief Type hash.
* @tparam Type Type for which to generate a name.
*/
template<typename Type, typename = void>
struct type_name {
    /**
    * @brief Returns the name of a given type.
    * @return The name of the given type.
    */
#if defined ENTT_PRETTY_FUNCTION_CONSTEXPR
    [[nodiscard]] static constexpr std::string_view value() ENTT_NOEXCEPT {
        constexpr auto value = internal::type_name<Type>();
        return value;
    }
#elif defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static std::string_view value() ENTT_NOEXCEPT {
        static const auto value = internal::type_name<Type>();
        return value;
    }
#else
    [[nodiscard]] static constexpr std::string_view value() ENTT_NOEXCEPT {
        return internal::type_name<Type>();
    }
#endif
};


}


#endif
