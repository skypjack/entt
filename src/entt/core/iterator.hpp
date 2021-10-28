#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

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
    using pointer = decltype(std::addressof(std::declval<Type &>()));

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

} // namespace entt

#endif
