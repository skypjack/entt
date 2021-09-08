#ifndef ENTT_CORE_MEMORY_HPP
#define ENTT_CORE_MEMORY_HPP


#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"


namespace entt {


/**
 * @brief Unwraps fancy pointers, does nothing otherwise (waiting for C++20).
 * @tparam Type Pointer type.
 * @param ptr Fancy or raw pointer.
 * @return A raw pointer that represents the address of the original pointer.
 */
template<typename Type>
[[nodiscard]] constexpr auto to_address(Type &&ptr) ENTT_NOEXCEPT {
    if constexpr(std::is_pointer_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
        return ptr;
    } else {
        return to_address(std::forward<Type>(ptr).operator->());
    }
}


/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_copy_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) ENTT_NOEXCEPT {
    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
        lhs = rhs;
    }
}


/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_move_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) ENTT_NOEXCEPT {
    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
        lhs = std::move(rhs);
    }
}


/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_swap(Allocator &lhs, Allocator &rhs) ENTT_NOEXCEPT {
    constexpr auto pocs = std::allocator_traits<Allocator>::propagate_on_container_swap::value;
    ENTT_ASSERT(pocs || lhs == rhs, "Cannot swap the containers");

    if constexpr(pocs) {
        using std::swap;
        swap(lhs, rhs);
    }
}


/**
 * @brief Checks whether a value is a power of two or not.
 * @param value A value that may or may not be a power of two.
 * @return True if the value is a power of two, false otherwise.
 */
[[nodiscard]] inline constexpr bool is_power_of_two(const std::size_t value) ENTT_NOEXCEPT {
    return value && ((value & (value - 1)) == 0);
}


/**
 * @brief Fast module utility function (powers of two only).
 * @tparam Value Compile-time page size, it must be a power of two.
 * @param value A value for which to calculate the modulus.
 * @return Remainder of division.
 */
template<std::size_t Value>
[[nodiscard]] constexpr std::size_t fast_mod(const std::size_t value) ENTT_NOEXCEPT {
    static_assert(is_power_of_two(Value), "Value must be a power of two");
    return value & (Value - 1u);
}


}


#endif
