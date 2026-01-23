#ifndef ENTT_CORE_BIT_HPP
#define ENTT_CORE_BIT_HPP

#include <bit>
#include <concepts>
#include <cstddef>
#include "../config/config.h"

namespace entt {

/**
 * @brief Fast module utility function (powers of two only).
 * @tparam Type Unsigned integer type.
 * @param value A value of unsigned integer type.
 * @param mod _Modulus_, it must be a power of two.
 * @return The common remainder.
 */
template<std::unsigned_integral Type>
[[nodiscard]] constexpr Type fast_mod(const Type value, const std::size_t mod) noexcept {
    ENTT_ASSERT_CONSTEXPR(std::has_single_bit(mod), "Value must be a power of two");
    return static_cast<Type>(value & (mod - 1u));
}

} // namespace entt

#endif
