#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
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
class meta_factory;

/**
 * @brief Extended meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 * @tparam Spec Property specialization pack used to disambiguate overloads.
 */
template<typename Type, typename... Spec>
class meta_factory<Type, Spec...>: public meta_factory<Type> {
    void link_prop_if_required(internal::meta_prop_node &node) ENTT_NOEXCEPT {
        if(meta_range<internal::meta_prop_node *, internal::meta_prop_node> range{*ref}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [&node](const auto *curr) { return curr->id == node.id; }) == range.cend(), "Duplicate identifier");
            node.next = *ref;
            *ref = &node;
        }
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<2>, std::tuple<Property...> property, Other &&...other) ENTT_NOEXCEPT {
        std::apply([this](auto &&...curr) { (this->unroll<Step>(choice<2>, std::forward<Property>(curr)...)); }, property);
        unroll<Step + sizeof...(Property)>(choice<2>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename... Property, typename... Other>
    void unroll(choice_t<1>, std::pair<Property...> property, Other &&...other) ENTT_NOEXCEPT {
        assign<Step>(std::move(property.first), std::move(property.second));
        unroll<Step + 1>(choice<2>, std::forward<Other>(other)...);
    }

    template<std::size_t Step = 0, typename Property, typename... Other>
    void unroll(choice_t<0>, Property &&property, Other &&...other) ENTT_NOEXCEPT {
        assign<Step>(std::forward<Property>(property));
        unroll<Step + 1>(choice<2>, std::forward<Other>(other)...);
    }

    template<std::size_t>
    void unroll(choice_t<0>) ENTT_NOEXCEPT {}

    template<std::size_t = 0>
    void assign(meta_any key, meta_any value = {}) {
        static meta_any property[2u]{};

        static internal::meta_prop_node node{
            nullptr,
            property[0u],
            property[1u]
            // tricks clang-format
        };

        property[0u] = std::move(key);
        property[1u] = std::move(value);

        link_prop_if_required(node);
    }

public:
    /**
     * @brief Constructs an extended factory from a given node.
     * @param target The underlying node to which to assign the properties.
     */
    meta_factory(internal::meta_prop_node **target) ENTT_NOEXCEPT
        : ref{target} {}

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
    meta_factory<Type> prop(PropertyOrKey &&property_or_key, Value &&...value) {
        if constexpr(sizeof...(Value) == 0) {
            unroll(choice<2>, std::forward<PropertyOrKey>(property_or_key));
        } else {
            assign(std::forward<PropertyOrKey>(property_or_key), std::forward<Value>(value)...);
        }

        return {};
    }

    /**
     * @brief Assigns properties to the last meta object created.
     *
     * Both key and value (if any) must be at least copy constructible.
     *
     * @tparam Property Types of the properties.
     * @param property Properties to assign to the last meta object created.
     * @return A meta factory for the parent type.
     */
    template<typename... Property>
    meta_factory<Type> props(Property... property) {
        unroll(choice<2>, std::forward<Property>(property)...);
        return {};
    }

private:
    internal::meta_prop_node **ref;
};

/**
 * @brief Basic meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory<Type> {
    void link_base_if_required(internal::meta_base_node &node) ENTT_NOEXCEPT {
        if(meta_range<internal::meta_base_node *, internal::meta_base_node> range{owner->base}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = owner->base;
            owner->base = &node;
        }
    }

    void link_conv_if_required(internal::meta_conv_node &node) ENTT_NOEXCEPT {
        if(meta_range<internal::meta_conv_node *, internal::meta_conv_node> range{owner->conv}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = owner->conv;
            owner->conv = &node;
        }
    }

    void link_ctor_if_required(internal::meta_ctor_node &node) ENTT_NOEXCEPT {
        if(meta_range<internal::meta_ctor_node *, internal::meta_ctor_node> range{owner->ctor}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = owner->ctor;
            owner->ctor = &node;
        }
    }

    void link_data_if_required(const id_type id, internal::meta_data_node &node) ENTT_NOEXCEPT {
        meta_range<internal::meta_data_node *, internal::meta_data_node> range{owner->data};
        ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [id, &node](const auto *curr) { return curr != &node && curr->id == id; }) == range.cend(), "Duplicate identifier");
        node.id = id;

        if(std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = owner->data;
            owner->data = &node;
        }
    }

    void link_func_if_required(const id_type id, internal::meta_func_node &node) ENTT_NOEXCEPT {
        node.id = id;

        if(meta_range<internal::meta_func_node *, internal::meta_func_node> range{owner->func}; std::find(range.cbegin(), range.cend(), &node) == range.cend()) {
            node.next = owner->func;
            owner->func = &node;
        }
    }

    template<typename Setter, auto Getter, typename Policy, std::size_t... Index>
    auto data(const id_type id, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        using data_type = std::invoke_result_t<decltype(Getter), Type &>;
        using args_type = type_list<typename meta_function_helper_t<Type, decltype(value_list_element_v<Index, Setter>)>::args_type...>;
        static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

        static internal::meta_data_node node{
            {},
            /* this is never static */
            (std::is_member_object_pointer_v<decltype(value_list_element_v<Index, Setter>)> && ... && std::is_const_v<std::remove_reference_t<data_type>>) ? internal::meta_traits::is_const : internal::meta_traits::is_none,
            nullptr,
            nullptr,
            Setter::size,
            internal::meta_node<std::remove_cv_t<std::remove_reference_t<data_type>>>::resolve(),
            &meta_arg<type_list<type_list_element_t<type_list_element_t<Index, args_type>::size != 1u, type_list_element_t<Index, args_type>>...>>,
            [](meta_handle instance, meta_any value) -> bool { return (meta_setter<Type, value_list_element_v<Index, Setter>>(*instance.operator->(), value.as_ref()) || ...); },
            &meta_getter<Type, Getter, Policy>
            // tricks clang-format
        };

        link_data_if_required(id, node);
        return meta_factory<Type, Setter, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
    }

