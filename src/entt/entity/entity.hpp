#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"


namespace entt {


/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
template<typename, typename = void>
struct entt_traits;


/**
 * @brief Entity traits for enumeration types.
 * @tparam Type The type to check.
 */
template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_enum_v<Type>>>
        : entt_traits<std::underlying_type_t<Type>>
{};


/**
 * @brief Entity traits for a 16 bits entity identifier.
 *
 * A 16 bits entity identifier guarantees:
 *
 * * 12 bits for the entity number (up to 4k entities).
 * * 4 bit for the version (resets in [0-15]).
 */
template<>
struct entt_traits<std::uint16_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint16_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint8_t;
    /*! @brief Difference type. */
    using difference_type = std::int32_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 12u;
};


/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier guarantees:
 *
 * * 20 bits for the entity number (suitable for almost all the games).
 * * 12 bit for the version (resets in [0-4095]).
 */
template<>
struct entt_traits<std::uint32_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint32_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint16_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 20u;
};


/**
 * @brief Entity traits for a 64 bits entity identifier.
 *
 * A 64 bits entity identifier guarantees:
 *
 * * 32 bits for the entity number (an indecently large number).
 * * 32 bit for the version (an indecently large number).
 */
template<>
struct entt_traits<std::uint64_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint64_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint32_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 32u;
};


/**
 * @brief Converts an entity type to its underlying type.
 * @tparam Entity The value type.
 * @param entity The value to convert.
 * @return The integral representation of the given value.
 */
template<typename Entity>
[[nodiscard]] constexpr auto to_integral(const Entity entity) ENTT_NOEXCEPT {
    return static_cast<typename entt_traits<Entity>::entity_type>(entity);
}


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct null {
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const ENTT_NOEXCEPT {
        return Entity{entt_traits<Entity>::entity_mask};
    }

    [[nodiscard]] constexpr bool operator==(null) const ENTT_NOEXCEPT {
        return true;
    }

    [[nodiscard]] constexpr bool operator!=(null) const ENTT_NOEXCEPT {
        return false;
    }

    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return (to_integral(entity) & entt_traits<Entity>::entity_mask) == to_integral(static_cast<Entity>(*this));
    }

    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
    }
};


template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity entity, null other) ENTT_NOEXCEPT {
    return other.operator==(entity);
}


template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity entity, null other) ENTT_NOEXCEPT {
    return !(other == entity);
}


}


/**
 * Internal details not to be documented.
 * @endcond
 */


/**
 * @brief Compile-time constant for null entities.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
inline constexpr auto null = internal::null{};


}


#endif
