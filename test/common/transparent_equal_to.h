#ifndef ENTT_COMMON_TRANSPARENT_EQUAL_TO_H
#define ENTT_COMMON_TRANSPARENT_EQUAL_TO_H

#include <type_traits>

namespace test {

struct transparent_equal_to {
    using is_transparent = void;

    template<typename Type, typename Other>
    constexpr std::enable_if_t<std::is_convertible_v<Other, Type>, bool>
    operator()(const Type &lhs, const Other &rhs) const {
        return lhs == static_cast<Type>(rhs);
    }
};

} // namespace test

#endif
