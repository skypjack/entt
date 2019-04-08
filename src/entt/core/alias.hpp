#ifndef ENTT_CORE_ALIAS_HPP
#define ENTT_CORE_ALIAS_HPP

#include <utility>
#include <type_traits>

namespace entt {

template <typename Value, typename Tag>
class alias {
public:
    using value_type = Value;
    using tag_type = Tag;

    template <typename... Args>
    constexpr explicit alias(Args &&... args) noexcept(noexcept(Value{std::forward<Args>(args)...}))
        : v{std::forward<Args>(args)...} {}

    template <typename OtherTag>
    constexpr explicit alias(const alias<Value, OtherTag> &other) noexcept(noexcept(Value{*other}))
        : v{*other} {}
    template <typename OtherTag>
    constexpr explicit alias(alias<Value, OtherTag> &&other) noexcept(noexcept(Value{std::move(*other)}))
        : v{std::move(*other)} {}

    constexpr Value &operator*() & noexcept {
        return v;
    }
    constexpr const Value &operator*() const & noexcept {
        return v;
    }
    constexpr Value &&operator*() && noexcept {
        return v;
    }
    constexpr const Value &&operator*() const && noexcept {
        return v;
    }

    constexpr Value *operator->() noexcept {
        return &v;
    }
    constexpr const Value *operator->() const noexcept {
        return &v;
    }

    constexpr bool operator==(const alias &other) const noexcept(noexcept(v == other.v)) {
        return v == other.v;
    }
    constexpr bool operator!=(const alias &other) const noexcept(noexcept(v != other.v)) {
        return v != other.v;
    }
    constexpr bool operator<(const alias &other) const noexcept(noexcept(v < other.v)) {
        return v < other.v;
    }
    constexpr bool operator>(const alias &other) const noexcept(noexcept(v > other.v)) {
        return v > other.v;
    }
    constexpr bool operator<=(const alias &other) const noexcept(noexcept(v <= other.v)) {
        return v <= other.v;
    }
    constexpr bool operator>=(const alias &other) const noexcept(noexcept(v >= other.v)) {
        return v >= other.v;
    }

private:
    Value v;
};

template <typename Value, typename Tag>
constexpr void swap(alias<Value, Tag> &lhs, alias<Value, Tag> &rhs) noexcept(noexcept(std::is_nothrow_swappable_v<Value>)) {
    using std::swap;
    swap(*lhs, *rhs);
}

}

template <typename Value, typename Tag>
struct std::hash<entt::alias<Value, Tag>> {
    constexpr size_t operator()(const entt::alias<Value, Tag> &alias) const noexcept(noexcept(std::hash<Value>{}(*alias))) {
        return std::hash<Value>{}(*alias);
    }
};

#endif // ENTT_CORE_ALIAS_HPP
