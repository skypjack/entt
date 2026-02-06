#ifndef ENTT_CORE_CONCEPTS_HPP
#define ENTT_CORE_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

namespace entt {

/**
 * @brief Specifies that a type is not a cv-qualified reference.
 * @tparam Type Type to check.
 */
template<typename Type>
concept cvref_unqualified = std::is_same_v<std::remove_cvref_t<Type>, Type>;

/**
 * @brief Specifies that a type is likely an allocator type.
 * @tparam Type Type to check.
 */
template<typename Type>
concept allocator_like = requires(Type alloc, typename Type::value_type *value) {
    { alloc.allocate(0) } -> std::same_as<decltype(value)>;
    { alloc.deallocate(value, 0) } -> std::same_as<void>;
};

} // namespace entt

#endif
