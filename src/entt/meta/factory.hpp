#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "internal.hpp"
#include "meta.hpp"
#include "policy.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename, bool = false>
struct meta_function_helper;


template<typename Ret, typename... Args, bool Const>
struct meta_function_helper<Ret(Args...), Const> {
    using return_type = std::remove_cv_t<std::remove_reference_t<Ret>>;
    using args_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

    static constexpr auto is_const = Const;

    [[nodiscard]] static auto arg(typename internal::meta_func_node::size_type index) ENTT_NOEXCEPT {
        return std::array<meta_type_node *, sizeof...(Args)>{{meta_info<Args>::resolve()...}}[index];
    }
};


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...), true>
to_meta_function_helper(Ret(Class:: *)(Args...) const);


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(Class:: *)(Args...));


template<typename Ret, typename... Args>
constexpr meta_function_helper<Ret(Args...)>
to_meta_function_helper(Ret(*)(Args...));


constexpr void to_meta_function_helper(...);


template<typename Candidate>
using meta_function_helper_t = decltype(to_meta_function_helper(std::declval<Candidate>()));


template<typename Type, typename... Args, std::size_t... Indexes>
[[nodiscard]] meta_any construct(meta_any * const args, std::index_sequence<Indexes...>) {
    [[maybe_unused]] auto direct = std::make_tuple((args+Indexes)->try_cast<Args>()...);
    return ((std::get<Indexes>(direct) || (args+Indexes)->convert<Args>()) && ...)
            ? Type{(std::get<Indexes>(direct) ? *std::get<Indexes>(direct) : (args+Indexes)->cast<Args>())...}
            : meta_any{};
}


template<typename Type, auto Data>
[[nodiscard]] bool setter([[maybe_unused]] meta_handle instance, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
        using helper_type = meta_function_helper_t<decltype(Data)>;
        using data_type = std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>;

        if(auto * const clazz = instance->try_cast<Type>(); clazz) {
            if(auto * const direct = value.try_cast<data_type>(); direct || value.convert<data_type>()) {
                std::invoke(Data, *clazz, direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        }
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;

        if constexpr(!std::is_array_v<data_type>) {
            if(auto * const clazz = instance->try_cast<Type>(); clazz) {
                if(auto * const direct = value.try_cast<data_type>(); direct || value.convert<data_type>()) {
                    std::invoke(Data, clazz) = (direct ? *direct : value.cast<data_type>());
                    accepted = true;
                }
            }
        }
    } else {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(*Data)>>;

        if constexpr(!std::is_array_v<data_type>) {
            if(auto * const direct = value.try_cast<data_type>(); direct || value.convert<data_type>()) {
                *Data = (direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        }
    }

    return accepted;
}


template<typename Type, auto Data, typename Policy>
[[nodiscard]] meta_any getter([[maybe_unused]] meta_handle instance) {
    [[maybe_unused]] auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>, std::forward<decltype(value)>(value)};
        } else if constexpr(std::is_same_v<Policy, as_ref_t>) {
            return meta_any{std::ref(std::forward<decltype(value)>(value))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>, "Policy not supported");
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
        auto * const clazz = instance->try_cast<Type>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        if constexpr(std::is_array_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>>) {
            return meta_any{};
        } else {
            auto * const clazz = instance->try_cast<Type>();
            return clazz ? dispatch(std::invoke(Data, clazz)) : meta_any{};
        }
    } else if constexpr(std::is_pointer_v<std::decay_t<decltype(Data)>>) {
        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            return meta_any{};
        } else {
            return dispatch(*Data);
        }
    } else {
        return dispatch(Data);
    }
}


template<typename Type, auto Candidate, typename Policy, std::size_t... Indexes>
[[nodiscard]] meta_any invoke([[maybe_unused]] meta_handle instance, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper_t<decltype(Candidate)>;

    auto dispatch = [](auto *... params) {
        if constexpr(std::is_void_v<typename helper_type::return_type> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, *params...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_ref_t>) {
            return meta_any{std::ref(std::invoke(Candidate, *params...))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>, "Policy not supported");
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
        auto * const clazz = instance->try_cast<Type>();
        return (clazz && (std::get<Indexes>(direct) && ...)) ? dispatch(clazz, std::get<Indexes>(direct)...) : meta_any{};
    }
}


}


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
 * @tparam Spec Property specialization pack used to disambiguate overloads.
 */
