#ifndef ENTT_COMMON_POINTER_STABLE_H
#define ENTT_COMMON_POINTER_STABLE_H

namespace test {

struct pointer_stable {
    static constexpr auto in_place_delete = true;
    int value{};
};

[[nodiscard]] inline bool operator==(const pointer_stable &lhs, const pointer_stable &rhs) {
    return lhs.value == rhs.value;
}

[[nodiscard]] inline bool operator<(const pointer_stable &lhs, const pointer_stable &rhs) {
    return lhs.value < rhs.value;
}

} // namespace test

#endif
