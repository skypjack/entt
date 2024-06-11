#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename = void>
struct entt_traits;

template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_enum_v<Type>>>
    : entt_traits<std::underlying_type_t<Type>> {
    using value_type = Type;
};

template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
    : entt_traits<typename Type::entity_type> {
    using value_type = Type;
};

template<>
struct entt_traits<std::uint32_t> {
    using value_type = std::uint32_t;

    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0xFFF;
};

template<>
struct entt_traits<std::uint64_t> {
    using value_type = std::uint64_t;

    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Common basic entity traits implementation.
 * @tparam Traits Actual entity traits to use.
 */
template<typename Traits>
class basic_entt_traits {
    static constexpr auto length = popcount(Traits::entity_mask);

    static_assert(Traits::entity_mask && ((Traits::entity_mask & (Traits::entity_mask + 1)) == 0), "Invalid entity mask");
    static_assert((Traits::version_mask & (Traits::version_mask + 1)) == 0, "Invalid version mask");

public:
    /*! @brief Value type. */
    using value_type = typename Traits::value_type;
    /*! @brief Underlying entity type. */
    using entity_type = typename Traits::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename Traits::version_type;

    /*! @brief Entity mask size. */
    static constexpr entity_type entity_mask = Traits::entity_mask;
    /*! @brief Version mask size */
    static constexpr entity_type version_mask = Traits::version_mask;

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
        return (to_integral(value) & entity_mask);
    }

    /**
     * @brief Returns the version part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the version part.
     */
    [[nodiscard]] static constexpr version_type to_version(const value_type value) noexcept {
        if constexpr(Traits::version_mask == 0u) {
            return version_type{};
        } else {
            return (static_cast<version_type>(to_integral(value) >> length) & version_mask);
        }
    }

    /**
     * @brief Returns the successor of a given identifier.
     * @param value The identifier of which to return the successor.
     * @return The successor of the given identifier.
     */
    [[nodiscard]] static constexpr value_type next(const value_type value) noexcept {
        const auto vers = to_version(value) + 1;
        return construct(to_integral(value), static_cast<version_type>(vers + (vers == version_mask)));
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
        if constexpr(Traits::version_mask == 0u) {
            return value_type{entity & entity_mask};
        } else {
            return value_type{(entity & entity_mask) | (static_cast<entity_type>(version & version_mask) << length)};
        }
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
        if constexpr(Traits::version_mask == 0u) {
            return value_type{lhs & entity_mask};
        } else {
            return value_type{(lhs & entity_mask) | (rhs & (version_mask << length))};
        }
    }
};

/**
 * @brief Entity traits.
 * @tparam Type Type of identifier.
 */
template<typename Type>
struct entt_traits: basic_entt_traits<internal::entt_traits<Type>> {
    /*! @brief Base type. */
    using base_type = basic_entt_traits<internal::entt_traits<Type>>;
    /*! @brief Page size, default is `ENTT_SPARSE_PAGE`. */
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

/**
 * @brief Converts an entity to its underlying type.
 * @tparam Entity The value type.
 * @param value The value to convert.
 * @return The integral representation of the given value.
 */
template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_integral(const Entity value) noexcept {
    return entt_traits<Entity>::to_integral(value);
}

/**
 * @brief Returns the entity part once converted to the underlying type.
 * @tparam Entity The value type.
 * @param value The value to convert.
 * @return The integral representation of the entity part.
 */
template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_entity(const Entity value) noexcept {
    return entt_traits<Entity>::to_entity(value);
}

/**
 * @brief Returns the version part once converted to the underlying type.
 * @tparam Entity The value type.
 * @param value The value to convert.
 * @return The integral representation of the version part.
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
        using traits_type = entt_traits<Entity>;
        constexpr auto value = traits_type::construct(traits_type::entity_mask, traits_type::version_mask);
        return value;
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
        using traits_type = entt_traits<Entity>;
        return traits_type::to_entity(entity) == traits_type::to_entity(*this);
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
 * @param lhs Identifier with which to compare.
 * @param rhs A null object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity lhs, const null_t rhs) noexcept {
    return rhs.operator==(lhs);
}

/**
 * @brief Compares a null object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param lhs Identifier with which to compare.
 * @param rhs A null object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity lhs, const null_t rhs) noexcept {
    return !(rhs == lhs);
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
        using traits_type = entt_traits<Entity>;
        constexpr auto value = traits_type::construct(traits_type::entity_mask, traits_type::version_mask);
        return value;
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
        using traits_type = entt_traits<Entity>;

        if constexpr(traits_type::version_mask == 0u) {
            return false;
        } else {
            return (traits_type::to_version(entity) == traits_type::to_version(*this));
        }
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
 * @param lhs Identifier with which to compare.
 * @param rhs A tombstone object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity lhs, const tombstone_t rhs) noexcept {
    return rhs.operator==(lhs);
}

/**
 * @brief Compares a tombstone object and an identifier of any type.
 * @tparam Entity Type of identifier.
 * @param lhs Identifier with which to compare.
 * @param rhs A tombstone object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity lhs, const tombstone_t rhs) noexcept {
    return !(rhs == lhs);
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
