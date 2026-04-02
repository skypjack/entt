// IWYU pragma: always_keep

#ifndef ENTT_META_POINTER_HPP
#define ENTT_META_POINTER_HPP

#include <memory>
#include "../stl/type_traits.hpp"
#include "type_traits.hpp"

namespace entt {

/**
 * @brief Makes `std::shared_ptr`s of any type pointer-like types for the meta
 * system.
 * @tparam Type Element type.
 */
template<typename Type>
struct is_meta_pointer_like<std::shared_ptr<Type>>
    : stl::true_type {};

/**
 * @brief Makes `std::unique_ptr`s of any type pointer-like types for the meta
 * system.
 * @tparam Type Element type.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct is_meta_pointer_like<std::unique_ptr<Type, Args...>>
    : stl::true_type {};

/**
 * @brief Specialization for self-proclaimed meta pointer like types.
 * @tparam Type Element type.
 */
template<typename Type>
requires requires { typename Type::is_meta_pointer_like; }
struct is_meta_pointer_like<Type>
    : stl::true_type {};

} // namespace entt

#endif