template<typename Type, typename... Spec>
class meta_factory<Type, Spec...>: public meta_factory<Type> {
    [[nodiscard]] bool exists(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
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
    meta_factory(internal::meta_prop_node **target) ENTT_NOEXCEPT
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
    internal::meta_prop_node **curr;
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
    bool exists(const id_type id, const Node *node) ENTT_NOEXCEPT {
        return node && (node->id == id || exists(id, node->next));
    }

public:
    /**
     * @brief Makes a meta type _searchable_.
     * @param id Optional unique identifier.
     * @return An extended meta factory for the given type.
     */
    auto type(const id_type id = type_info<Type>::id()) {
        auto * const node = internal::meta_info<Type>::resolve();

        ENTT_ASSERT(!exists(id, *internal::meta_context::global()));
        ENTT_ASSERT(!exists(node, *internal::meta_context::global()));
        node->id = id;
        node->next = *internal::meta_context::global();
        *internal::meta_context::global() = node;

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
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](const void *instance) ENTT_NOEXCEPT -> const void * {
                return static_cast<const Base *>(static_cast<const Type *>(instance));
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
        static_assert(std::is_convertible_v<Type, To>, "Could not convert to the required type");
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
        static_assert(std::is_same_v<typename helper_type::return_type, Type>, "The function doesn't return an object of the required type");
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            type,
            nullptr,
            nullptr,
            std::tuple_size_v<typename helper_type::args_type>,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func, Policy>({}, any, std::make_index_sequence<std::tuple_size_v<typename helper_type::args_type>>{});
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
            std::tuple_size_v<typename helper_type::args_type>,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, std::remove_cv_t<std::remove_reference_t<Args>>...>(any, std::make_index_sequence<std::tuple_size_v<typename helper_type::args_type>>{});
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
        static_assert(std::is_invocable_v<decltype(Func), Type &>, "The function doesn't accept an object of the type provided");
        auto * const type = internal::meta_info<Type>::resolve();

        ENTT_ASSERT(!type->dtor);

        type->dtor = [](void *instance) {
            if(instance) {
                std::invoke(Func, *static_cast<Type *>(instance));
            }
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
            using data_type = std::remove_pointer_t<std::decay_t<decltype(Data)>>;
            auto * const type = internal::meta_info<Type>::resolve();

            static internal::meta_data_node node{
                {},
                type,
                nullptr,
                nullptr,
                true,
                &internal::meta_info<data_type>::resolve,
                []() -> std::remove_const_t<decltype(internal::meta_data_node::set)> {
                    if constexpr(std::is_same_v<Type, data_type> || std::is_const_v<data_type>) {
                        return nullptr;
                    } else {
                        return &internal::setter<Type, Data>;
                    }
                }(),
                &internal::getter<Type, Data, Policy>
            };

            ENTT_ASSERT(!exists(id, type->data));
            ENTT_ASSERT(!exists(&node, type->data));
            node.id = id;
            node.next = type->data;
            type->data = &node;

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
            type,
            nullptr,
            nullptr,
            false,
            &internal::meta_info<underlying_type>::resolve,
            []() -> std::remove_const_t<decltype(internal::meta_data_node::set)> {
                if constexpr(std::is_same_v<decltype(Setter), std::nullptr_t> || (std::is_member_object_pointer_v<decltype(Setter)> && std::is_const_v<underlying_type>)) {
                    return nullptr;
                } else {
                    return &internal::setter<Type, Setter>;
                }
            }(),
            &internal::getter<Type, Getter, Policy>
        };

        ENTT_ASSERT(!exists(id, type->data));
        ENTT_ASSERT(!exists(&node, type->data));
        node.id = id;
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
     * @param id Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto func(const id_type id) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            {},
            type,
            nullptr,
            nullptr,
            std::tuple_size_v<typename helper_type::args_type>,
            helper_type::is_const,
            !std::is_member_function_pointer_v<decltype(Candidate)>,
            &internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename helper_type::return_type>>::resolve,
            &helper_type::arg,
            [](meta_handle instance, meta_any *args) {
                return internal::invoke<Type, Candidate, Policy>(*instance, args, std::make_index_sequence<std::tuple_size_v<typename helper_type::args_type>>{});
            }
        };

        ENTT_ASSERT(!exists(id, type->func));
        ENTT_ASSERT(!exists(&node, type->func));
        node.id = id;
        node.next = type->func;
        type->func = &node;

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


}


#endif
