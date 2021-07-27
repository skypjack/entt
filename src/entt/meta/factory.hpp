#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"
#include "range.hpp"
#include "utility.hpp"


namespace entt {


/**
 * @brief Meta factory to be used for reflection purposes.
 *
 * The meta factory is an utility class used to reflect types, data members and
 * functions of all sorts. This class ensures that the underlying web of types
 * is built correctly and performs some checks in debug mode to ensure that
 * there are no subtle errors at runtime.
 */
template<typename...>
struct meta_factory;


/**
 * @brief Extended meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 * @tparam Spec Property specialization pack used to disambiguate overloads.
 */
template<typename Type, typename... Spec>
struct meta_factory<Type, Spec...>: public meta_factory<Type> {
private:
    template<std::size_t Step = 0, std::size_t... Index, typename... Property, typename... Other>
    void unpack(std::index_sequence<Index...>, std::tuple<Property...> property, Other &&... other) {
        unroll<Step>(choice<3>, std::move(std::get<Index>(property))..., std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<3>, std::tuple<Property...> property, Other &&... other) {
        unpack<Step>(std::index_sequence_for<Property...>{}, std::move(property), std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<2>, std::pair<Property...> property, Other &&... other) {
        assign<Step>(std::move(property.first), std::move(property.second));
        unroll<Step+1>(choice<3>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename Property, typename... Other>
    std::enable_if_t<!std::is_invocable_v<Property>>
    unroll(choice_t<1>, Property &&property, Other &&... other) {
        assign<Step>(std::forward<Property>(property));
        unroll<Step+1>(choice<3>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename Func, typename... Other>
    void unroll(choice_t<0>, Func &&invocable, Other &&... other) {
        unroll<Step>(choice<3>, std::forward<Func>(invocable)(), std::forward<Other>(other)...);
    }

    template<std::size_t>
    void unroll(choice_t<0>) {}

    template<std::size_t = 0, typename Key>
    void assign(Key &&key, meta_any value = {}) {
        static meta_any property[2u]{};

        static internal::meta_prop_node node{
            nullptr,
            property[0u],
            property[1u]
        };

        entt::meta_any instance{std::forward<Key>(key)};
        property[0u] = std::move(instance);
        property[1u] = std::move(value);

        if(meta_range<internal::meta_prop_node *, internal::meta_prop_node> range{*ref}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [&instance](const auto *curr) { return curr->id == instance; }) == range.cend(), "Duplicate identifier");
            node.next = *ref;
            *ref = &node;
        }
    }

public:
    /**
     * @brief Constructs an extended factory from a given node.
     * @param target The underlying node to which to assign the properties.
     */
    meta_factory(internal::meta_prop_node **target) ENTT_NOEXCEPT
        : ref{target}
    {}

    /**
     * @brief Assigns a property to the last meta object created.
     *
     * Both the key and the value (if any) must be at least copy constructible.
     *
     * @tparam PropertyOrKey Type of the property or property key.
     * @tparam Value Optional type of the property value.
     * @param property_or_key Property or property key.
     * @param value Optional property value.
     * @return A meta factory for the parent type.
     */
    template<typename PropertyOrKey, typename... Value>
    auto prop(PropertyOrKey &&property_or_key, Value &&... value) && {
        if constexpr(sizeof...(Value) == 0) {
            unroll(choice<3>, std::forward<PropertyOrKey>(property_or_key));
        } else {
            assign(std::forward<PropertyOrKey>(property_or_key), std::forward<Value>(value)...);
        }

        return meta_factory<Type, Spec..., PropertyOrKey, Value...>{ref};
    }

    /**
     * @brief Assigns properties to the last meta object created.
     *
     * Both the keys and the values (if any) must be at least copy
     * constructible.
     *
     * @tparam Property Types of the properties.
     * @param property Properties to assign to the last meta object created.
     * @return A meta factory for the parent type.
     */
    template <typename... Property>
    auto props(Property... property) && {
        unroll(choice<3>, std::forward<Property>(property)...);
        return meta_factory<Type, Spec..., Property...>{ref};
    }

private:
    internal::meta_prop_node **ref;
};


/**
 * @brief Basic meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
struct meta_factory<Type> {
    /**
     * @brief Makes a meta type _searchable_.
     * @param id Optional unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto type(const id_type id = type_hash<Type>::value()) {
        auto * const node = internal::meta_info<Type>::resolve();

        node->id = id;

        if(meta_range<internal::meta_type_node *, internal::meta_type_node> range{*internal::meta_context::global()}; std::find(range.cbegin(), range.cend(), node) == range.cend()) {
            ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [id](const auto *curr) { return curr->id == id; }) == range.cend(), "Duplicate identifier");
            node->next = *internal::meta_context::global();
            *internal::meta_context::global() = node;
        }

        return meta_factory<Type, Type>{&node->prop};
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
    auto base() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<Base, Type>, "Invalid base type");
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            nullptr,
            internal::meta_info<Base>::resolve(),
            [](const void *instance) ENTT_NOEXCEPT -> const void * {
                return static_cast<const Base *>(static_cast<const Type *>(instance));
            }
        };

        if(meta_range<internal::meta_base_node *, internal::meta_base_node> range{type->base}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->base;
            type->base = &node;
        }

        return meta_factory<Type>{};
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
    std::enable_if_t<std::is_member_function_pointer_v<decltype(Candidate)>, meta_factory<Type>> conv() ENTT_NOEXCEPT {
        using conv_type = std::invoke_result_t<decltype(Candidate), Type &>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            nullptr,
            internal::meta_info<conv_type>::resolve(),
            [](const void *instance) -> meta_any {
                return (static_cast<const Type *>(instance)->*Candidate)();
            }
        };

        if(meta_range<internal::meta_conv_node *, internal::meta_conv_node> range{type->conv}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->conv;
            type->conv = &node;
        }

        return meta_factory<Type>{};
    }

    /*! @copydoc conv */
    template<auto Candidate>
    std::enable_if_t<!std::is_member_function_pointer_v<decltype(Candidate)>, meta_factory<Type>> conv() ENTT_NOEXCEPT {
        using conv_type = std::invoke_result_t<decltype(Candidate), Type &>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            nullptr,
            internal::meta_info<conv_type>::resolve(),
            [](const void *instance) -> meta_any {
                return Candidate(*static_cast<const Type *>(instance));
            }
        };

        if(meta_range<internal::meta_conv_node *, internal::meta_conv_node> range{type->conv}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->conv;
            type->conv = &node;
        }

        return meta_factory<Type>{};
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
    auto conv() ENTT_NOEXCEPT {
        static_assert(std::is_convertible_v<Type, To>, "Could not convert to the required type");
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            nullptr,
            internal::meta_info<To>::resolve(),
            [](const void *instance) -> meta_any {
                return static_cast<To>(*static_cast<const Type *>(instance));
            }
        };

        if(meta_range<internal::meta_conv_node *, internal::meta_conv_node> range{type->conv}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->conv;
            type->conv = &node;
        }

        return meta_factory<Type>{};
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
    auto ctor() ENTT_NOEXCEPT {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(std::is_same_v<std::decay_t<typename descriptor::return_type>, Type>, "The function doesn't return an object of the required type");
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            nullptr,
            nullptr,
            descriptor::args_type::size,
            &meta_arg<typename descriptor::args_type>,
            &meta_construct<Type, Candidate, Policy>
        };

        if(meta_range<internal::meta_ctor_node *, internal::meta_ctor_node> range{type->ctor}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->ctor;
            type->ctor = &node;
        }

        return meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
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
    auto ctor() ENTT_NOEXCEPT {
        using descriptor = meta_function_helper_t<Type, Type(*)(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            nullptr,
            nullptr,
            descriptor::args_type::size,
            &meta_arg<typename descriptor::args_type>,
            &meta_construct<Type, Args...>
        };

        if(meta_range<internal::meta_ctor_node *, internal::meta_ctor_node> range{type->ctor}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = type->ctor;
            type->ctor = &node;
        }

        return meta_factory<Type, Type(Args...)>{&node.prop};
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of destructors.
     * The signature of the function should identical to the following:
     *
     * @code{.cpp}
     * void(Type &);
     * @endcode
     *
     * The purpose is to give users the ability to free up resources that
     * require special treatment before an object is actually destroyed.
     *
     * @tparam Func The actual function to use as a destructor.
     * @return A meta factory for the parent type.
     */
    template<auto Func>
    auto dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type &>, "The function doesn't accept an object of the type provided");
        auto * const type = internal::meta_info<Type>::resolve();

        type->dtor = [](void *instance) {
            Func(*static_cast<Type *>(instance));
        };

        return meta_factory<Type>{};
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
    auto data(const id_type id) ENTT_NOEXCEPT {
        if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            return data<Data, Data, Policy>(id);
        } else {
            using data_type = std::remove_pointer_t<decltype(Data)>;
            auto * const type = internal::meta_info<Type>::resolve();

            static internal::meta_data_node node{
                {},
                nullptr,
                nullptr,
                internal::meta_trait::IS_NONE
                    | ((std::is_same_v<Type, data_type> || std::is_const_v<data_type>) ? internal::meta_trait::IS_CONST : internal::meta_trait::IS_NONE)
                    | internal::meta_trait::IS_STATIC,
                internal::meta_info<data_type>::resolve(),
                &meta_setter<Type, Data>,
                &meta_getter<Type, Data, Policy>
            };

            node.id = id;

            if(meta_range<internal::meta_data_node *, internal::meta_data_node> range{type->data}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
                ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [id](const auto *curr) { return curr->id == id; }) == range.cend(), "Duplicate identifier");
                node.next = type->data;
                type->data = &node;
            }

            return meta_factory<Type, std::integral_constant<decltype(Data), Data>>{&node.prop};
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
    auto data(const id_type id) ENTT_NOEXCEPT {
        using underlying_type = std::remove_reference_t<std::invoke_result_t<decltype(Getter), Type &>>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_data_node node{
            {},
            nullptr,
            nullptr,
            internal::meta_trait::IS_NONE
                | ((std::is_same_v<decltype(Setter), std::nullptr_t> || (std::is_member_object_pointer_v<decltype(Setter)> && std::is_const_v<underlying_type>)) ? internal::meta_trait::IS_CONST : internal::meta_trait::IS_NONE)
                /* this is never static */,
            internal::meta_info<underlying_type>::resolve(),
            &meta_setter<Type, Setter>,
            &meta_getter<Type, Getter, Policy>
        };

        node.id = id;

        if(meta_range<internal::meta_data_node *, internal::meta_data_node> range{type->data}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [id](const auto *curr) { return curr->id == id; }) == range.cend(), "Duplicate identifier");
            node.next = type->data;
            type->data = &node;
        }

        return meta_factory<Type, std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
    }

