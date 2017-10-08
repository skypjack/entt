#ifndef ENTT_ENTITY_ENTT_HPP
#define ENTT_ENTITY_ENTT_HPP


#include <cstdint>


namespace entt {


/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
template<typename>
struct entt_traits;


/**
 * @brief Entity traits for a 16 bits entity identifier.
 *
 * A 16 bits entity identifier guarantees:
 * * 12 bits for the entity number (up to 4k entities).
 * * 4 bit for the version (resets in [0-15]).
 */
template<>
struct entt_traits<std::uint16_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint16_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint8_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr auto entity_mask = 0xFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr auto version_mask = 0xF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto version_shift = 12;
};


/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier guarantees:
 * * 24 bits for the entity number (suitable for almost all the games).
 * * 8 bit for the version (resets in [0-255]).
 */
template<>
struct entt_traits<std::uint32_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint32_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint16_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr auto entity_mask = 0xFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr auto version_mask = 0xFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto version_shift = 24;
};


/**
 * @brief Entity traits for a 64 bits entity identifier.
 *
 * A 64 bits entity identifier guarantees:
 * * 40 bits for the entity number (an indecently large number).
 * * 24 bit for the version (an indecently large number).
 */
template<>
struct entt_traits<std::uint64_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint64_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint32_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr auto entity_mask = 0xFFFFFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr auto version_mask = 0xFFFFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr auto version_shift = 40;
};


}


#endif // ENTT_ENTITY_ENTT_HPP
