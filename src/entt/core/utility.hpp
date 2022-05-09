#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP

#include <type_traits>
#include <utility>

namespace entt {

/*! @brief Identity function object (waiting for C++20). */
struct identity {
    /*! @brief Indicates that this is a transparent function object. */
    using is_transparent = void;

    /**
     * @brief Returns its argument unchanged.
     * @tparam Type Type of the argument.
     * @param value The actual argument.
     * @return The submitted value as-is.
     */
    template<class Type>
    [[nodiscard]] constexpr Type &&operator()(Type &&value) const noexcept {
        return std::forward<Type>(value);
    }
};

/**
 * @brief Constant utility to disambiguate overloaded members of a class.
 * @tparam Type Type of the desired overload.
 * @tparam Class Type of class to which the member belongs.
 * @param member A valid pointer to a member.
 * @return Pointer to the member.
 */
template<typename Type, typename Class>
[[nodiscard]] constexpr auto overload(Type Class::*member) noexcept {
    return member;
}

/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Func Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Func>
[[nodiscard]] constexpr auto overload(Func *func) noexcept {
    return func;
}

/**
 * @brief Helper type for visitors.
 * @tparam Func Types of function objects.
 */
template<class... Func>
struct overloaded: Func... {
    using Func::operator()...;
};

/**
 * @brief Deduction guide.
 * @tparam Func Types of function objects.
 */
template<class... Func>
overloaded(Func...) -> overloaded<Func...>;

/**
 * @brief Basic implementation of a y-combinator.
 * @tparam Func Type of a potentially recursive function.
 */
template<class Func>
struct y_combinator {
    /**
     * @brief Constructs a y-combinator from a given function.
     * @param recursive A potentially recursive function.
     */
    constexpr y_combinator(Func recursive) noexcept(std::is_nothrow_move_constructible_v<Func>)
        : func{std::move(recursive)} {}

    /**
     * @brief Invokes a y-combinator and therefore its underlying function.
     * @tparam Args Types of arguments to use to invoke the underlying function.
     * @param args Parameters to use to invoke the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template<class... Args>
    constexpr decltype(auto) operator()(Args &&...args) const noexcept(std::is_nothrow_invocable_v<Func, const y_combinator &, Args...>) {
        return func(*this, std::forward<Args>(args)...);
    }

    /*! @copydoc operator()() */
    template<class... Args>
    constexpr decltype(auto) operator()(Args &&...args) noexcept(std::is_nothrow_invocable_v<Func, y_combinator &, Args...>) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};

} // namespace entt

#endif
