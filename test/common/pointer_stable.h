#ifndef ENTT_COMMON_POINTER_STABLE_H
#define ENTT_COMMON_POINTER_STABLE_H

#include <compare>

namespace test {

struct pointer_stable {
    static constexpr auto in_place_delete = true;
    int value{};

    [[nodiscard]] constexpr bool operator==(const pointer_stable &other) const noexcept {
        return value == other.value;
    }

    [[nodiscard]] constexpr auto operator<=>(const pointer_stable &other) const noexcept {
        return value <=> other.value;
    }
};

} // namespace test

#endif
