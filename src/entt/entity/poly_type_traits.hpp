#ifndef ENTT_ENTITY_POLY_TYPE_TRAITS_HPP
#define ENTT_ENTITY_POLY_TYPE_TRAITS_HPP

#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"


namespace entt {

/** @brief Validates a polymorphic type and returns it unchanged */
template<typename T, typename... Parents>
struct poly_type_validate;

/**
 * @brief Declares direct parent types of polymorphic component type.
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
 * @brief For given polymorphic component type returns entt::type_list of direct parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_direct_parent_types_t = typename poly_direct_parent_types<T>::parent_types;

/**
 * @brief Declares list of all parent types of polymorphic component type.
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
 * @brief For given polymorphic component type returns entt::type_list of all parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_parent_types_t = typename poly_parent_types<T>::parent_types;

/**
 * @brief For a given type, detects, if it was declared polymorphic. Type considered polymorphic, if it was either:<br/>
 *  - inherited from entt::inherit<br/>
 *  - declared types direct_parent_types or all_parent_types<br/>
 *  - for this type there is specialization of either entt::poly_direct_parent_types or entt::poly_parent_types<br/>
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

/** @copydoc poly_type_validate */
template<typename T, typename... Parents>
struct poly_type_validate<T, type_list<Parents...>> {
    static_assert(std::is_same_v<std::decay_t<T>, T>, "only decayed types allowed to be declared as polymorphic");
    static_assert(is_poly_type_v<T>, "validating non-polymorphic type (probably some polymorphic type inherits type, that was not declared polymorphic)");
    static_assert((is_poly_type_v<Parents> && ...), "all parent types of a polymorphic type must be also polymorphic");
    static_assert((!std::is_pointer_v<std::remove_pointer_t<T>> && ... && !std::is_pointer_v<std::remove_pointer_t<Parents>>), "double pointers are not allowed as a polymorphic components");
    static_assert(((std::is_pointer_v<T> == std::is_pointer_v<Parents>) && ...), "you cannot mix pointer-based and value-based polymorphic components inside one hierarchy");

    /** @brief same input type, but validated */
    using type = T;
};

/** @copydoc poly_type_validate */
template<typename T>
using poly_type_validate_t = typename poly_type_validate<T, poly_parent_types_t<T>>::type;

/**
 * @brief Returns if one polymorphic component type is same, or is declared as a parent of another polymorphic type
 * @tparam Parent Parent type
 * @tparam Type Child Type
 */
template<typename Parent, typename Type>
constexpr bool is_poly_parent_of_v = is_poly_type_v<Type> && (type_list_contains_v<poly_parent_types_t<Type>, Parent> || std::is_same_v<Type, Parent>);

/**
 * @brief Used to inherit from all given parent types and declare inheriting type polymorphic with given direct parents.
 * All parent types are required to be polymorphic
 * @code{.cpp}
 * struct A : public entt::inherit<> {}; // base polymorphic type with no parents
 * struct B : public entt::inherit<A> {}; // B inherits A, and declared as polymorphic component with direct parents {A}
 * struct C : public entt::inherit<B> {}; // C inherits B, and now has direct parents {B} and all parents {A, B}
 * @endcode
 * @tparam Parents list of parent types
 */
template<typename... Parents>
struct inherit : public poly_type_validate_t<Parents>... {
    using direct_parent_types = type_list<Parents...>;
};

} // entt

#endif
