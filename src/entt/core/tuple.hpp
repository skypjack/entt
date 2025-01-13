#ifndef ENTT_CORE_TUPLE_HPP
#define ENTT_CORE_TUPLE_HPP

#include <tuple>
#include <type_traits>
#include <utility>

namespace entt {

/**
 * @brief Provides the member constant `value` to true if a given type is a
 * tuple, false otherwise.
 * @tparam Type The type to test.
 */
template<typename Type>
struct is_tuple: std::false_type {};

/**
 * @copybrief is_tuple
 * @tparam Args Tuple template arguments.
 */
template<typename... Args>
struct is_tuple<std::tuple<Args...>>: std::true_type {};

/**
 * @brief Helper variable template.
 * @tparam Type The type to test.
 */
template<typename Type>
inline constexpr bool is_tuple_v = is_tuple<Type>::value;

/**
 * @brief Utility function to unwrap tuples of a single element.
 * @tparam Type Tuple type of any sizes.
 * @param value A tuple object of the given type.
 * @return The tuple itself if it contains more than one element, the first
 * element otherwise.
 */
template<typename Type>
constexpr decltype(auto) unwrap_tuple(Type &&value) noexcept {
    if constexpr(std::tuple_size_v<std::remove_reference_t<Type>> == 1u) {
        return std::get<0>(std::forward<Type>(value));
    } else {
        return std::forward<Type>(value);
    }
}

/**
 * @brief Utility class to forward-and-apply tuple objects.
 * @tparam Func Type of underlying invocable object.
 */
template<typename Func>
struct forward_apply: private Func {
    /**
     * @brief Constructs a forward-and-apply object.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename... Args>
    constexpr forward_apply(Args &&...args) noexcept(std::is_nothrow_constructible_v<Func, Args...>)
        : Func{std::forward<Args>(args)...} {}

    /**
     * @brief Forwards and applies the arguments with the underlying function.
     * @tparam Type Tuple-like type to forward to the underlying function.
     * @param args Parameters to forward to the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template<typename Type>
    constexpr decltype(auto) operator()(Type &&args) noexcept(noexcept(std::apply(std::declval<Func &>(), args))) {
        return std::apply(static_cast<Func &>(*this), std::forward<Type>(args));
    }

    /*! @copydoc operator()() */
    template<typename Type>
    constexpr decltype(auto) operator()(Type &&args) const noexcept(noexcept(std::apply(std::declval<const Func &>(), args))) {
        return std::apply(static_cast<const Func &>(*this), std::forward<Type>(args));
    }
};

/**
 * @brief Deduction guide.
 * @tparam Func Type of underlying invocable object.
 */
template<typename Func>
forward_apply(Func) -> forward_apply<std::remove_reference_t<std::remove_cv_t<Func>>>;

} // namespace entt

#endif
