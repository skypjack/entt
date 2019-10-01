#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


#include "../config/config.h"


namespace entt {


/*! @brief Identity function object (waiting for C++20). */
struct identity {
    /**
     * @brief Returns its argument unchanged.
     * @tparam Type Type of the argument.
     * @param value The actual argument.
     * @return The submitted value as-is.
     */
    template<class Type>
    constexpr Type && operator()(Type &&value) const ENTT_NOEXCEPT {
        return std::forward<Type>(value);
    }
};


/**
 * @brief Constant utility to disambiguate overloaded member functions.
 * @tparam Type Function type of the desired overload.
 * @tparam Class Type of class to which the member functions belong.
 * @param member A valid pointer to a member function.
 * @return Pointer to the member function.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) ENTT_NOEXCEPT { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Type Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Type>
constexpr auto overload(Type *func) ENTT_NOEXCEPT { return func; }


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
template<class... Type>
overloaded(Type...) -> overloaded<Type...>;


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
    y_combinator(Func recursive):
        func{std::move(recursive)}
    {}

    /**
     * @brief Invokes a y-combinator and therefore its underlying function.
     * @tparam Args Types of arguments to use to invoke the underlying function.
     * @param args Parameters to use to invoke the underlying function.
     * @return Return value of the underlying function, if any.
     */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) const {
        return func(*this, std::forward<Args>(args)...);
    }

    /*! @copydoc operator()() */
    template <class... Args>
    decltype(auto) operator()(Args &&... args) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};


}


#endif // ENTT_CORE_UTILITY_HPP
