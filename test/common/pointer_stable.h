#ifndef ENTT_COMMON_POINTER_STABLE_H
#define ENTT_COMMON_POINTER_STABLE_H

#include <compare>

namespace test {

struct pointer_stable {
    static constexpr auto in_place_delete = true;
    [[nodiscard]] constexpr bool operator==(const pointer_stable &other) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const pointer_stable &other) const noexcept = default;
    int value{};
};

} // namespace test

#endif
