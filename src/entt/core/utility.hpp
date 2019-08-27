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


}


#endif // ENTT_CORE_UTILITY_HPP
