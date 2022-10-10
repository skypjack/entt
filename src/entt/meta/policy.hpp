#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP

#include <type_traits>

namespace entt {

/*! @brief Basic common class for meta policies. */
struct meta_policy {};

/*! @brief Empty class type used to request the _as ref_ policy. */
struct as_ref_t final: meta_policy {
    /**
     * @cond TURN_OFF_DOXYGEN
     * Internal details not to be documented.
     */
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>;
    /**
     * Internal details not to be documented.
     * @endcond
     */
};

/*! @brief Empty class type used to request the _as cref_ policy. */
struct as_cref_t final: meta_policy {
    /**
     * @cond TURN_OFF_DOXYGEN
     * Internal details not to be documented.
     */
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type>;
    /**
     * Internal details not to be documented.
     * @endcond
     */
};

/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t final: meta_policy {
    /**
     * @cond TURN_OFF_DOXYGEN
     * Internal details not to be documented.
     */
    template<typename>
    static constexpr bool value = true;
    /**
     * Internal details not to be documented.
     * @endcond
     */
};

/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t final: meta_policy {
    /**
     * @cond TURN_OFF_DOXYGEN
     * Internal details not to be documented.
     */
    template<typename>
    static constexpr bool value = true;
    /**
     * Internal details not to be documented.
     * @endcond
     */
};

} // namespace entt

#endif
