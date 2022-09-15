#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "context.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"
#include "range.hpp"
#include "resolve.hpp"
#include "utility.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

inline internal::meta_base_node &meta_extend(internal::meta_type_node &owner, const id_type id, internal::meta_base_node node) {
    return owner.details->base.insert_or_assign(id, std::move(node)).first->second;
}

inline internal::meta_conv_node &meta_extend(internal::meta_type_node &owner, const id_type id, internal::meta_conv_node node) {
    return owner.details->conv.insert_or_assign(id, std::move(node)).first->second;
}

inline internal::meta_ctor_node &meta_extend(internal::meta_type_node &owner, const id_type id, internal::meta_ctor_node node) {
    return owner.details->ctor.insert_or_assign(id, std::move(node)).first->second;
}

inline internal::meta_dtor_node &meta_extend(internal::meta_type_node &owner, internal::meta_dtor_node node) {
    return (owner.dtor = std::move(node));
}

inline internal::meta_data_node &meta_extend(internal::meta_type_node &owner, const id_type id, internal::meta_data_node node) {
    return owner.details->data.insert_or_assign(id, std::move(node)).first->second;
}

inline internal::meta_func_node &meta_extend(internal::meta_type_node &owner, const id_type id, internal::meta_func_node node) {
    if(auto it = owner.details->func.find(id); it != owner.details->func.end()) {
        for(auto *curr = &it->second; curr; curr = curr->next.get()) {
            if(curr->invoke == node.invoke) {
                node.next = std::move(curr->next);
                *curr = std::move(node);
                return *curr;
            }
        }

        // locally overloaded function
        node.next = std::make_shared<internal::meta_func_node>(std::move(owner.details->func[id]));
    }

    return owner.details->func.insert_or_assign(id, std::move(node)).first->second;
}

template<typename Owner>
inline internal::meta_prop_node &meta_extend(Owner &owner, const id_type id, internal::meta_prop_node node) {
    if(!owner.details) {
        owner.details = std::make_shared<typename decltype(owner.details)::element_type>();
    }

    return (owner.details->prop[id] = std::move(node));
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Meta factory to be used for reflection purposes.
 *
 * The meta factory is an utility class used to reflect types, data members and
 * functions of all sorts. This class ensures that the underlying web of types
 * is built correctly and performs some checks in debug mode to ensure that
 * there are no subtle errors at runtime.
 */
template<typename...>
class meta_factory;

/**
 * @brief Extended meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 * @tparam Node Node for which to create properties, if any.
 */
template<typename Type, typename Node>
class meta_factory<Type, Node>: public meta_factory<Type> {
public:
    /**
     * @brief Constructs an extended factory from a given node.
     * @param target The node for which to store properties, if any.
     */
    meta_factory(Node &target) noexcept
        : parent{&target} {}

    /**
     * @brief Assigns a property to the last meta object created.
     *
     * Both the key and the value (if any) must be at least copy constructible.
     *
     * @tparam Value Optional type of the property value.
     * @param id Property key.
     * @param value Optional property value.
     * @return An extended meta factory for the given type.
     */
    template<typename... Value>
    meta_factory prop(id_type id, [[maybe_unused]] Value &&...value) {
        if constexpr(sizeof...(Value) == 0u) {
            internal::meta_extend(
                *parent,
                id,
                internal::meta_prop_node{
                    &internal::resolve<void>});
        } else {
            internal::meta_extend(
                *parent,
                id,
                internal::meta_prop_node{
                    &internal::resolve<std::decay_t<Value>>...,
                    std::make_shared<std::decay_t<Value>>(std::forward<Value>(value))...});
        }

        return *this;
    }

private:
    Node *parent;
};

/**
 * @brief Basic meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory<Type> {
    decltype(auto) owner() {
        return internal::meta_context::from(locator<meta_ctx>::value_or()).value[type_id<Type>().hash()];
    }

    template<typename Setter, auto Getter, typename Policy, std::size_t... Index>
    auto data(const id_type id, std::index_sequence<Index...>) noexcept {
        using data_type = std::invoke_result_t<decltype(Getter), Type &>;
        using args_type = type_list<typename meta_function_helper_t<Type, decltype(value_list_element_v<Index, Setter>)>::args_type...>;
        static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

        auto &&elem = internal::meta_extend(
            owner(),
            id,
            internal::meta_data_node{
                /* this is never static */
                (std::is_member_object_pointer_v<decltype(value_list_element_v<Index, Setter>)> && ... && std::is_const_v<std::remove_reference_t<data_type>>) ? internal::meta_traits::is_const : internal::meta_traits::is_none,
                Setter::size,
                &internal::resolve<std::remove_cv_t<std::remove_reference_t<data_type>>>,
                &meta_arg<type_list<type_list_element_t<type_list_element_t<Index, args_type>::size != 1u, type_list_element_t<Index, args_type>>...>>,
                +[](meta_handle instance, meta_any value) { return (meta_setter<Type, value_list_element_v<Index, Setter>>(*instance.operator->(), value.as_ref()) || ...); },
                &meta_getter<Type, Getter, Policy>});

        return meta_factory<Type, internal::meta_data_node>{elem};
    }

