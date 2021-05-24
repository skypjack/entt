#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename, typename = void>
struct entt_traits;


template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_enum_v<Type>>>
    : entt_traits<std::underlying_type_t<Type>>
{};


template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
    : entt_traits<typename Type::entity_type>
{};


template<>
struct entt_traits<std::uint32_t> {
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    using difference_type = std::int64_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0xFFF;
    static constexpr std::size_t entity_shift = 20u;
};


template<>
struct entt_traits<std::uint64_t> {
    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;
    using difference_type = std::int64_t;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    static constexpr std::size_t entity_shift = 32u;
};


}


/**
* Internal details not to be documented.
* @endcond
*/


/**
 * @brief Entity traits.
 * @tparam Type Type of identifier.
 */
template<typename Type>
class entt_traits: public internal::entt_traits<Type> {
    using traits_type = internal::entt_traits<Type>;

public:
    /*! @brief Underlying entity type. */
    using entity_type = typename traits_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Difference type. */
    using difference_type = typename traits_type::difference_type;

    /**
     * @brief Converts an entity to its underlying type.
     * @param value The value to convert.
     * @return The integral representation of the given value.
     */
    [[nodiscard]] static constexpr auto to_integral(const Type value) ENTT_NOEXCEPT {
        return static_cast<entity_type>(value);
    }

    /**
     * @brief Returns the entity part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the entity part.
     */
    [[nodiscard]] static constexpr auto to_entity(const Type value) {
        return (to_integral(value) & traits_type::entity_mask);
    }

    /**
     * @brief Returns the version part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the version part.
     */
    [[nodiscard]] static constexpr auto to_version(const Type value) {
        constexpr auto mask = (traits_type::version_mask << traits_type::entity_shift);
        return ((to_integral(value) & mask) >> traits_type::entity_shift);
    }

    /**
     * @brief Constructs an identifier from its parts.
     * @param entity The entity part of the identifier.
     * @param version The version part of the identifier.
     * @return A properly constructed identifier.
     */
    [[nodiscard]] static constexpr auto to_type(const entity_type entity, const version_type version = {}) {
        return Type{entity | (version << traits_type::entity_shift)};
    }
};


/**
 * @brief Converts an entity to its underlying type.
 * @tparam Entity The value type.
 * @param entity The value to convert.
 * @return The integral representation of the given value.
 */
template<typename Entity>
[[nodiscard]] constexpr auto to_integral(const Entity entity) ENTT_NOEXCEPT {
    return entt_traits<Entity>::to_integral(entity);
}


/*! @brief Null object for all entity identifiers.  */
struct null_t {
    /**
     * @brief Converts the null object to identifiers of any type.
     * @tparam Entity Type of entity identifier.
     * @return The null representation for the given identifier.
     */
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const ENTT_NOEXCEPT {
        return Entity{entt_traits<Entity>::entity_mask};
    }

    /**
     * @brief Compares two null objects.
     * @return True in all cases.
     */
    [[nodiscard]] constexpr bool operator==(const null_t &) const ENTT_NOEXCEPT {
        return true;
    }

    /**
     * @brief Compares two null objects.
     * @return False in all cases.
     */
    [[nodiscard]] constexpr bool operator!=(const null_t &) const ENTT_NOEXCEPT {
        return false;
    }

    /**
     * @brief Compares a null object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return False if the two elements differ, true otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity &entity) const ENTT_NOEXCEPT {
        return entt_traits<Entity>::to_entity(entity) == static_cast<typename entt_traits<Entity>::entity_type>(*this);
    }

    /**
     * @brief Compares a null object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return True if the two elements differ, false otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity &entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
    }
};


/**
 * @brief Compares a null object and an entity identifier of any type.
 * @tparam Entity Type of entity identifier.
 * @param entity Entity identifier with which to compare.
 * @param other A null object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity &entity, const null_t &other) ENTT_NOEXCEPT {
    return other.operator==(entity);
}


/**
 * @brief Compares a null object and an entity identifier of any type.
 * @tparam Entity Type of entity identifier.
 * @param entity Entity identifier with which to compare.
 * @param other A null object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity &entity, const null_t &other) ENTT_NOEXCEPT {
    return !(other == entity);
}


/**
 * @brief Compile-time constant for null entities.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * null entity and any other entity identifier.
 */
inline constexpr null_t null{};


}


#endif
