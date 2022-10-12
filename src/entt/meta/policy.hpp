#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP

#include <type_traits>

namespace entt {

/*! @brief Empty class type used to request the _as ref_ policy. */
struct as_ref_t final {
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
struct as_cref_t final {
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
struct as_is_t final {
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
struct as_void_t final {
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

/**
 * @brief Provides the member constant `value` to true if a type also is a meta
 * policy, false otherwise.
 * @tparam Type Type to check.
 */
template<typename Type>
struct is_meta_policy
    : std::disjunction<
          std::is_same<Type, as_ref_t>,
          std::is_same<Type, as_cref_t>,
          std::is_same<Type, as_is_t>,
          std::is_same<Type, as_void_t>> {};

/**
 * @brief Helper variable template.
 * @tparam Type Type to check.
 */
template<typename Type>
inline constexpr bool is_meta_policy_v = is_meta_policy<Type>::value;

} // namespace entt

#endif
