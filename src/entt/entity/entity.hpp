#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include "../config/config.h"
#include "entt_traits.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct null {
    template<typename Entity>
    constexpr operator Entity() const ENTT_NOEXCEPT {
        using traits_type = entt_traits<Entity>;
        return traits_type::entity_mask | (traits_type::version_mask << traits_type::entity_shift);
    }

    constexpr bool operator==(null) const ENTT_NOEXCEPT {
        return true;
    }

    constexpr bool operator!=(null) const ENTT_NOEXCEPT {
        return false;
    }

    template<typename Entity>
    constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return entity == static_cast<Entity>(*this);
    }

    template<typename Entity>
    constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return entity != static_cast<Entity>(*this);
    }
};


template<typename Entity>
constexpr bool operator==(const Entity entity, null other) ENTT_NOEXCEPT {
    return other == entity;
}


template<typename Entity>
constexpr bool operator!=(const Entity entity, null other) ENTT_NOEXCEPT {
    return other != entity;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Null entity.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
constexpr auto null = internal::null{};


}


#endif // ENTT_ENTITY_ENTITY_HPP
