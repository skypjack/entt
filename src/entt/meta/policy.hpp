#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP


namespace entt {


/*! @brief Empty class type used for disambiguation. */
struct as_is_t {};


/*! @brief Disambiguation tag. */
constexpr as_is_t as_is;


/*! @brief Empty class type used for disambiguation. */
struct as_alias_t {};


/*! @brief Disambiguation tag. */
constexpr as_alias_t as_alias;


/*! @brief Empty class type used for disambiguation. */
struct as_void_t {};


/*! @brief Disambiguation tag. */
constexpr as_void_t as_void;


}


#endif // ENTT_META_POLICY_HPP
