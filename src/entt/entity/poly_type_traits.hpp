#ifndef ENTT_ENTITY_POLY_TYPE_TRAITS_HPP
#define ENTT_ENTITY_POLY_TYPE_TRAITS_HPP

#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"


namespace entt {

/**
 * @brief declares direct parent types of polymorphic component type.
 * By default it uses the list from T::direct_parent_types, if it present, otherwise empty list is used.
 * All parent types must be declared polymorphic.
 * @code{.cpp}
 * struct A {};
 * struct B : A {};
 *
 * struct entt::poly_direct_parent_types<A> {
 *     using parent_types = entt::type_list<>; // declares A as polymorphic type with no parents
 * }
 * struct entt::poly_direct_parent_types<B> {
 *     using parent_types = entt::type_list<A>; // declares B as polymorphic type with parent A
 * }
 * @endcode
 * @tparam T polymorphic component type
 */
template<typename T, typename = void>
struct poly_direct_parent_types {
    /** @brief entt::type_list of direct parent types */
    using parent_types = type_list<>;

    /** @brief used to detect, if this template was specialized for a type */
    using not_redefined_tag = void;
};

/** @copydoc poly_direct_parent_types */
template<typename T>
struct poly_direct_parent_types<T, std::void_t<typename T::direct_parent_types>> {
    using parent_types = typename T::direct_parent_types;
};

/**
 * @brief for given polymorphic component type returns entt::type_list of direct parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_direct_parent_types_t = typename poly_direct_parent_types<T>::parent_types;

/**
 * @brief declares list of all parent types of polymorphic component type.
 * By default will concatenates list of all direct parent types and all lists of all parents for each parent type. All parent
 * types must be declared polymorphic.
 * @tparam T
 */
template<typename T, typename = void>
struct poly_parent_types {
private:
    template<typename, typename...>
    struct all_parent_types;

    template<typename... DirectParents>
    struct all_parent_types<type_list<DirectParents...>> {
        using types = type_list_cat_t<type_list<DirectParents...>, typename poly_parent_types<DirectParents>::parent_types...>;
    };

public:
    /** @brief entt::type_list of all parent types */
    using parent_types = typename all_parent_types<poly_direct_parent_types_t<T>>::types;

    /** @brief used to detect, if this template was specialized for a type */
    using not_redefined_tag = void;
};

/** @copydoc poly_parent_types */
template<typename T>
struct poly_parent_types<T, std::void_t<typename T::all_parent_types>> {
    using parent_types = typename T::all_parent_types;
};

/**
 * @brief for given polymorphic component type returns entt::type_list of all parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_parent_types_t = typename poly_parent_types<T>::parent_types;


/**
 * @brief for given type, detects, if it was declared polymorphic. Type considered polymorphic,
 * if it was either:
 *  - inherited from entt::inherit
 *  - declared types direct_parent_types or all_parent_types
 *  - for this type there is specialization of either entt::poly_direct_parent_types or entt::poly_parent_types
 * @tparam T type to check
 */
template<typename T, typename = void>
struct is_poly_type {
    static constexpr bool value = true;
};

/** @copydoc is_poly_type */
template<typename T>
struct is_poly_type<T, std::void_t<typename poly_direct_parent_types<T>::not_redefined_tag, typename poly_parent_types<T>::not_redefined_tag>> {
    static constexpr bool value = false;
};

/** @copydoc is_poly_type */
template<typename T>
static constexpr bool is_poly_type_v = is_poly_type<T>::value;

/**
 * @brief used to inherit from all given parent types and declare inheriting type polymorphic with given direct parents.
 * All parent types are required to be polymorphic
 * @code{.cpp}
 * struct A : public entt::inherit<> {}; // base polymorphic type with no parents
 * struct B : public entt::inherit<A> {}; // B inherits A, and declared as polymorphic component with direct parents {A}
 * struct C : public entt::inherit<B> {}; // C inherits B, and now has direct parents {B} and all parents {A, B}
 * @endcode
 * @tparam Parents list of parent types
 */
template<typename... Parents>
struct inherit : public Parents... {
    static_assert((is_poly_type_v<Parents> && ...), "entt::inherit must be used only with polymorphic types");
    using direct_parent_types = type_list<Parents...>;
};

/**
 * @brief TODO
 */
template<typename T>
struct poly_all {
    static_assert(is_poly_type_v<T>, "entt::poly_all must be used only with polymorphic types");
    using type = T;
    using poly_all_tag = void;
};

/**
 * @brief TODO
 */
template<typename T>
struct poly_any {
    static_assert(is_poly_type_v<T>, "entt::poly_any must be used only with polymorphic types");
    using type = T;
    using poly_any_tag = void;
};

} // entt

#endif // ENTT_ENTITY_POLY_TYPE_TRAITS_HPP
