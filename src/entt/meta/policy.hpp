#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP


namespace entt {


/*! @brief Empty class type used to request the _as ref_ policy. */
struct as_ref_t {};


/*! @brief Disambiguation tag. */
inline constexpr as_ref_t as_ref;


/*! @copydoc as_ref_t */
using as_alias_t [[deprecated("use as_ref_t instead")]] = as_ref_t;


/*! @copydoc as_ref */
[[deprecated("use as_ref instead")]]
inline constexpr as_ref_t as_alias;


/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t {};


/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t {};


}


#endif
