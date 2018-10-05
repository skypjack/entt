#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP


namespace entt {


/**
 * @brief Constant utility to disambiguate overloaded member functions.
 * @tparam Type Function type of the desired overload.
 * @tparam Class Type of class to which the member functions belong.
 * @param member A valid pointer to a member function.
 * @return Pointer to the member function.
 */
template<typename Type, typename Class>
constexpr auto overload(Type Class:: *member) { return member; }


/**
 * @brief Constant utility to disambiguate overloaded functions.
 * @tparam Type Function type of the desired overload.
 * @param func A valid pointer to a function.
 * @return Pointer to the function.
 */
template<typename Type>
constexpr auto overload(Type *func) { return func; }


}


#endif // ENTT_CORE_UTILITY_HPP
