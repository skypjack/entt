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
class entt_traits: private internal::entt_traits<Type> {
    using traits_type = internal::entt_traits<Type>;

public:
    /*! @brief Value type. */
    using value_type = Type;
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
    [[nodiscard]] static constexpr entity_type to_integral(const value_type value) ENTT_NOEXCEPT {
        return static_cast<entity_type>(value);
    }

    /**
     * @brief Returns the entity part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the entity part.
     */
    [[nodiscard]] static constexpr entity_type to_entity(const value_type value) ENTT_NOEXCEPT {
        return (to_integral(value) & traits_type::entity_mask);
    }

    /**
     * @brief Returns the version part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the version part.
     */
    [[nodiscard]] static constexpr version_type to_version(const value_type value) ENTT_NOEXCEPT {
        constexpr auto mask = (traits_type::version_mask << traits_type::entity_shift);
        return ((to_integral(value) & mask) >> traits_type::entity_shift);
    }

    /**
     * @brief Constructs an identifier from its parts.
     *
     * If the version part is not provided, a tombstone is returned.<br/>
     * If the entity part is not provided, a null identifier is returned.
     *
     * @param entity The entity part of the identifier.
     * @param version The version part of the identifier.
     * @return A properly constructed identifier.
     */
    [[nodiscard]] static constexpr value_type construct(const entity_type entity = traits_type::entity_mask, const version_type version = traits_type::version_mask) ENTT_NOEXCEPT {
        return value_type{(entity & traits_type::entity_mask) | (static_cast<entity_type>(version) << traits_type::entity_shift)};
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
     * @return The null representation for the given type.
     */
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const ENTT_NOEXCEPT {
        return entt_traits<Entity>::construct();
    }

    /**
     * @brief Compares two null objects.
     * @param other A null object.
     * @return True in all cases.
     */
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const null_t other) const ENTT_NOEXCEPT {
        return true;
    }

    /**
     * @brief Compares two null objects.
     * @param other A null object.
     * @return False in all cases.
     */
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const null_t other) const ENTT_NOEXCEPT {
        return false;
    }

    /**
     * @brief Compares a null object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return False if the two elements differ, true otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return entt_traits<Entity>::to_entity(entity) == entt_traits<Entity>::to_entity(*this);
    }

    /**
     * @brief Compares a null object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return True if the two elements differ, false otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
    }

    /**
     * @brief Creates a null object from an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier to turn into a null object.
     * @return The null representation for the given identifier.
     */
    template<typename Entity>
    [[nodiscard]] constexpr Entity operator|(const Entity entity) const ENTT_NOEXCEPT {
        return entt_traits<Entity>::construct(entt_traits<Entity>::to_entity(*this), entt_traits<Entity>::to_version(entity));
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
[[nodiscard]] constexpr bool operator==(const Entity entity, const null_t other) ENTT_NOEXCEPT {
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
[[nodiscard]] constexpr bool operator!=(const Entity entity, const null_t other) ENTT_NOEXCEPT {
    return !(other == entity);
}


/*! @brief Tombstone object for all entity identifiers.  */
struct tombstone_t {
    /**
     * @brief Converts the tombstone object to identifiers of any type.
     * @tparam Entity Type of entity identifier.
     * @return The tombstone representation for the given type.
     */
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const ENTT_NOEXCEPT {
        return entt_traits<Entity>::construct();
    }

    /**
     * @brief Compares two tombstone objects.
     * @param other A tombstone object.
     * @return True in all cases.
     */
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const tombstone_t other) const ENTT_NOEXCEPT {
        return true;
    }

    /**
     * @brief Compares two tombstone objects.
     * @param other A tombstone object.
     * @return False in all cases.
     */
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const tombstone_t other) const ENTT_NOEXCEPT {
        return false;
    }

    /**
     * @brief Compares a tombstone object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return False if the two elements differ, true otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const ENTT_NOEXCEPT {
        return entt_traits<Entity>::to_version(entity) == entt_traits<Entity>::to_version(*this);
    }

    /**
     * @brief Compares a tombstone object and an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier with which to compare.
     * @return True if the two elements differ, false otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const ENTT_NOEXCEPT {
        return !(entity == *this);
    }

    /**
     * @brief Creates a tombstone object from an entity identifier of any type.
     * @tparam Entity Type of entity identifier.
     * @param entity Entity identifier to turn into a tombstone object.
     * @return The tombstone representation for the given identifier.
     */
    template<typename Entity>
    [[nodiscard]] constexpr Entity operator|(const Entity entity) const ENTT_NOEXCEPT {
        return entt_traits<Entity>::construct(entt_traits<Entity>::to_entity(entity));
    }
};


/**
 * @brief Compares a tombstone object and an entity identifier of any type.
 * @tparam Entity Type of entity identifier.
 * @param entity Entity identifier with which to compare.
 * @param other A tombstone object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity entity, const tombstone_t other) ENTT_NOEXCEPT {
    return other.operator==(entity);
}


/**
 * @brief Compares a tombstone object and an entity identifier of any type.
 * @tparam Entity Type of entity identifier.
 * @param entity Entity identifier with which to compare.
 * @param other A tombstone object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity entity, const tombstone_t other) ENTT_NOEXCEPT {
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


/**
 * @brief Compile-time constant for tombstone entities.
 *
 * There exist implicit conversions from this variable to entity identifiers of
 * any allowed type. Similarly, there exist comparision operators between the
 * tombstone entity and any other entity identifier.
 */
inline constexpr tombstone_t tombstone{};


}


#endif
