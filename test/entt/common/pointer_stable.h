#ifndef ENTT_COMMON_POINTER_STABLE_HPP
#define ENTT_COMMON_POINTER_STABLE_HPP

#include <type_traits>

namespace test {

struct pointer_stable {
    static constexpr auto in_place_delete = true;
    int value{};
};

inline bool operator==(const pointer_stable &lhs, const pointer_stable &rhs) {
    return lhs.value == rhs.value;
}

inline bool operator<(const pointer_stable &lhs, const pointer_stable &rhs) {
    return lhs.value < rhs.value;
}

// ensure that we've at least an aggregate type to test here
static_assert(std::is_aggregate_v<test::pointer_stable>, "Not an aggregate type");

} // namespace test

#endif
