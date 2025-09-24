#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP

#include <type_traits>

namespace entt {

/*! @brief Empty class type used to request the _as ref_ policy. */
struct as_ref_t final {
    /*! @cond TURN_OFF_DOXYGEN */
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>;
    /*! @endcond */
};

/*! @brief Empty class type used to request the _as cref_ policy. */
struct as_cref_t final {
    /*! @cond TURN_OFF_DOXYGEN */
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type>;
    /*! @endcond */
};

/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t final {
    /*! @cond TURN_OFF_DOXYGEN */
    template<typename>
    static constexpr bool value = true;
    /*! @endcond */
};

/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t final {
    /*! @cond TURN_OFF_DOXYGEN */
    template<typename>
    static constexpr bool value = true;
    /*! @endcond */
};

/**
 * @brief Provides the member constant `value` to true if a type also is a meta
 * policy, false otherwise.
 * @tparam Type Type to check.
 */
template<typename Type>
struct is_meta_policy
    : std::bool_constant<std::is_same_v<Type, as_ref_t> || std::is_same_v<Type, as_cref_t> || std::is_same_v<Type, as_is_t> || std::is_same_v<Type, as_void_t>> {};

/**
 * @brief Helper variable template.
 * @tparam Type Type to check.
 */
template<typename Type>
inline constexpr bool is_meta_policy_v = is_meta_policy<Type>::value;

} // namespace entt

#endif
