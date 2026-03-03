#ifndef ENTT_COMMON_VALUE_TYPE_H
#define ENTT_COMMON_VALUE_TYPE_H

#include <compare>
#include <type_traits>

namespace test {

namespace internal {

template<typename Type>
struct pointer_stable_mixin: Type {
    static constexpr auto in_place_delete = true;
    using Type::Type;
    using Type::operator=;
};

template<typename Type>
struct non_default_constructible_mixin: Type {
    using Type::Type;
    using Type::operator=;
    non_default_constructible_mixin() = delete;
};

template<typename Type>
struct non_trivially_destructible_mixin: Type {
    using Type::Type;
    using Type::operator=;
    virtual ~non_trivially_destructible_mixin() noexcept = default;
};

template<typename Type>
struct non_comparable_mixin: Type {
    using Type::Type;
    using Type::operator=;
    bool operator==(const non_comparable_mixin &) const noexcept = delete;
};

template<typename Type>
struct non_movable_mixin: Type {
    using Type::Type;
    non_movable_mixin(non_movable_mixin &&) noexcept = delete;
    non_movable_mixin(const non_movable_mixin &) noexcept = default;
    non_movable_mixin &operator=(non_movable_mixin &&) noexcept = delete;
    non_movable_mixin &operator=(const non_movable_mixin &) noexcept = default;
};

template<typename Type>
struct new_delete_mixin: Type {
    static void *operator new(std::size_t count) {
        return ::operator new(count);
    }

    static void operator delete(void *ptr) {
        ::operator delete(ptr);
    }
};

struct empty_type {};

struct aggregate_type {
    [[nodiscard]] constexpr bool operator==(const aggregate_type &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const aggregate_type &) const noexcept = default;
    int value{};
};

template<typename Type>
struct value_type {
    constexpr value_type() = default;
    constexpr value_type(Type elem): value{elem} {}
    [[nodiscard]] constexpr bool operator==(const value_type &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const value_type &) const noexcept = default;
    operator Type() const noexcept {
        return value;
    }
    Type value{};
};

} // namespace internal

using pointer_stable = internal::pointer_stable_mixin<internal::value_type<int>>;
using pointer_stable_non_trivially_destructible = internal::pointer_stable_mixin<internal::non_trivially_destructible_mixin<internal::value_type<int>>>;

using non_default_constructible = internal::non_default_constructible_mixin<internal::value_type<int>>;
using non_trivially_destructible = internal::non_trivially_destructible_mixin<internal::value_type<int>>;
using non_comparable = internal::non_comparable_mixin<internal::empty_type>;
using non_movable = internal::non_movable_mixin<internal::value_type<int>>;

using new_delete = internal::new_delete_mixin<internal::value_type<int>>;

using boxed_int = internal::value_type<int>;
using boxed_char = internal::value_type<char>;

using empty = internal::empty_type;
struct other_empty: internal::empty_type {};

using aggregate = internal::aggregate_type;

static_assert(std::is_trivially_destructible_v<pointer_stable>, "Not a trivially destructible type");
static_assert(!std::is_trivially_destructible_v<non_trivially_destructible>, "Trivially destructible type");
static_assert(!std::is_trivially_destructible_v<pointer_stable_non_trivially_destructible>, "Trivially destructible type");
static_assert(!std::is_move_constructible_v<non_movable> && !std::is_move_assignable_v<non_movable>, "Movable type");
static_assert(std::is_aggregate_v<aggregate>, "Not an aggregate type");

} // namespace test

#endif
