#ifndef ENTT_CORE_ENUM_HPP
#define ENTT_CORE_ENUM_HPP

#include <type_traits>

namespace entt {

/**
 * @brief Enable bitmask support for enum classes.
 * @tparam Type The enum type for which to enable bitmask support.
 */
template<typename Type, typename = void>
struct enum_as_bitmask: std::false_type {};

/*! @copydoc enum_as_bitmask */
template<typename Type>
struct enum_as_bitmask<Type, std::void_t<decltype(Type::_entt_enum_as_bitmask)>>: std::is_enum<Type> {};

/**
 * @brief Helper variable template.
 * @tparam Type The enum class type for which to enable bitmask support.
 */
template<typename Type>
inline constexpr bool enum_as_bitmask_v = enum_as_bitmask<Type>::value;

} // namespace entt

/**
 * @brief Operator available for enums for which bitmask support is enabled.
 * @tparam Type Enum class type.
 * @param lhs The first value to use.
 * @param rhs The second value to use.
 * @return The result of invoking the operator on the underlying types of the
 * two values provided.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type>
operator|(const Type lhs, const Type rhs) noexcept {
    return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(lhs) | static_cast<std::underlying_type_t<Type>>(rhs));
}

/*! @copydoc operator| */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type>
operator&(const Type lhs, const Type rhs) noexcept {
    return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(lhs) & static_cast<std::underlying_type_t<Type>>(rhs));
}

/*! @copydoc operator| */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type>
operator^(const Type lhs, const Type rhs) noexcept {
    return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(lhs) ^ static_cast<std::underlying_type_t<Type>>(rhs));
}

/**
 * @brief Operator available for enums for which bitmask support is enabled.
 * @tparam Type Enum class type.
 * @param value The value to use.
 * @return The result of invoking the operator on the underlying types of the
 * value provided.
 */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type>
operator~(const Type value) noexcept {
    return static_cast<Type>(~static_cast<std::underlying_type_t<Type>>(value));
}

/*! @copydoc operator~ */
template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, bool>
operator!(const Type value) noexcept {
    return !static_cast<std::underlying_type_t<Type>>(value);
}

/*! @copydoc operator| */
template<typename Type>
constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type &>
operator|=(Type &lhs, const Type rhs) noexcept {
    return (lhs = (lhs | rhs));
}

/*! @copydoc operator| */
template<typename Type>
constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type &>
operator&=(Type &lhs, const Type rhs) noexcept {
    return (lhs = (lhs & rhs));
}

/*! @copydoc operator| */
template<typename Type>
constexpr std::enable_if_t<entt::enum_as_bitmask_v<Type>, Type &>
operator^=(Type &lhs, const Type rhs) noexcept {
    return (lhs = (lhs ^ rhs));
}

#endif
