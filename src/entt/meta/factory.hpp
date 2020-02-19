#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <tuple>
#include <array>
#include <cstddef>
#include <utility>
#include <functional>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "policy.hpp"
#include "meta.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename>
struct meta_function_helper;


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...)> {
    using return_type = std::remove_cv_t<std::remove_reference_t<Ret>>;
    using args_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

    static constexpr std::index_sequence_for<Args...> index_sequence{};
    static constexpr auto is_const = false;

    static auto arg(typename internal::meta_func_node::size_type index) ENTT_NOEXCEPT {
        return std::array<meta_type_node *, sizeof...(Args)>{{meta_info<Args>::resolve()...}}[index];
    }
};


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...) const>: meta_function_helper<Ret(Args...)> {
    static constexpr auto is_const = true;
};


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(Class:: *)(Args...));


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...) const>
to_meta_function_helper(Ret(Class:: *)(Args...) const);


template<typename Ret, typename... Args>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(*)(Args...));


constexpr void to_meta_function_helper(...);


template<typename Candidate>
using meta_function_helper_t = decltype(to_meta_function_helper(std::declval<Candidate>()));


template<typename Type, typename... Args, std::size_t... Indexes>
meta_any construct(meta_any * const args, std::index_sequence<Indexes...>) {
    [[maybe_unused]] auto direct = std::make_tuple((args+Indexes)->try_cast<Args>()...);
    meta_any any{};

    if(((std::get<Indexes>(direct) || (args+Indexes)->convert<Args>()) && ...)) {
        any = Type{(std::get<Indexes>(direct) ? *std::get<Indexes>(direct) : (args+Indexes)->cast<Args>())...};
    }

    return any;
}


template<bool Const, typename Type, auto Data>
bool setter([[maybe_unused]] meta_any instance, [[maybe_unused]] meta_any index, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(!Const) {
        if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using helper_type = meta_function_helper_t<decltype(Data)>;
            using data_type = std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>;
            static_assert(std::is_invocable_v<decltype(Data), Type &, data_type>);
            auto * const clazz = instance.try_cast<Type>();
            auto * const direct = value.try_cast<data_type>();

            if(clazz && (direct || value.convert<data_type>())) {
                std::invoke(Data, *clazz, direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
            static_assert(std::is_invocable_v<decltype(Data), Type *>);
            auto * const clazz = instance.try_cast<Type>();

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                auto * const direct = value.try_cast<underlying_type>();
                auto * const idx = index.try_cast<std::size_t>();

                if(clazz && idx && (direct || value.convert<underlying_type>())) {
                    std::invoke(Data, clazz)[*idx] = direct ? *direct : value.cast<underlying_type>();
                    accepted = true;
                }
            } else {
                auto * const direct = value.try_cast<data_type>();

                if(clazz && (direct || value.convert<data_type>())) {
                    std::invoke(Data, clazz) = (direct ? *direct : value.cast<data_type>());
                    accepted = true;
                }
            }
        } else {
            static_assert(std::is_pointer_v<decltype(Data)>);
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(*Data)>>;

            if constexpr(std::is_array_v<data_type>) {
                using underlying_type = std::remove_extent_t<data_type>;
                auto * const direct = value.try_cast<underlying_type>();
                auto * const idx = index.try_cast<std::size_t>();

                if(idx && (direct || value.convert<underlying_type>())) {
                    (*Data)[*idx] = (direct ? *direct : value.cast<underlying_type>());
                    accepted = true;
                }
            } else {
                auto * const direct = value.try_cast<data_type>();

                if(direct || value.convert<data_type>()) {
                    *Data = (direct ? *direct : value.cast<data_type>());
                    accepted = true;
                }
            }
        }
    }

    return accepted;
}


