#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"
#include "long_lived_versions.hpp"


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


template<>
struct entt_traits<std::uint32_t> {
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    using difference_type = std::int64_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0xFFF;
    static constexpr std::size_t entity_shift = 20u;
    inline static const version_type default_version() {
      return version_mask;
    }

    template<typename CastFromType>
    inline static entity_type to_integral(CastFromType v) { return static_cast<entity_type>(v); }
    template<typename CastFromType>
    inline static entity_type to_entity(CastFromType v) { return static_cast<entity_type>(v) & entity_mask; }
    template<typename CastFromType>
    inline static version_type to_version(const CastFromType value) {
      constexpr auto mask = (version_mask << entity_shift);
      return ((to_integral(value) & mask) >> entity_shift);
    }
    template<typename  CastFromType>
    inline static version_type to_next_version(const CastFromType value) {
      return to_version(value) + 1u;
    }
    inline static version_type first_nonzero_version() { return 1u; }
    inline static version_type inc_version(const version_type version) { return version + 1u; }
    template<typename CastToType>
    inline static constexpr CastToType construct(const entity_type entity, const version_type version) {
        return CastToType{(entity & entity_mask) | (static_cast<entity_type>(version) << entity_shift)};
    }
};


template<>
struct entt_traits<std::uint64_t> {
    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;
    using difference_type = std::int64_t;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    static constexpr std::size_t entity_shift = 32u;
    inline static const version_type default_version() {
      return version_mask;
    }

    template<typename CastFromType>
    inline static entity_type to_integral(CastFromType v) { return static_cast<entity_type>(v); }
    template<typename CastFromType>
    inline static entity_type to_entity(CastFromType v) { return static_cast<entity_type>(v) & entity_mask; }
    template<typename CastFromType>
    inline static version_type to_version(const CastFromType value) {
      constexpr auto mask = (version_mask << entity_shift);
      return ((to_integral(value) & mask) >> entity_shift);
    }
    template<typename  CastFromType>
    inline static version_type to_next_version(const CastFromType value) {
      return to_version(value) + 1u;
    }
    inline static version_type first_nonzero_version() { return 1u; }
    inline static version_type inc_version(const version_type version) { return version + 1u; }
    template<typename CastToType>
    inline static constexpr CastToType construct(const entity_type entity, const version_type version) {
        return CastToType{(entity & entity_mask) | (static_cast<entity_type>(version) << entity_shift)};
    }
};


template <typename Type>
struct entt_traits<Type, std::enable_if_t<(std::is_same_v<LongLivedVersionIdRef, typename Type::version_type>)>>
{
  using entity_type = typename Type::entity_type;
  using version_type = typename Type::version_type;
  static constexpr entity_type entity_mask = ~((entity_type)0);
  static const typename Type::version_type version_mask;
  using difference_type = std::int64_t;
  static const version_type default_version() {
    return Type::default_version();
  }

  template<typename CastFromType>
  inline static typename Type::entity_type to_integral(CastFromType v) { return v.entity_id; }
  template<typename CastFromType>
  inline static typename Type::entity_type to_entity(CastFromType v) { return to_integral<CastFromType>(v); }
  template<typename CastFromType>
  inline static LongLivedVersionIdRef to_version(const CastFromType v) {
    return v.version_id;
  }
  template<typename  CastFromType>
  inline static version_type to_next_version(const CastFromType value) {
    LongLivedVersionIdRef c_vid = to_version(value);
    return inc_version(c_vid);
  }
  inline static version_type first_nonzero_version() { return default_version(); }
  inline static version_type inc_version(const version_type version) {
    if (version == version_mask)
      return default_version();
    return version_type(version).upgrade_lookahead();
  }
  template<typename CastToType>
  inline static constexpr CastToType construct(const entity_type entity, const version_type version) {
      return CastToType(entity, version);
  }
};

template <typename Type>
const typename Type::version_type entt_traits<Type, std::enable_if_t<(std::is_same_v<LongLivedVersionIdRef, typename Type::version_type>)>>::version_mask = typename Type::version_type{};

/*
// need to figure out a way for this more general specialization can coexist with the type-specific specialization for LL versions
template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
    : entt_traits<typename Type::entity_type>
{
  inline static const typename Type::version_type default_version() {
    return Type::version_mask;
  }
  static constexpr typename Type::version_type version_mask = nullptr;
  template<typename CastFromType>
  inline static typename Type::entity_type to_integral(const CastFromType v) { return static_cast<typename Type::entity_type>(v); }
  template<typename CastFromType>
  inline static typename Type::entity_type to_entity(const CastFromType v) { return (to_integral<CastFromType>(v)) & Type::entity_mask; }
  template<typename CastFromType>
  inline static typename Type::version_type to_version(const CastFromType value) {
    constexpr auto mask = (Type::version_mask << Type::entity_shift);
    return ((to_integral(value) & mask) >> Type::entity_shift);
  }
  template<typename CastToType>
  inline static constexpr CastToType construct(const typename Type::entity_type entity, const typename Type::version_type version) {
      return CastToType{(entity & Type::entity_mask) | (static_cast<typename Type::entity_type>(version) << Type::entity_shift)};
  }
};*/

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
        return traits_type::template to_integral<value_type>(value);
    }

    /**
     * @brief Returns the entity part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the entity part.
     */
    [[nodiscard]] static constexpr entity_type to_entity(const value_type value) ENTT_NOEXCEPT {
        return traits_type::template to_entity<value_type>(value);
    }

    /**
     * @brief Returns the version part once converted to the underlying type.
     * @param value The value to convert.
     * @return The integral representation of the version part.
     */
    [[nodiscard]] static constexpr version_type to_version(const value_type value) ENTT_NOEXCEPT {
        return traits_type::template to_version<value_type>(value);
    }

    /**
     * @brief Returns the next version of the underlying type.
     * @param value The value to convert.
     * @return The representation of the version part.
     */
    [[nodiscard]] static version_type to_next_version(const value_type value) ENTT_NOEXCEPT {
        return traits_type::template to_next_version<value_type>(value);
    }

    /**
     * @brief Returns the next version of the underlying version.
     * @param version The version to increment.
     * @return The representation of the incremented version.
     */
    [[nodiscard]] static version_type inc_version(const version_type version) ENTT_NOEXCEPT {
        return traits_type::inc_version(version);
    }

    [[nodiscard]] static version_type first_nonzero_version() ENTT_NOEXCEPT {
        return traits_type::first_nonzero_version();
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
        return traits_type::template construct<value_type>(entity, version);
        //return value_type{(entity & traits_type::entity_mask) | (static_cast<entity_type>(version) << traits_type::entity_shift)};
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
