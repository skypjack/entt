#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP


#include "../config/config.h"
#include "../core/attribute.h"
#include "hashed_string.hpp"
#include "fwd.hpp"


#ifndef ENTT_PRETTY_FUNCTION
#   define ENTT_TYPE_ID_API ENTT_API
#else
#   define ENTT_TYPE_ID_API
#endif


namespace entt {


#ifndef ENTT_PRETTY_FUNCTION
/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct ENTT_API type_id_generator {
    static id_type next() ENTT_NOEXCEPT {
        static id_type value{};
        return value++;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */
#endif


/**
 * @brief Types identifiers.
 * @tparam Type Type for which to generate an identifier.
 */
template<typename Type, typename = void>
struct ENTT_TYPE_ID_API type_info {
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
        static const id_type value = internal::type_id_generator::next();
        return value;
    }
#endif
};


}


#endif
