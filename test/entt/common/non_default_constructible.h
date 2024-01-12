#ifndef ENTT_COMMON_NON_DEFAULT_CONSTRUCTIBLE_H
#define ENTT_COMMON_NON_DEFAULT_CONSTRUCTIBLE_H

namespace test {

struct non_default_constructible {
    non_default_constructible() = delete;

    non_default_constructible(int v)
        : value{v} {}

    int value;
};

} // namespace test

#endif
