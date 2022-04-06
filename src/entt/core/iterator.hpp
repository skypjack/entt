#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

#include <iterator>
#include <memory>
#include <utility>
#include "../config/config.h"

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
    input_iterator_pointer(input_iterator_pointer &&) = default;

    /**
     * @brief Constructs a proxy object by move.
     * @param val Value to use to initialize the proxy object.
     */
    input_iterator_pointer(Type &&val)
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
    input_iterator_pointer &operator=(input_iterator_pointer &&) = default;

    /**
     * @brief Access operator for accessing wrapped values.
     * @return A pointer to the wrapped value.
     */
    [[nodiscard]] pointer operator->() ENTT_NOEXCEPT {
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
    iterable_adaptor() = default;

    /**
     * @brief Creates an iterable object from a pair of iterators.
     * @param from Begin iterator.
     * @param to End iterator.
     */
    iterable_adaptor(iterator from, sentinel to)
        : first{from},
          last{to} {}

    /**
     * @brief Returns an iterator to the beginning.
     * @return An iterator to the first element of the range.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return first;
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last element of the
     * range.
     */
    [[nodiscard]] sentinel end() const ENTT_NOEXCEPT {
        return last;
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator cbegin() const ENTT_NOEXCEPT {
        return begin();
    }

    /*! @copydoc end */
    [[nodiscard]] sentinel cend() const ENTT_NOEXCEPT {
        return end();
    }

private:
    It first;
    Sentinel last;
};

} // namespace entt

#endif
