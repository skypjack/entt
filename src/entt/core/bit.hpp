#ifndef ENTT_CORE_BIT_HPP
#define ENTT_CORE_BIT_HPP

#include <cstddef>
#include <limits>
#include <type_traits>
#include "../config/config.h"

namespace entt {

/**
 * @brief Returns the number of set bits in a value (waiting for C++20 and
 * `std::popcount`).
 * @tparam Type Unsigned integer type.
 * @param value A value of unsigned integer type.
 * @return The number of set bits in the value.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, int> popcount(const Type value) noexcept {
    return value ? (int(value & 1) + popcount(static_cast<Type>(value >> 1))) : 0;
}

/**
 * @brief Checks whether a value is a power of two or not (waiting for C++20 and
 * `std::has_single_bit`).
 * @tparam Type Unsigned integer type.
 * @param value A value of unsigned integer type.
 * @return True if the value is a power of two, false otherwise.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, bool> has_single_bit(const Type value) noexcept {
    return value && ((value & (value - 1)) == 0);
}

/**
 * @brief Computes the smallest power of two greater than or equal to a value
 * (waiting for C++20 and `std::bit_ceil`).
 * @tparam Type Unsigned integer type.
 * @param value A value of unsigned integer type.
 * @return The smallest power of two greater than or equal to the given value.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, Type> next_power_of_two(const Type value) noexcept {
    // NOLINTNEXTLINE(bugprone-assert-side-effect)
    ENTT_ASSERT_CONSTEXPR(value < (Type{1u} << (std::numeric_limits<Type>::digits - 1)), "Numeric limits exceeded");
    Type curr = value - (value != 0u);

    for(int next = 1; next < std::numeric_limits<Type>::digits; next = next * 2) {
        curr |= (curr >> next);
    }

    return ++curr;
}

/**
 * @brief Fast module utility function (powers of two only).
 * @tparam Type Unsigned integer type.
 * @param value A value of unsigned integer type.
 * @param mod _Modulus_, it must be a power of two.
 * @return The common remainder.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, Type> fast_mod(const Type value, const std::size_t mod) noexcept {
    ENTT_ASSERT_CONSTEXPR(has_single_bit(mod), "Value must be a power of two");
    return value & (mod - 1u);
}

} // namespace entt

#endif