template<typename Type, auto Data, typename Policy>
meta_any getter([[maybe_unused]] meta_any instance, [[maybe_unused]] meta_any index) {
    auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>, std::forward<decltype(value)>(value)};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::forward<decltype(value)>(value))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
        static_assert(std::is_invocable_v<decltype(Data), Type &>);
        auto * const clazz = instance.try_cast<Type>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
        static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto * const clazz = instance.try_cast<Type>();

        if constexpr(std::is_array_v<data_type>) {
            auto * const idx = index.try_cast<std::size_t>();
            return (clazz && idx) ? dispatch(std::invoke(Data, clazz)[*idx]) : meta_any{};
        } else {
            return clazz ? dispatch(std::invoke(Data, clazz)) : meta_any{};
        }
    } else {
        static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);

        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            auto * const idx = index.try_cast<std::size_t>();
            return idx ? dispatch((*Data)[*idx]) : meta_any{};
        } else {
            return dispatch(*Data);
        }
    }
}


template<typename Type, auto Candidate, typename Policy, std::size_t... Indexes>
meta_any invoke([[maybe_unused]] meta_any instance, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper_t<decltype(Candidate)>;

    auto dispatch = [](auto *... params) {
        if constexpr(std::is_void_v<typename helper_type::return_type> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, *params...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::invoke(Candidate, *params...))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::invoke(Candidate, *params...)};
        }
    };

    [[maybe_unused]] const auto direct = std::make_tuple([](meta_any *any, auto *value) {
        using arg_type = std::remove_reference_t<decltype(*value)>;

        if(!value && any->convert<arg_type>()) {
            value = any->try_cast<arg_type>();
        }

        return value;
    }(args+Indexes, (args+Indexes)->try_cast<std::tuple_element_t<Indexes, typename helper_type::args_type>>())...);

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Candidate)>>>) {
        return (std::get<Indexes>(direct) && ...) ? dispatch(std::get<Indexes>(direct)...) : meta_any{};
    } else {
        auto * const clazz = instance.try_cast<Type>();
        return (clazz && (std::get<Indexes>(direct) && ...)) ? dispatch(clazz, std::get<Indexes>(direct)...) : meta_any{};
    }
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
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
 * @tparam Spec Property specialization pack used to disambiguate overloads.
 */
template<typename Type, typename... Spec>
class meta_factory<Type, Spec...>: public meta_factory<Type> {
    bool exists(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node && (node->key() == key || exists(key, node->next));
    }

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

    template<std::size_t = 0, typename Key, typename... Value>
    void assign(Key &&key, Value &&... value) {
        static const auto property{std::make_tuple(std::forward<Key>(key), std::forward<Value>(value)...)};

        static internal::meta_prop_node node{
            nullptr,
            []() -> meta_any {
                return std::get<0>(property);
            },
            []() -> meta_any {
                if constexpr(sizeof...(Value) == 0) {
                    return {};
                } else {
                    return std::get<1>(property);
                }
            }
        };

        ENTT_ASSERT(!exists(node.key(), *curr));
        node.next = *curr;
        *curr = &node;
    }

public:
    /**
     * @brief Constructs an extended factory from a given node.
     * @param target The underlying node to which to assign the properties.
     */
    meta_factory(entt::internal::meta_prop_node **target) ENTT_NOEXCEPT
        : curr{target}
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

        return meta_factory<Type, Spec..., PropertyOrKey, Value...>{curr};
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
        return meta_factory<Type, Spec..., Property...>{curr};
    }

private:
    entt::internal::meta_prop_node **curr;
};


