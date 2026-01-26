#ifndef ENTT_COMMON_TRANSPARENT_EQUAL_TO_H
#define ENTT_COMMON_TRANSPARENT_EQUAL_TO_H

#include <concepts>
#include <type_traits>

namespace test {

struct transparent_equal_to {
    using is_transparent = void;

    template<typename Type, std::convertible_to<Type> Other>
    constexpr bool operator()(const Type &lhs, const Other &rhs) const {
        return lhs == static_cast<Type>(rhs);
    }
};

} // namespace test

#endif
