#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

#include <memory>
#include "../config/config.h"

namespace entt {

/**
 * @brief Helper type to use as pointer with input iterators.
 * @tparam Type of wrapped value.
 */
template<typename Type>
struct input_iterator_proxy {
    /**
     * @brief Constructs a proxy object from a given value.
     * @param val Value to use to initialize the proxy object.
     */
    input_iterator_proxy(Type &&val)
        : value{std::forward<Type>(val)} {}

    /**
     * @brief Access operator for accessing wrapped values.
     * @return A pointer to the wrapped value.
     */
    [[nodiscard]] Type *operator->() ENTT_NOEXCEPT {
        return std::addressof(value);
    }

private:
    Type value;
};

/**
 * @brief Deduction guide.
 * @tparam Type Type of wrapped value.
 */
template<typename Type>
input_iterator_proxy(Type &&) -> input_iterator_proxy<Type>;

} // namespace entt

#endif
