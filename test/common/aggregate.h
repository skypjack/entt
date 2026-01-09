#ifndef ENTT_COMMON_AGGREGATE_H
#define ENTT_COMMON_AGGREGATE_H

#include <type_traits>

namespace test {

struct aggregate {
    int value{};

    [[nodiscard]] constexpr bool operator==(const aggregate &other) const noexcept {
        return value == other.value;
    }

    [[nodiscard]] constexpr auto operator<=>(const aggregate &other) const noexcept {
        return value <=> other.value;
    }
};

// ensure aggregate-ness :)
static_assert(std::is_aggregate_v<test::aggregate>, "Not an aggregate type");

} // namespace test

#endif
