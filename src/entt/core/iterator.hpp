#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

#include <iterator>
#include <memory>
#include <utility>

namespace entt {

/**
 * @brief Helper type to use as pointer with input iterators.
 * @tparam Type of wrapped value.
 */
template<typename Type>
struct input_iterator_pointer final {
    /*! @brief Pointer type. */
    using pointer = Type *;

    /*! @brief Default copy constructor, deleted on purpose. */
    input_iterator_pointer(const input_iterator_pointer &) = delete;

    /*! @brief Default move constructor. */
    constexpr input_iterator_pointer(input_iterator_pointer &&) = default;

    /**
     * @brief Constructs a proxy object by move.
     * @param val Value to use to initialize the proxy object.
     */
    constexpr input_iterator_pointer(Type &&val)
        : value{std::move(val)} {}

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This proxy object.
     */
    input_iterator_pointer &operator=(const input_iterator_pointer &) = delete;

    /**
     * @brief Default move assignment operator.
     * @return This proxy object.
     */
    constexpr input_iterator_pointer &operator=(input_iterator_pointer &&) = default;

    /**
     * @brief Access operator for accessing wrapped values.
     * @return A pointer to the wrapped value.
     */
    [[nodiscard]] constexpr pointer operator->() noexcept {
        return std::addressof(value);
    }

private:
    Type value;
};

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
    constexpr iterable_adaptor()
        : first{},
          last{} {}

    /**
     * @brief Creates an iterable object from a pair of iterators.
     * @param from Begin iterator.
     * @param to End iterator.
     */
    constexpr iterable_adaptor(iterator from, sentinel to)
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
