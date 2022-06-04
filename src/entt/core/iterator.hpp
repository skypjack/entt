#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace entt {

/**
 * @brief Helper type to use as pointer with input iterators.
 * @tparam Type of wrapped value.
 */
template<typename Type>
struct input_iterator_pointer final {
    /*! @brief Value type. */
    using value_type = Type;
    /*! @brief Pointer type. */
    using pointer = Type *;
    /*! @brief Reference type. */
    using reference = Type &;

    /**
     * @brief Constructs a proxy object by move.
     * @param val Value to use to initialize the proxy object.
     */
    constexpr input_iterator_pointer(value_type &&val) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        : value{std::move(val)} {}

    /**
     * @brief Access operator for accessing wrapped values.
     * @return A pointer to the wrapped value.
     */
    [[nodiscard]] constexpr pointer operator->() noexcept {
        return std::addressof(value);
    }

    /**
     * @brief Dereference operator for accessing wrapped values.
     * @return A reference to the wrapped value.
     */
    [[nodiscard]] constexpr reference operator*() noexcept {
        return value;
    }

private:
    Type value;
};

/**
 * @brief Plain iota iterator (waiting for C++20).
 * @tparam Type Value type.
 */
template<typename Type>
class iota_iterator final {
    static_assert(std::is_integral_v<Type>, "Not an integral type");

public:
    /*! @brief Value type, likely an integral one. */
    using value_type = Type;
    /*! @brief Invalid pointer type. */
    using pointer = void;
    /*! @brief Non-reference type, same as value type. */
    using reference = value_type;
    /*! @brief Difference type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Iterator category. */
    using iterator_category = std::input_iterator_tag;

    /*! @brief Default constructor. */
    constexpr iota_iterator() noexcept
        : current{} {}

    /**
     * @brief Constructs an iota iterator from a given value.
     * @param init The initial value assigned to the iota iterator.
     */
    constexpr iota_iterator(const value_type init) noexcept
        : current{init} {}

    /**
     * @brief Pre-increment operator.
     * @return This iota iterator.
     */
    constexpr iota_iterator &operator++() noexcept {
        return ++current, *this;
    }

    /**
     * @brief Post-increment operator.
     * @return This iota iterator.
     */
    constexpr iota_iterator operator++(int) noexcept {
        iota_iterator orig = *this;
        return ++(*this), orig;
    }

    /**
     * @brief Dereference operator.
     * @return The underlying value.
     */
    [[nodiscard]] constexpr reference operator*() const noexcept {
        return current;
    }

private:
    value_type current;
};

/**
 * @brief Comparison operator.
 * @tparam Type Value type of the iota iterator.
 * @param lhs A properly initialized iota iterator.
 * @param rhs A properly initialized iota iterator.
 * @return True if the two iterators are identical, false otherwise.
 */
template<typename Type>
[[nodiscard]] constexpr bool operator==(const iota_iterator<Type> &lhs, const iota_iterator<Type> &rhs) noexcept {
    return *lhs == *rhs;
}

/**
 * @brief Comparison operator.
 * @tparam Type Value type of the iota iterator.
 * @param lhs A properly initialized iota iterator.
 * @param rhs A properly initialized iota iterator.
 * @return True if the two iterators differ, false otherwise.
 */
template<typename Type>
[[nodiscard]] constexpr bool operator!=(const iota_iterator<Type> &lhs, const iota_iterator<Type> &rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief Utility class to create an iterable object from a pair of iterators.
 * @tparam It Type of iterator.
 * @tparam Sentinel Type of sentinel.
 */
template<typename It, typename Sentinel = It>
struct iterable_adaptor final {
    /*! @brief Value type. */
    using value_type = typename std::iterator_traits<It>::value_type;
    /*! @brief Iterator type. */
    using iterator = It;
    /*! @brief Sentinel type. */
    using sentinel = Sentinel;

    /*! @brief Default constructor. */
    constexpr iterable_adaptor() noexcept(std::is_nothrow_default_constructible_v<iterator> &&std::is_nothrow_default_constructible_v<sentinel>)
        : first{},
          last{} {}

    /**
     * @brief Creates an iterable object from a pair of iterators.
     * @param from Begin iterator.
     * @param to End iterator.
     */
    constexpr iterable_adaptor(iterator from, sentinel to) noexcept(std::is_nothrow_move_constructible_v<iterator> &&std::is_nothrow_move_constructible_v<sentinel>)
        : first{std::move(from)},
          last{std::move(to)} {}

    /**
     * @brief Returns an iterator to the beginning.
     * @return An iterator to the first element of the range.
     */
    [[nodiscard]] constexpr iterator begin() const noexcept {
        return first;
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last element of the
     * range.
     */
    [[nodiscard]] constexpr sentinel end() const noexcept {
        return last;
    }

    /*! @copydoc begin */
    [[nodiscard]] constexpr iterator cbegin() const noexcept {
        return begin();
    }

    /*! @copydoc end */
    [[nodiscard]] constexpr sentinel cend() const noexcept {
        return end();
    }

private:
    It first;
    Sentinel last;
};

} // namespace entt

#endif