    /**
     * @brief Assigns a meta funcion to a meta type.
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
    auto func(const id_type id) ENTT_NOEXCEPT {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            {},
            nullptr,
            nullptr,
            descriptor::args_type::size,
            internal::meta_trait::IS_NONE
                | (descriptor::is_const ? internal::meta_trait::IS_CONST : internal::meta_trait::IS_NONE)
                | (descriptor::is_static ? internal::meta_trait::IS_STATIC : internal::meta_trait::IS_NONE),
            internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename descriptor::return_type>>::resolve(),
            &meta_arg<typename descriptor::args_type>,
            &meta_invoke<Type, Candidate, Policy>
        };

        for(auto *it = &type->func; *it; it = &(*it)->next) {
            if(*it == &node) {
                *it = node.next;
                break;
            }
        }

        internal::meta_func_node **it = &type->func;
        for(; *it && (*it)->id != id; it = &(*it)->next);
        for(; *it && (*it)->id == id && (*it)->arity < node.arity; it = &(*it)->next);

        node.id = id;
        node.next = *it;
        *it = &node;

        return meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
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
[[nodiscard]] auto meta() ENTT_NOEXCEPT {
    auto * const node = internal::meta_info<Type>::resolve();
    // extended meta factory to allow assigning properties to opaque meta types
    return meta_factory<Type, Type>{&node->prop};
}


/**
 * @brief Resets a type and all its parts.
 *
 * Resets a type and all its data members, member functions and properties, as
 * well as its constructors, destructors and conversion functions if any.<br/>
 * Base classes aren't reset but the link between the two types is removed.
 *
 * The type is also removed from the list of searchable types.
 *
 * @param id Unique identifier.
 */
inline void meta_reset(const id_type id) ENTT_NOEXCEPT {
    auto clear_chain = [](auto **curr, auto... member) {
        for(; *curr; *curr = std::exchange((*curr)->next, nullptr)) {
            if constexpr(sizeof...(member) != 0u) {
                static_assert(sizeof...(member) == 1u, "Assert in defense of the future me");
                for(auto **it = (&((*curr)->*member), ...); *it; *it = std::exchange((*it)->next, nullptr));
            }
        }
    };

    for(auto** it = internal::meta_context::global(); *it; it = &(*it)->next) {
        if(auto *node = *it; node->id == id) {
            clear_chain(&node->prop);
            clear_chain(&node->base);
            clear_chain(&node->conv);
            clear_chain(&node->ctor, &internal::meta_ctor_node::prop);
            clear_chain(&node->data, &internal::meta_data_node::prop);
            clear_chain(&node->func, &internal::meta_func_node::prop);

            node->id = {};
            node->ctor = node->def_ctor;
            node->dtor = nullptr;
            *it = std::exchange(node->next, nullptr);

            break;
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
void meta_reset() ENTT_NOEXCEPT {
    meta_reset(internal::meta_info<Type>::resolve()->id);
}


/**
 * @brief Resets all searchable types.
 *
 * @sa meta_reset
 */
inline void meta_reset() ENTT_NOEXCEPT {
    while(*internal::meta_context::global()) {
        meta_reset((*internal::meta_context::global())->id);
    }
}


}


#endif
