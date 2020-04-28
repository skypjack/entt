#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


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
#if defined ENTT_PRETTY_FUNCTION
    static ENTT_PRETTY_FUNCTION_CONSTEXPR() id_type id() ENTT_NOEXCEPT {
        ENTT_PRETTY_FUNCTION_CONSTEXPR(static const) auto value = entt::hashed_string::value(ENTT_PRETTY_FUNCTION);
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
