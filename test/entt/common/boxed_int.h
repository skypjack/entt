#ifndef ENTT_COMMON_BOXED_INT_H
#define ENTT_COMMON_BOXED_INT_H

namespace test {

struct boxed_int {
    int value{};
};

inline bool operator==(const boxed_int &lhs, const boxed_int &rhs) {
    return lhs.value == rhs.value;
}

} // namespace test

#endif
