#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP


namespace entt {


/*! @brief A class to use to push around lists of types, nothing more. */
template<typename...>
struct type_list {};


/**
 * @brief Concatenates multiple type lists into one.
 * @tparam Type List of types.
 * @return The given type list.
 */
template<typename... Type>
constexpr auto type_list_cat(type_list<Type...> = type_list<>{}) {
    return type_list<Type...>{};
}


/**
 * @brief Concatenates multiple type lists into one.
 * @tparam Type List of types.
 * @tparam Other List of types.
 * @tparam List Type list instances.
 * @return A type list that is the concatenation of the given type lists.
 */
template<typename... Type, typename... Other, typename... List>
constexpr auto type_list_cat(type_list<Type...>, type_list<Other...>, List...) {
    return type_list_cat(type_list<Type..., Other...>{}, List{}...);
}


/**
 * @brief Variable template for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
constexpr type_list<Type...> exclude{};


}


#endif // ENTT_CORE_TYPE_TRAITS_HPP
