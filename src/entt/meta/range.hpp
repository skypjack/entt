#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP

#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "context.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

struct meta_base_node;

template<typename Type, typename It>
struct meta_range_iterator final {
    using value_type = std::pair<id_type, Type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
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
        if constexpr(std::is_same_v<It, typename meta_context::container_type::const_iterator>) {
            return {it[value].first, Type{*ctx, *it[value].second}};
        } else if constexpr(std::is_same_v<typename std::iterator_traits<It>::value_type, meta_base_node>) {
            return {it[value].type, Type{*ctx, it[value]}};
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

    [[nodiscard]] constexpr std::ptrdiff_t operator-(const meta_range_iterator &other) const noexcept {
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

} // namespace internal
/*! @endcond */

/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned.
 * @tparam It Type of forward iterator.
 */
template<typename Type, std::forward_iterator It>
using meta_range = iterable_adaptor<internal::meta_range_iterator<Type, It>>;

} // namespace entt

#endif
