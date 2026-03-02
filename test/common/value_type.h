#ifndef ENTT_COMMON_VALUE_TYPE_H
#define ENTT_COMMON_VALUE_TYPE_H

#include <compare>
#include <type_traits>

namespace test {

namespace internal {

template<typename Type>
struct pointer_stable_mixin: Type {
    static constexpr auto in_place_delete = true;
    using Type::Type;
    using Type::operator=;
};

template<typename Type>
struct non_default_constructible_mixin: Type {
    using Type::Type;
    using Type::operator=;
    non_default_constructible_mixin() = delete;
};

template<typename Type>
struct non_trivially_destructible_mixin: Type {
    using Type::Type;
    using Type::operator=;
    virtual ~non_trivially_destructible_mixin() noexcept = default;
};

template<typename Type>
struct non_comparable_mixin: Type {
    using Type::Type;
    using Type::operator=;
    bool operator==(const non_comparable_mixin &) const noexcept = delete;
};

template<typename Type>
struct non_movable_mixin: Type {
    using Type::Type;
    non_movable_mixin(non_movable_mixin &&) noexcept = delete;
    non_movable_mixin(const non_movable_mixin &) noexcept = default;
    non_movable_mixin &operator=(non_movable_mixin &&) noexcept = delete;
    non_movable_mixin &operator=(const non_movable_mixin &) noexcept = default;
};

struct value_type {
    constexpr value_type() = default;
    constexpr value_type(int elem): value{elem} {}
    [[nodiscard]] constexpr bool operator==(const value_type &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const value_type &) const noexcept = default;
    int value{};
};

} // namespace internal

using pointer_stable = internal::pointer_stable_mixin<internal::value_type>;
using non_default_constructible = internal::non_default_constructible_mixin<internal::value_type>;
using non_trivially_destructible = internal::non_trivially_destructible_mixin<internal::value_type>;
using pointer_stable_non_trivially_destructible = internal::pointer_stable_mixin<internal::non_trivially_destructible_mixin<internal::value_type>>;
using non_comparable = internal::non_comparable_mixin<internal::value_type>;
using non_movable = internal::non_movable_mixin<internal::value_type>;

static_assert(std::is_trivially_destructible_v<test::pointer_stable>, "Not a trivially destructible type");
static_assert(!std::is_trivially_destructible_v<test::non_trivially_destructible>, "Trivially destructible type");
static_assert(!std::is_trivially_destructible_v<test::pointer_stable_non_trivially_destructible>, "Trivially destructible type");
static_assert(!std::is_move_constructible_v<test::non_movable> && !std::is_move_assignable_v<test::non_movable>, "Movable type");

} // namespace test

#endif
