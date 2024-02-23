#ifndef ENTT_COMMON_BOXED_TYPE_H
#define ENTT_COMMON_BOXED_TYPE_H

namespace test {

template<typename Type>
struct boxed_type {
    Type value{};

    operator Type() const noexcept {
        return value;
    }
};

template<typename Type>
inline bool operator==(const boxed_type<Type> &lhs, const boxed_type<Type> &rhs) {
    return lhs.value == rhs.value;
}

using boxed_int = boxed_type<int>;
using boxed_char = boxed_type<char>;

} // namespace test

#endif
