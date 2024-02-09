#ifndef ENTT_COMMON_NON_MOVABLE_H
#define ENTT_COMMON_NON_MOVABLE_H

namespace test {

struct non_movable {
    non_movable() = default;

    non_movable(const non_movable &) = default;
    non_movable(non_movable &&) = delete;

    non_movable &operator=(const non_movable &) = default;
    non_movable &operator=(non_movable &&) = delete;

    int value{};
};

} // namespace test

#endif
