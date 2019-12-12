#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include "../config/config.h"
#include "../core/attribute.h"


namespace entt {


/*! @brief Sequential number generator. */
template<typename...>
struct ENTT_API generator {
    /**
     * @brief Returns the next available value.
     * @return The next available value.
     */
    static ENTT_ID_TYPE next() ENTT_NOEXCEPT {
        static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) value{};
        return value++;
    }
};


/**
 * @brief Runtime type unique identifier.
 * @tparam Type Type for which to generate an unique identifier.
 * @tparam Generator Tags to use to discriminate between different generators.
 */
template<typename Type, typename... Generator>
struct ENTT_API family {
    /**
     * @brief Statically generated unique identifier for a given type.
     * @return The runtime unique identifier for the given type.
     */
    static ENTT_ID_TYPE type() ENTT_NOEXCEPT {
        static const ENTT_ID_TYPE value = generator<Generator...>::next();
        return value;
    }
};


}


#endif // ENTT_CORE_FAMILY_HPP
