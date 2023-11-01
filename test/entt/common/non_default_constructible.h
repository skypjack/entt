#ifndef ENTT_COMMON_NON_DEFAULT_CONSTRUCTIBLE_HPP
#define ENTT_COMMON_NON_DEFAULT_CONSTRUCTIBLE_HPP

namespace test {

struct non_default_constructible {
    non_default_constructible() = delete;

    non_default_constructible(int v)
        : value{v} {}

    int value;
};

} // namespace test

#endif
