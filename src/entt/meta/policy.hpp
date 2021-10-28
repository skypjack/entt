#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP

namespace entt {

/*! @brief Empty class type used to request the _as ref_ policy. */
struct as_ref_t {};

/*! @brief Empty class type used to request the _as cref_ policy. */
struct as_cref_t {};

/*! @brief Empty class type used to request the _as-is_ policy. */
struct as_is_t {};

/*! @brief Empty class type used to request the _as void_ policy. */
struct as_void_t {};

} // namespace entt

#endif
