#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP

#include <cstddef>
#include <iterator>
#include <utility>
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "context.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type, typename It>
struct meta_range_iterator final {
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<id_type, Type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr meta_range_iterator() noexcept
        : it{},
          ctx{} {}

    constexpr meta_range_iterator(const meta_ctx &area, const It iter) noexcept
        : it{iter},
          ctx{&area} {}

    constexpr meta_range_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr meta_range_iterator operator++(int) noexcept {
        meta_range_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr meta_range_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr meta_range_iterator operator--(int) noexcept {
        meta_range_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr meta_range_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr meta_range_iterator operator+(const difference_type value) const noexcept {
        meta_range_iterator copy = *this;
        return (copy += value);
    }

    constexpr meta_range_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr meta_range_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return {it[value].first, Type{*ctx, it[value].second}};
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return {it->first, Type{*ctx, it->second}};
    }

    template<typename... Args>
    friend constexpr std::ptrdiff_t operator-(const meta_range_iterator<Args...> &, const meta_range_iterator<Args...> &) noexcept;

    template<typename... Args>
    friend constexpr bool operator==(const meta_range_iterator<Args...> &, const meta_range_iterator<Args...> &) noexcept;

    template<typename... Args>
    friend constexpr bool operator<(const meta_range_iterator<Args...> &, const meta_range_iterator<Args...> &) noexcept;

private:
    It it;
    const meta_ctx *ctx;
};

template<typename... Args>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename... Args>
[[nodiscard]] constexpr bool operator==(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename... Args>
[[nodiscard]] constexpr bool operator!=(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename... Args>
[[nodiscard]] constexpr bool operator<(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename... Args>
[[nodiscard]] constexpr bool operator>(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return rhs < lhs;
}

template<typename... Args>
[[nodiscard]] constexpr bool operator<=(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename... Args>
[[nodiscard]] constexpr bool operator>=(const meta_range_iterator<Args...> &lhs, const meta_range_iterator<Args...> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned.
 * @tparam It Type of forward iterator.
 */
template<typename Type, typename It>
using meta_range = iterable_adaptor<internal::meta_range_iterator<Type, It>>;

} // namespace entt

#endif
