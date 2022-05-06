#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"
#include "fwd.hpp"

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
    : entt_traits<std::underlying_type_t<Type>> {};

template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
    : entt_traits<typename Type::entity_type> {};

template<>
struct entt_traits<std::uint32_t> {
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0xFFF;
    static constexpr std::size_t entity_shift = 20u;
};

template<>
struct entt_traits<std::uint64_t> {
    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    static constexpr std::size_t entity_shift = 32u;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Entity traits.
 * @tparam Type Type of identifier.
 */
template<typename Type>
class entt_traits: internal::entt_traits<Type> {
    using base_type = internal::entt_traits<Type>;

public:
    /*! @brief Value type. */
    using value_type = Type;
    /*! @brief Underlying entity type. */
    using entity_type = typename base_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename base_type::version_type;
    /*! @brief Reserved identifier. */
    static constexpr entity_type reserved = base_type::entity_mask | (base_type::version_mask << base_type::entity_shift);
    /*! @brief Page size, default is `ENTT_SPARSE_PAGE`. */
    static constexpr auto page_size = ENTT_SPARSE_PAGE;

    /**
     * @brief Converts an entity to its underlying type.
     * @param value The value to convert.
     * @return The integral representation of the given value.
     */
    [[nodiscard]] static constexpr entity_type to_integral(const value_type value) noexcept {
        return static_cast<entity_type>(value);
    }

    /**
     * @brief Returns the entity part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the entity part.
     */
    [[nodiscard]] static constexpr entity_type to_entity(const value_type value) noexcept {
        return (to_integral(value) & base_type::entity_mask);
    }

    /**
     * @brief Returns the version part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the version part.
     */
    [[nodiscard]] static constexpr version_type to_version(const value_type value) noexcept {
        return (to_integral(value) >> base_type::entity_shift);
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
    [[nodiscard]] static constexpr value_type construct(const entity_type entity, const version_type version) noexcept {
        return value_type{(entity & base_type::entity_mask) | (static_cast<entity_type>(version) << base_type::entity_shift)};
    }

    /**
     * @brief Combines two identifiers in a single one.
     *
     * The returned identifier is a copy of the first element except for its
     * version, which is taken from the second element.
     *
     * @param lhs The identifier from which to take the entity part.
     * @param rhs The identifier from which to take the version part.
     * @return A properly constructed identifier.
     */
    [[nodiscard]] static constexpr value_type combine(const entity_type lhs, const entity_type rhs) noexcept {
        constexpr auto mask = (base_type::version_mask << base_type::entity_shift);
        return value_type{(lhs & base_type::entity_mask) | (rhs & mask)};
    }
};

/**
 * @copydoc entt_traits<Entity>::to_integral
 * @tparam Entity The value type.
 */
template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_integral(const Entity value) noexcept {
    return entt_traits<Entity>::to_integral(value);
}

/**
 * @copydoc entt_traits<Entity>::to_entity
 * @tparam Entity The value type.
 */
template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_entity(const Entity value) noexcept {
    return entt_traits<Entity>::to_entity(value);
}

/**
 * @copydoc entt_traits<Entity>::to_version
 * @tparam Entity The value type.
 */
template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::version_type to_version(const Entity value) noexcept {
    return entt_traits<Entity>::to_version(value);
}

/*! @brief Null object for all identifiers.  */
struct null_t {
    /**
     * @brief Converts the null object to identifiers of any type.
     * @tparam Entity Type of identifier.
     * @return The null representation for the given type.
     */
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept {
        using entity_traits = entt_traits<Entity>;
        return entity_traits::combine(entity_traits::reserved, entity_traits::reserved);
    }

    /**
     * @brief Compares two null objects.
     * @param other A null object.
     * @return True in all cases.
     */
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const null_t other) const noexcept {
        return true;
    }

    /**
     * @brief Compares two null objects.
     * @param other A null object.
     * @return False in all cases.
     */
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const null_t other) const noexcept {
        return false;
    }

    /**
     * @brief Compares a null object and an identifier of any type.
     * @tparam Entity Type of identifier.
     * @param entity Identifier with which to compare.
     * @return False if the two elements differ, true otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept {
        using entity_traits = entt_traits<Entity>;
        return entity_traits::to_entity(entity) == entity_traits::to_entity(*this);
    }

    /**
     * @brief Compares a null object and an identifier of any type.
     * @tparam Entity Type of identifier.
     * @param entity Identifier with which to compare.
     * @return True if the two elements differ, false otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept {
        return !(entity == *this);
    }
};

/**
 * @brief Compares a null object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param entity Identifier with which to compare.
 * @param other A null object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity entity, const null_t other) noexcept {
    return other.operator==(entity);
}

/**
 * @brief Compares a null object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param entity Identifier with which to compare.
 * @param other A null object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity entity, const null_t other) noexcept {
    return !(other == entity);
}

/*! @brief Tombstone object for all identifiers.  */
struct tombstone_t {
    /**
     * @brief Converts the tombstone object to identifiers of any type.
     * @tparam Entity Type of identifier.
     * @return The tombstone representation for the given type.
     */
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept {
        using entity_traits = entt_traits<Entity>;
        return entity_traits::combine(entity_traits::reserved, entity_traits::reserved);
    }

    /**
     * @brief Compares two tombstone objects.
     * @param other A tombstone object.
     * @return True in all cases.
     */
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const tombstone_t other) const noexcept {
        return true;
    }

    /**
     * @brief Compares two tombstone objects.
     * @param other A tombstone object.
     * @return False in all cases.
     */
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const tombstone_t other) const noexcept {
        return false;
    }

    /**
     * @brief Compares a tombstone object and an identifier of any type.
     * @tparam Entity Type of identifier.
     * @param entity Identifier with which to compare.
     * @return False if the two elements differ, true otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept {
        using entity_traits = entt_traits<Entity>;
        return entity_traits::to_version(entity) == entity_traits::to_version(*this);
    }

    /**
     * @brief Compares a tombstone object and an identifier of any type.
     * @tparam Entity Type of identifier.
     * @param entity Identifier with which to compare.
     * @return True if the two elements differ, false otherwise.
     */
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept {
        return !(entity == *this);
    }
};

/**
 * @brief Compares a tombstone object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param entity Identifier with which to compare.
 * @param other A tombstone object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity entity, const tombstone_t other) noexcept {
    return other.operator==(entity);
}

/**
 * @brief Compares a tombstone object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param entity Identifier with which to compare.
 * @param other A tombstone object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity entity, const tombstone_t other) noexcept {
    return !(other == entity);
}

/**
 * @brief Compile-time constant for null entities.
 *
 * There exist implicit conversions from this variable to identifiers of any
 * allowed type. Similarly, there exist comparison operators between the null
 * entity and any other identifier.
 */
inline constexpr null_t null{};

/**
 * @brief Compile-time constant for tombstone entities.
 *
 * There exist implicit conversions from this variable to identifiers of any
 * allowed type. Similarly, there exist comparison operators between the
 * tombstone entity and any other identifier.
 */
inline constexpr tombstone_t tombstone{};

} // namespace entt

#endif
