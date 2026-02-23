#ifndef ENTT_COMMON_VALUE_TYPE_H
#define ENTT_COMMON_VALUE_TYPE_H

#include <compare>
#include <type_traits>

namespace test {

template<typename... Type>
struct pointer_stable_mixin: Type... {
    static constexpr auto in_place_delete = true;
    [[nodiscard]] constexpr bool operator==(const pointer_stable_mixin &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const pointer_stable_mixin &) const noexcept = default;
};

template<typename... Type>
struct non_trivially_destructible_mixin: Type... {
    [[nodiscard]] constexpr bool operator==(const non_trivially_destructible_mixin &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const non_trivially_destructible_mixin &) const noexcept = default;
    virtual ~non_trivially_destructible_mixin() = default;
};

template<typename... Type>
struct value_type final: Type... {
    constexpr value_type() = default;
    constexpr value_type(int elem): value{elem} {}
    [[nodiscard]] constexpr bool operator==(const value_type &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const value_type &) const noexcept = default;
    int value{};
};

using pointer_stable = value_type<pointer_stable_mixin<>>;
using non_trivially_destructible = value_type<non_trivially_destructible_mixin<>>;
using pointer_stable_non_trivially_destructible = value_type<pointer_stable_mixin<non_trivially_destructible_mixin<>>>;

static_assert(std::is_trivially_destructible_v<test::pointer_stable>, "Not a trivially destructible type");
static_assert(!std::is_trivially_destructible_v<test::non_trivially_destructible>, "Trivially destructible type");
static_assert(!std::is_trivially_destructible_v<test::pointer_stable_non_trivially_destructible>, "Trivially destructible type");

} // namespace test

#endif