public:
    /*! @brief Default constructor. */
    meta_factory() ENTT_NOEXCEPT
        : owner{internal::meta_node<Type>::resolve()} {}

    /**
     * @brief Makes a meta type _searchable_.
     * @param id Optional unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto type(const id_type id = type_hash<Type>::value()) ENTT_NOEXCEPT {
        meta_range<internal::meta_type_node *, internal::meta_type_node> range{*internal::meta_context::global()};
        ENTT_ASSERT(std::find_if(range.cbegin(), range.cend(), [id, this](const auto *curr) { return curr != owner && curr->id == id; }) == range.cend(), "Duplicate identifier");
        owner->id = id;

        if(std::find(range.cbegin(), range.cend(), owner) == range.cend()) {
            owner->next = *internal::meta_context::global();
            *internal::meta_context::global() = owner;
        }

        return meta_factory<Type, Type>{&owner->prop};
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
        static_assert(!std::is_same_v<Type, Base> && std::is_base_of_v<Base, Type>, "Invalid base type");

        static internal::meta_base_node node{
            nullptr,
            internal::meta_node<Base>::resolve(),
            [](meta_any other) ENTT_NOEXCEPT -> meta_any {
                if(auto *ptr = other.data(); ptr) {
                    return forward_as_meta(*static_cast<Base *>(static_cast<Type *>(ptr)));
                }

                return forward_as_meta(*static_cast<const Base *>(static_cast<const Type *>(std::as_const(other).data())));
            }
            // tricks clang-format
        };

        link_base_if_required(node);
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
    auto conv() ENTT_NOEXCEPT {
        static internal::meta_conv_node node{
            nullptr,
            internal::meta_node<std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<decltype(Candidate), Type &>>>>::resolve(),
            [](const meta_any &instance) -> meta_any {
                return forward_as_meta(std::invoke(Candidate, *static_cast<const Type *>(instance.data())));
            }
            // tricks clang-format
        };

        link_conv_if_required(node);
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
        static internal::meta_conv_node node{
            nullptr,
            internal::meta_node<std::remove_cv_t<std::remove_reference_t<To>>>::resolve(),
            [](const meta_any &instance) -> meta_any { return forward_as_meta(static_cast<To>(*static_cast<const Type *>(instance.data()))); }
            // tricks clang-format
        };

        link_conv_if_required(node);
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
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");
        static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<typename descriptor::return_type>>, Type>, "The function doesn't return an object of the required type");

        static internal::meta_ctor_node node{
            nullptr,
            descriptor::args_type::size,
            &meta_arg<typename descriptor::args_type>,
            &meta_construct<Type, Candidate, Policy>
            // tricks clang-format
        };

        link_ctor_if_required(node);
        return meta_factory<Type>{};
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
        using descriptor = meta_function_helper_t<Type, Type (*)(Args...)>;

        static internal::meta_ctor_node node{
            nullptr,
            descriptor::args_type::size,
            &meta_arg<typename descriptor::args_type>,
            &meta_construct<Type, Args...>
            // tricks clang-format
        };

        link_ctor_if_required(node);
        return meta_factory<Type>{};
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
    auto dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type &>, "The function doesn't accept an object of the type provided");
        owner->dtor = [](void *instance) { std::invoke(Func, *static_cast<Type *>(instance)); };
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
            using data_type = std::remove_reference_t<std::invoke_result_t<decltype(Data), Type &>>;

            static internal::meta_data_node node{
                {},
                /* this is never static */
                std::is_const_v<data_type> ? internal::meta_traits::is_const : internal::meta_traits::is_none,
                nullptr,
                nullptr,
                1u,
                internal::meta_node<std::remove_const_t<data_type>>::resolve(),
                &meta_arg<type_list<std::remove_const_t<data_type>>>,
                &meta_setter<Type, Data>,
                &meta_getter<Type, Data, Policy>
                // tricks clang-format
            };

            link_data_if_required(id, node);
            return meta_factory<Type, std::integral_constant<decltype(Data), Data>, std::integral_constant<decltype(Data), Data>>{&node.prop};
        } else {
            using data_type = std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>;

            static internal::meta_data_node node{
                {},
                ((std::is_same_v<Type, std::remove_const_t<data_type>> || std::is_const_v<data_type>) ? internal::meta_traits::is_const : internal::meta_traits::is_none) | internal::meta_traits::is_static,
                nullptr,
                nullptr,
                1u,
                internal::meta_node<std::remove_const_t<data_type>>::resolve(),
                &meta_arg<type_list<std::remove_const_t<data_type>>>,
                &meta_setter<Type, Data>,
                &meta_getter<Type, Data, Policy>
                // tricks clang-format
            };

            link_data_if_required(id, node);
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
        using data_type = std::invoke_result_t<decltype(Getter), Type &>;
        static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

        if constexpr(std::is_same_v<decltype(Setter), std::nullptr_t>) {
            static internal::meta_data_node node{
                {},
                /* this is never static */
                internal::meta_traits::is_const,
                nullptr,
                nullptr,
                0u,
                internal::meta_node<std::remove_cv_t<std::remove_reference_t<data_type>>>::resolve(),
                &meta_arg<type_list<>>,
                &meta_setter<Type, Setter>,
                &meta_getter<Type, Getter, Policy>
                // tricks clang-format
            };

            link_data_if_required(id, node);
            return meta_factory<Type, std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
        } else {
            using args_type = typename meta_function_helper_t<Type, decltype(Setter)>::args_type;

            static internal::meta_data_node node{
                {},
                /* this is never static nor const */
                internal::meta_traits::is_none,
                nullptr,
                nullptr,
                1u,
                internal::meta_node<std::remove_cv_t<std::remove_reference_t<data_type>>>::resolve(),
                &meta_arg<type_list<type_list_element_t<args_type::size != 1u, args_type>>>,
                &meta_setter<Type, Setter>,
                &meta_getter<Type, Getter, Policy>
                // tricks clang-format
            };

            link_data_if_required(id, node);
            return meta_factory<Type, std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
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
    auto data(const id_type id) ENTT_NOEXCEPT {
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
    auto func(const id_type id) ENTT_NOEXCEPT {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");

        static internal::meta_func_node node{
            {},
            (descriptor::is_const ? internal::meta_traits::is_const : internal::meta_traits::is_none) | (descriptor::is_static ? internal::meta_traits::is_static : internal::meta_traits::is_none),
            nullptr,
            nullptr,
            descriptor::args_type::size,
            internal::meta_node<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, std::remove_cv_t<std::remove_reference_t<typename descriptor::return_type>>>>::resolve(),
            &meta_arg<typename descriptor::args_type>,
            &meta_invoke<Type, Candidate, Policy>
            // tricks clang-format
        };

        link_func_if_required(id, node);
        return meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
    }

private:
    internal::meta_type_node *owner;
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
    auto *const node = internal::meta_node<Type>::resolve();
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
                for(auto **sub = (&((*curr)->*member), ...); *sub; *sub = std::exchange((*sub)->next, nullptr)) {}
            }
        }
    };

    for(auto **it = internal::meta_context::global(); *it; it = &(*it)->next) {
        if(auto *node = *it; node->id == id) {
            clear_chain(&node->prop);
            clear_chain(&node->base);
            clear_chain(&node->conv);
            clear_chain(&node->ctor);
            clear_chain(&node->data, &internal::meta_data_node::prop);
            clear_chain(&node->func, &internal::meta_func_node::prop);

            node->id = {};
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
    meta_reset(internal::meta_node<Type>::resolve()->id);
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

} // namespace entt

#endif
