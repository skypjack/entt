#ifndef ENTT_COMMON_NON_TRIVIALLY_DESTRUCTIBLE_H
#define ENTT_COMMON_NON_TRIVIALLY_DESTRUCTIBLE_H

#include <compare>
#include <type_traits>

namespace test {

struct non_trivially_destructible final {
    ~non_trivially_destructible() {}
    [[nodiscard]] constexpr bool operator==(const non_trivially_destructible &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const non_trivially_destructible &) const noexcept = default;
    int value{};
};

// ensure non trivially destructible-ness :)
static_assert(!std::is_trivially_destructible_v<test::non_trivially_destructible>, "Not a trivially destructible type");

} // namespace test

#endif
