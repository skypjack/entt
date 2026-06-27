#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP

#include "../config/module.h"

#ifndef ENTT_MODULE
#    include <compare>
#    include "../core/fwd.hpp"
#    include "../core/iterator.hpp"
#    include "../stl/concepts.hpp"
#    include "../stl/cstddef.hpp"
#    include "../stl/iterator.hpp"
#    include "../stl/utility.hpp"
#    include "context.hpp"
#endif // ENTT_MODULE

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

struct meta_base_node;

template<typename Type, typename It>
struct meta_range_iterator final {
    using value_type = stl::pair<id_type, Type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = stl::ptrdiff_t;
    using iterator_category = stl::input_iterator_tag;
    using iterator_concept = stl::random_access_iterator_tag;

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
        const meta_range_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr meta_range_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr meta_range_iterator operator--(int) noexcept {
        const meta_range_iterator orig = *this;
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
        if constexpr(stl::is_same_v<It, typename meta_context::container_type::const_iterator>) {
            return {it[value].first, Type{*ctx, *it[value].second}};
        } else {
            return {it[value].id, Type{*ctx, it[value]}};
        }
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr stl::ptrdiff_t operator-(const meta_range_iterator &other) const noexcept {
        return it - other.it;
    }

    [[nodiscard]] constexpr bool operator==(const meta_range_iterator &other) const noexcept {
        return it == other.it;
    }

    [[nodiscard]] constexpr auto operator<=>(const meta_range_iterator &other) const noexcept {
        return it <=> other.it;
    }

private:
    It it;
    const meta_ctx *ctx;
};

ENTT_MODULE_EXPORT_BEGIN

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

ENTT_MODULE_EXPORT_END

} // namespace internal
/*! @endcond */

ENTT_MODULE_EXPORT_BEGIN

/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned.
 * @tparam It Type of forward iterator.
 */
template<typename Type, stl::forward_iterator It>
using meta_range = iterable_adaptor<internal::meta_range_iterator<Type, It>>;

ENTT_MODULE_EXPORT_END

} // namespace entt

#endif
