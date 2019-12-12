#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


#include <type_traits>
#include "../config/config.h"
#include "hashed_string.hpp"


namespace entt {


/**
 * @brief Types identifiers.
 * @tparam Type Type for which to generate an identifier.
 */
template<typename... Type>
struct type_id {
    static_assert(sizeof...(Type) == ((bool{true || sizeof(Type)}) + ...));

#if defined _MSC_VER
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
    static constexpr ENTT_ID_TYPE value() ENTT_NOEXCEPT {
        return entt::hashed_string{__FUNCSIG__};
    }
#elif defined __GNUC__
    /**
     * @brief Returns the numeric representation of a given type.
     * @return The numeric representation of the given type.
     */
    static constexpr ENTT_ID_TYPE value() ENTT_NOEXCEPT {
        return entt::hashed_string{__PRETTY_FUNCTION__};
    }
#endif
};


/**
 * @brief Helper variable template.
 * @tparam Type Type for which to generate an identifier.
 */
template<typename Type>
static constexpr auto type_id_v = type_id<Type>::value();


}


#endif // ENTT_CORE_TYPE_INFO_HPP
