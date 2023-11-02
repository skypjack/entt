#ifndef ENTT_COMMON_AGGREGATE_HPP
#define ENTT_COMMON_AGGREGATE_HPP

#include <type_traits>

namespace test {

struct aggregate {
    int value{};
};

inline bool operator==(const aggregate &lhs, const aggregate &rhs) {
    return lhs.value == rhs.value;
}

inline bool operator<(const aggregate &lhs, const aggregate &rhs) {
    return lhs.value < rhs.value;
}

// ensure aggregate-ness :)
static_assert(std::is_aggregate_v<test::aggregate>, "Not an aggregate type");

} // namespace test

#endif
