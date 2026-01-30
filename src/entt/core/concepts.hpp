#ifndef ENTT_CORE_CONCEPTS_HPP
#define ENTT_CORE_CONCEPTS_HPP

#include <type_traits>

namespace entt {

/**
 * @brief Specifies that a type is not a cv-qualified reference.
 * @tparam Type Type to check.
 */
template<typename Type>
concept cvref_unqualified = std::is_same_v<std::remove_cvref_t<Type>, Type>;

} // namespace entt

#endif