public:
    /*! @brief Default constructor. */
    meta_factory() noexcept {
        if(const auto it = internal::meta_context::from(locator<meta_ctx>::value_or()).value.try_emplace(type_id<Type>().hash(), internal::resolve<Type>()).first; !it->second.details) {
            it->second.details = std::make_shared<internal::meta_type_descriptor>();
        }
    }

    /**
     * @brief Assigns a custom unique identifier to a meta type.
     * @param id A custom unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto type(const id_type id) noexcept {
        auto &&elem = owner();
        ENTT_ASSERT(elem.id == id || !resolve(id), "Duplicate identifier");
        elem.id = id;
        return meta_factory<Type, internal::meta_type_node>{elem};
    }

    /**
     * @brief Assigns a meta base to a meta type.
     *
     * A reflected base class must be a real base class of the reflected type.
     *
     * @tparam Base Type of the base class to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename Base>
    auto base() noexcept {
        static_assert(!std::is_same_v<Type, Base> && std::is_base_of_v<Base, Type>, "Invalid base type");

        internal::meta_extend(
            owner(),
            type_id<Base>().hash(),
            internal::meta_base_node{
                &internal::resolve<Base>,
                +[](const void *instance) noexcept {
                    return static_cast<const void *>(static_cast<const Base *>(static_cast<const Type *>(instance)));
                }});

        return *this;
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * Conversion functions can be either free functions or member
     * functions.<br/>
     * In case of free functions, they must accept a const reference to an
     * instance of the parent type as an argument. In case of member functions,
     * they should have no arguments at all.
     *
     * @tparam Candidate The actual function to use for the conversion.
     * @return A meta factory for the parent type.
     */
    template<auto Candidate>
    auto conv() noexcept {
        using conv_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<decltype(Candidate), Type &>>>;

        internal::meta_extend(
            owner(),
            type_id<conv_type>().hash(),
            internal::meta_conv_node{
                +[](const void *instance) {
                    return forward_as_meta(std::invoke(Candidate, *static_cast<const Type *>(instance)));
                }});

        return *this;
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * The given type must be such that an instance of the reflected type can be
     * converted to it.
     *
     * @tparam To Type of the conversion function to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename To>
    auto conv() noexcept {
        using conv_type = std::remove_cv_t<std::remove_reference_t<To>>;

        internal::meta_extend(
            owner(),
            type_id<conv_type>().hash(),
            internal::meta_conv_node{
                +[](const void *instance) {
                    return forward_as_meta(static_cast<To>(*static_cast<const Type *>(instance)));
                }});

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Both member functions and free function can be assigned to meta types in
     * the role of constructors. All that is required is that they return an
     * instance of the underlying type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or not.
     *
     * @tparam Candidate The actual function to use as a constructor.
     * @tparam Policy Optional policy (no policy set by default).
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto ctor() noexcept {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");
        static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<typename descriptor::return_type>>, Type>, "The function doesn't return an object of the required type");

        internal::meta_extend(
            owner(),
            type_id<typename descriptor::args_type>().hash(),
            internal::meta_ctor_node{
                descriptor::args_type::size,
                &meta_arg<typename descriptor::args_type>,
                &meta_construct<Type, Candidate, Policy>});

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * A meta constructor is uniquely identified by the types of its arguments
     * and is such that there exists an actual constructor of the underlying
     * type that can be invoked with parameters whose types are those given.
     *
     * @tparam Args Types of arguments to use to construct an instance.
     * @return An extended meta factory for the parent type.
     */
    template<typename... Args>
    auto ctor() noexcept {
        using descriptor = meta_function_helper_t<Type, Type (*)(Args...)>;

        internal::meta_extend(
            owner(),
            type_id<typename descriptor::args_type>().hash(),
            internal::meta_ctor_node{
                descriptor::args_type::size,
                &meta_arg<typename descriptor::args_type>,
                &meta_construct<Type, Args...>});

        return *this;
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Both free functions and member functions can be assigned to meta types in
     * the role of destructors.<br/>
     * The signature of a free function should be identical to the following:
     *
     * @code{.cpp}
     * void(Type &);
     * @endcode
     *
     * Member functions should not take arguments instead.<br/>
     * The purpose is to give users the ability to free up resources that
     * require special treatment before an object is actually destroyed.
     *
     * @tparam Func The actual function to use as a destructor.
     * @return A meta factory for the parent type.
     */
    template<auto Func>
    auto dtor() noexcept {
        static_assert(std::is_invocable_v<decltype(Func), Type &>, "The function doesn't accept an object of the type provided");

        internal::meta_extend(
            owner(),
            internal::meta_dtor_node{
                +[](void *instance) { std::invoke(Func, *static_cast<Type *>(instance)); }});

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type.
     *
     * Both data members and static and global variables, as well as constants
     * of any kind, can be assigned to a meta type.<br/>
     * From a client's point of view, all the variables associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Data The actual variable to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_is_t>
    auto data(const id_type id) noexcept {
        if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_reference_t<std::invoke_result_t<decltype(Data), Type &>>;

            auto &&elem = internal::meta_extend(
                owner(),
                id,
                internal::meta_data_node{
                    /* this is never static */
                    std::is_const_v<data_type> ? internal::meta_traits::is_const : internal::meta_traits::is_none,
                    1u,
                    &internal::resolve<std::remove_const_t<data_type>>,
                    &meta_arg<type_list<std::remove_const_t<data_type>>>,
                    &meta_setter<Type, Data>,
                    &meta_getter<Type, Data, Policy>});

            return meta_factory<Type, internal::meta_data_node>{elem};
        } else {
            using data_type = std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>;

            auto &&elem = internal::meta_extend(
                owner(),
                id,
                internal::meta_data_node{
                    ((std::is_same_v<Type, std::remove_const_t<data_type>> || std::is_const_v<data_type>) ? internal::meta_traits::is_const : internal::meta_traits::is_none) | internal::meta_traits::is_static,
                    1u,
                    &internal::resolve<std::remove_const_t<data_type>>,
                    &meta_arg<type_list<std::remove_const_t<data_type>>>,
                    &meta_setter<Type, Data>,
                    &meta_getter<Type, Data, Policy>});

            return meta_factory<Type, internal::meta_data_node>{elem};
        }
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     *
     * Setters and getters can be either free functions, member functions or a
     * mix of them.<br/>
     * In case of free functions, setters and getters must accept a reference to
     * an instance of the parent type as their first argument. A setter has then
     * an extra argument of a type convertible to that of the parameter to
     * set.<br/>
     * In case of member functions, getters have no arguments at all, while
     * setters has an argument of a type convertible to that of the parameter to
     * set.
     *
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_is_t>
    auto data(const id_type id) noexcept {
        using data_type = std::invoke_result_t<decltype(Getter), Type &>;
        static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

        if constexpr(std::is_same_v<decltype(Setter), std::nullptr_t>) {
            auto &&elem = internal::meta_extend(
                owner(),
                id,
                internal::meta_data_node{
                    /* this is never static */
                    internal::meta_traits::is_const,
                    0u,
                    &internal::resolve<std::remove_cv_t<std::remove_reference_t<data_type>>>,
                    &meta_arg<type_list<>>,
                    &meta_setter<Type, Setter>,
                    &meta_getter<Type, Getter, Policy>});

            return meta_factory<Type, internal::meta_data_node>{elem};
        } else {
            using args_type = typename meta_function_helper_t<Type, decltype(Setter)>::args_type;

            auto &&elem = internal::meta_extend(
                owner(),
                id,
                internal::meta_data_node{
                    /* this is never static nor const */
                    internal::meta_traits::is_none,
                    1u,
                    &internal::resolve<std::remove_cv_t<std::remove_reference_t<data_type>>>,
                    &meta_arg<type_list<type_list_element_t<args_type::size != 1u, args_type>>>,
                    &meta_setter<Type, Setter>,
                    &meta_getter<Type, Getter, Policy>});

            return meta_factory<Type, internal::meta_data_node>{elem};
        }
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setters and
     * getter.
     *
     * Multi-setter support for meta data members. All setters are tried in the
     * order of definition before returning to the caller.<br/>
     * Setters can be either free functions, member functions or a mix of them
     * and are provided via a `value_list` type.
     *
     * @sa data
     *
     * @tparam Setter The actual functions to use as setters.
     * @tparam Getter The actual getter function.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<typename Setter, auto Getter, typename Policy = as_is_t>
    auto data(const id_type id) noexcept {
        return data<Setter, Getter, Policy>(id, std::make_index_sequence<Setter::size>{});
    }

    /**
     * @brief Assigns a meta function to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Candidate The actual function to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto func(const id_type id) noexcept {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");

        auto &&elem = internal::meta_extend(
            owner(),
            id,
            internal::meta_func_node{
                (descriptor::is_const ? internal::meta_traits::is_const : internal::meta_traits::is_none) | (descriptor::is_static ? internal::meta_traits::is_static : internal::meta_traits::is_none),
                descriptor::args_type::size,
                &internal::resolve<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, std::remove_cv_t<std::remove_reference_t<typename descriptor::return_type>>>>,
                &meta_arg<typename descriptor::args_type>,
                &meta_invoke<Type, Candidate, Policy>});

        return meta_factory<Type, internal::meta_func_node>{elem};
    }
};

/**
 * @brief Utility function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach meta objects through a
 * dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @return A meta factory for the given type.
 */
template<typename Type>
[[nodiscard]] auto meta() noexcept {
    auto res = internal::meta_context::from(locator<meta_ctx>::value_or()).value.try_emplace(type_id<Type>().hash(), internal::resolve<Type>());
    // extended meta factory to allow assigning properties to opaque meta types
    return meta_factory<Type, internal::meta_type_node>{res.first->second};
}

/**
 * @brief Resets a type and all its parts.
 *
 * Resets a type and all its data members, member functions and properties, as
 * well as its constructors, destructors and conversion functions if any.<br/>
 * Base classes aren't reset but the link between the two types is removed.
 *
 * The type is also removed from the set of searchable types.
 *
 * @param id Unique identifier.
 */
inline void meta_reset(const id_type id) noexcept {
    auto &&context = internal::meta_context::from(locator<meta_ctx>::value_or());

    for(auto it = context.value.begin(); it != context.value.end();) {
        if(it->second.id == id) {
            it = context.value.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief Resets a type and all its parts.
 *
 * @sa meta_reset
 *
 * @tparam Type Type to reset.
 */
template<typename Type>
void meta_reset() noexcept {
    internal::meta_context::from(locator<meta_ctx>::value_or()).value.erase(type_id<Type>().hash());
}

/**
 * @brief Resets all meta types.
 *
 * @sa meta_reset
 */
inline void meta_reset() noexcept {
    internal::meta_context::from(locator<meta_ctx>::value_or()).value.clear();
}

} // namespace entt

#endif