/**
 * @brief Basic meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory<Type> {
    template<typename Node>
    bool exists(const Node *candidate, const Node *node) ENTT_NOEXCEPT {
        return node && (node == candidate || exists(candidate, node->next));
    }

    template<typename Node>
    bool exists(const ENTT_ID_TYPE alias, const Node *node) ENTT_NOEXCEPT {
        return node && (node->alias == alias || exists(alias, node->next));
    }

public:
    /**
     * @brief Extends a meta type by assigning it an alias.
     * @param value Unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto alias(const ENTT_ID_TYPE value) ENTT_NOEXCEPT {
        auto * const node = internal::meta_info<Type>::resolve();

        ENTT_ASSERT(!exists(value, *internal::meta_info<>::global));
        ENTT_ASSERT(!exists(node, *internal::meta_info<>::global));
        node->alias = value;
        node->next = *internal::meta_info<>::global;
        *internal::meta_info<>::global = node;

        return meta_factory<Type, Type>{&node->prop};
    }

    /*! @copydoc alias */
    [[deprecated("Use ::alias instead")]]
    auto type(const ENTT_ID_TYPE value) ENTT_NOEXCEPT {
        return alias(value);
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
        static_assert(std::is_base_of_v<Base, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](void *instance) ENTT_NOEXCEPT -> void * {
                return static_cast<Base *>(static_cast<Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->base));
        node.next = type->base;
        type->base = &node;

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
        static_assert(std::is_convertible_v<Type, To>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            type,
            nullptr,
            &internal::meta_info<To>::resolve,
            [](const void *instance) -> meta_any {
                return static_cast<To>(*static_cast<const Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->conv));
        node.next = type->conv;
        type->conv = &node;

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
        using conv_type = std::invoke_result_t<decltype(Candidate), Type &>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            type,
            nullptr,
            &internal::meta_info<conv_type>::resolve,
            [](const void *instance) -> meta_any {
                return std::invoke(Candidate, *static_cast<const Type *>(instance));
            }
        };

        ENTT_ASSERT(!exists(&node, type->conv));
        node.next = type->conv;
        type->conv = &node;

        return meta_factory<Type>{};
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of constructors.
     * All that is required is that they return an instance of the underlying
     * type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or a free function.
     *
     * @tparam Func The actual function to use as a constructor.
     * @tparam Policy Optional policy (no policy set by default).
     * @return An extended meta factory for the parent type.
     */
    template<auto Func, typename Policy = as_is_t>
    auto ctor() ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Func)>;
        static_assert(std::is_same_v<typename helper_type::return_type, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func, Policy>({}, any, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

        return meta_factory<Type, std::integral_constant<decltype(Func), Func>>{&node.prop};
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
        using helper_type = internal::meta_function_helper_t<Type(*)(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, std::remove_cv_t<std::remove_reference_t<Args>>...>(any, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

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
        static_assert(std::is_invocable_v<decltype(Func), Type &>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_dtor_node node{
            type,
            [](void *instance) {
                if(instance) {
                    std::invoke(Func, *static_cast<Type *>(instance));
                }
            }
        };

        ENTT_ASSERT(!type->dtor);
        type->dtor = &node;

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
     * @param alias Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_is_t>
    auto data(const ENTT_ID_TYPE alias) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::resolve();
        internal::meta_data_node *curr = nullptr;

        if constexpr(std::is_same_v<Type, decltype(Data)>) {
            static_assert(std::is_same_v<Policy, as_is_t>);

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                true,
                true,
                &internal::meta_info<Type>::resolve,
                [](meta_any, meta_any, meta_any) { return false; },
                [](meta_any, meta_any) -> meta_any { return Data; }
            };

            curr = &node;
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_reference_t<decltype(std::declval<Type>().*Data)>;

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data, Policy>
            };

            curr = &node;
        } else {
            static_assert(std::is_pointer_v<std::decay_t<decltype(Data)>>);
            using data_type = std::remove_pointer_t<std::decay_t<decltype(Data)>>;

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type>::resolve,
                &internal::setter<std::is_const_v<data_type>, Type, Data>,
                &internal::getter<Type, Data, Policy>
            };

            curr = &node;
        }

        ENTT_ASSERT(!exists(alias, type->data));
        ENTT_ASSERT(!exists(curr, type->data));
        curr->alias = alias;
        curr->next = type->data;
        type->data = curr;

        return meta_factory<Type, std::integral_constant<decltype(Data), Data>>{&curr->prop};
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
     * @param alias Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_is_t>
    auto data(const ENTT_ID_TYPE alias) ENTT_NOEXCEPT {
        using underlying_type = std::invoke_result_t<decltype(Getter), Type &>;
        static_assert(std::is_invocable_v<decltype(Setter), Type &, underlying_type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_data_node node{
            {},
            type,
            nullptr,
            nullptr,
            false,
            false,
            &internal::meta_info<underlying_type>::resolve,
            &internal::setter<false, Type, Setter>,
            &internal::getter<Type, Getter, Policy>
        };

        ENTT_ASSERT(!exists(alias, type->data));
        ENTT_ASSERT(!exists(&node, type->data));
        node.alias = alias;
        node.next = type->data;
        type->data = &node;

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
     * @param alias Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto func(const ENTT_ID_TYPE alias) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            {},
            type,
            nullptr,
            nullptr,
            helper_type::index_sequence.size(),
            helper_type::is_const,
            !std::is_member_function_pointer_v<decltype(Candidate)>,
            &internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename helper_type::return_type>>::resolve,
            &helper_type::arg,
            [](meta_any instance, meta_any *args) {
                return internal::invoke<Type, Candidate, Policy>(std::move(instance), args, helper_type::index_sequence);
            }
        };

        ENTT_ASSERT(!exists(alias, type->func));
        ENTT_ASSERT(!exists(&node, type->func));
        node.alias = alias;
        node.next = type->func;
        type->func = &node;

        return meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
    }

    /**
     * @brief Resets a meta type and all its parts.
     *
     * This function resets a meta type and all its data members, member
     * functions and properties, as well as its constructors, destructors and
     * conversion functions if any.<br/>
     * Base classes aren't reset but the link between the two types is removed.
     *
     * @return An extended meta factory for the given type.
     */
    auto reset() ENTT_NOEXCEPT {
        auto * const node = internal::meta_info<Type>::resolve();
        auto **it = internal::meta_info<>::global;

        while(*it && *it != node) {
            it = &(*it)->next;
        }

        if(*it) {
            *it = (*it)->next;
        }

        const auto unregister_all = y_combinator{
            [](auto &&self, auto **curr, auto... member) {
                while(*curr) {
                    auto *prev = *curr;
                    (self(&(prev->*member)), ...);
                    *curr = prev->next;
                    prev->next = nullptr;
                }
            }
        };

        unregister_all(&node->prop);
        unregister_all(&node->base);
        unregister_all(&node->conv);
        unregister_all(&node->ctor, &internal::meta_ctor_node::prop);
        unregister_all(&node->data, &internal::meta_data_node::prop);
        unregister_all(&node->func, &internal::meta_func_node::prop);

        node->alias = {};
        node->next = nullptr;
        node->dtor = nullptr;

        return meta_factory<Type, Type>{&node->prop};
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
 * @return An meta factory for the given type.
 */
template<typename Type>
inline meta_factory<Type> meta() ENTT_NOEXCEPT {
    auto * const node = internal::meta_info<Type>::resolve();
    // extended meta factory to allow assigning properties to opaque meta types
    return meta_factory<Type, Type>{&node->prop};
}


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve();
}


/**
 * @brief Returns the meta type associated with a given alias.
 * @param alias Unique identifier.
 * @return The meta type associated with the given alias, if any.
 */
inline meta_type resolve(const ENTT_ID_TYPE alias) ENTT_NOEXCEPT {
    return internal::find_if([alias](const auto *curr) {
        return curr->alias == alias;
    }, *internal::meta_info<>::global);
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
inline std::enable_if_t<std::is_invocable_v<Op, meta_type>, void>
resolve(Op op) {
    internal::visit<meta_type>(std::move(op), *internal::meta_info<>::global);
}


}


#endif
