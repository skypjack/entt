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

    static constexpr auto size = sizeof...(Args);
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
bool setter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index, [[maybe_unused]] meta_any value) {
    bool accepted = false;

    if constexpr(!Const) {
        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using helper_type = meta_function_helper_t<decltype(Data)>;
            using data_type = std::tuple_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename helper_type::args_type>;
            static_assert(std::is_invocable_v<decltype(Data), Type &, data_type>);
            auto * const clazz = meta_any{handle}.try_cast<Type>();
            auto * const direct = value.try_cast<data_type>();

            if(clazz && (direct || value.convert<data_type>())) {
                std::invoke(Data, *clazz, direct ? *direct : value.cast<data_type>());
                accepted = true;
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
            static_assert(std::is_invocable_v<decltype(Data), Type *>);
            auto * const clazz = meta_any{handle}.try_cast<Type>();

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
meta_any getter([[maybe_unused]] meta_handle handle, [[maybe_unused]] meta_any index) {
    auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::forward<decltype(value)>(value))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Data)>> || std::is_member_function_pointer_v<decltype(Data)>) {
        static_assert(std::is_invocable_v<decltype(Data), Type &>);
        auto * const clazz = meta_any{handle}.try_cast<Type>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>;
        static_assert(std::is_invocable_v<decltype(Data), Type *>);
        auto * const clazz = meta_any{handle}.try_cast<Type>();

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
meta_any invoke([[maybe_unused]] meta_handle handle, meta_any *args, std::index_sequence<Indexes...>) {
    using helper_type = meta_function_helper_t<decltype(Candidate)>;

    auto dispatch = [](auto *... args) {
        if constexpr(std::is_void_v<typename helper_type::return_type> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, *args...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_alias_t>) {
            return meta_any{std::ref(std::invoke(Candidate, *args...))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>);
            return meta_any{std::invoke(Candidate, *args...)};
        }
    };

    [[maybe_unused]] const auto direct = std::make_tuple([](meta_any *any, auto *instance) {
        using arg_type = std::remove_reference_t<decltype(*instance)>;

        if(!instance && any->convert<arg_type>()) {
            instance = any->try_cast<arg_type>();
        }

        return instance;
    }(args+Indexes, (args+Indexes)->try_cast<std::tuple_element_t<Indexes, typename helper_type::args_type>>())...);

    if constexpr(std::is_function_v<std::remove_pointer_t<decltype(Candidate)>>) {
        return (std::get<Indexes>(direct) && ...) ? dispatch(std::get<Indexes>(direct)...) : meta_any{};
    } else {
        auto * const clazz = meta_any{handle}.try_cast<Type>();
        return (clazz && (std::get<Indexes>(direct) && ...)) ? dispatch(clazz, std::get<Indexes>(direct)...) : meta_any{};
    }
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


template<typename, typename...>
class extended_meta_factory;


/**
 * @brief A meta factory to be used for reflection purposes.
 *
 * A meta factory is an utility class used to reflect types, data and functions
 * of all sorts. This class ensures that the underlying web of types is built
 * correctly and performs some checks in debug mode to ensure that there are no
 * subtle errors at runtime.
 *
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory {
    template<typename Node>
    bool duplicate(const Node *candidate, const Node *node) ENTT_NOEXCEPT {
        return node && (node == candidate || duplicate(candidate, node->next));
    }

    template<typename Node>
    bool duplicate(const ENTT_ID_TYPE identifier, const Node *node) ENTT_NOEXCEPT {
        return node && (node->identifier == identifier || duplicate(identifier, node->next));
    }

public:
    /**
     * @brief Extends a meta type by assigning it an identifier.
     * @param identifier Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    auto type(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
        auto * const node = internal::meta_info<Type>::resolve();

        ENTT_ASSERT(!duplicate(identifier, *internal::meta_info<>::ctx));
        ENTT_ASSERT(!duplicate(node, *internal::meta_info<>::ctx));
        node->identifier = identifier;
        node->next = *internal::meta_info<>::ctx;
        *internal::meta_info<>::ctx = node;

        return extended_meta_factory<Type>{&node->prop};
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

        ENTT_ASSERT(!duplicate(&node, type->base));
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

        ENTT_ASSERT(!duplicate(&node, type->conv));
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

        ENTT_ASSERT(!duplicate(&node, type->conv));
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
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func, Policy>({}, any, std::make_index_sequence<helper_type::size>{});
            }
        };

        ENTT_ASSERT(!duplicate(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

        return extended_meta_factory<Type, std::integral_constant<decltype(Func), Func>>{&node.prop};
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
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, std::remove_cv_t<std::remove_reference_t<Args>>...>(any, std::make_index_sequence<helper_type::size>{});
            }
        };

        ENTT_ASSERT(!duplicate(&node, type->ctor));
        node.next = type->ctor;
        type->ctor = &node;

        return extended_meta_factory<Type, Type(Args...)>{&node.prop};
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
            [](meta_handle handle) {
                const auto valid = (handle.type() == internal::meta_info<Type>::resolve());

                if(valid) {
                    std::invoke(Func, *meta_any{handle}.try_cast<Type>());
                }

                return valid;
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
     * @param identifier Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_is_t>
    auto data(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
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
                [](meta_handle, meta_any, meta_any) { return false; },
                [](meta_handle, meta_any) -> meta_any { return Data; }
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

        ENTT_ASSERT(!duplicate(identifier, type->data));
        ENTT_ASSERT(!duplicate(curr, type->data));
        curr->identifier = identifier;
        curr->next = type->data;
        type->data = curr;

        return extended_meta_factory<Type, std::integral_constant<decltype(Data), Data>>{&curr->prop};
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
     * @param identifier Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_is_t>
    auto data(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
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

        ENTT_ASSERT(!duplicate(identifier, type->data));
        ENTT_ASSERT(!duplicate(&node, type->data));
        node.identifier = identifier;
        node.next = type->data;
        type->data = &node;

        return extended_meta_factory<Type, std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>{&node.prop};
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
     * @param identifier Unique identifier.
     * @return An extended meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_is_t>
    auto func(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper_t<decltype(Candidate)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            {},
            type,
            nullptr,
            nullptr,
            helper_type::size,
            helper_type::is_const,
            !std::is_member_function_pointer_v<decltype(Candidate)>,
            &internal::meta_info<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, typename helper_type::return_type>>::resolve,
            &helper_type::arg,
            [](meta_handle handle, meta_any *any) {
                return internal::invoke<Type, Candidate, Policy>(handle, any, std::make_index_sequence<helper_type::size>{});
            }
        };

        ENTT_ASSERT(!duplicate(identifier, type->func));
        ENTT_ASSERT(!duplicate(&node, type->func));
        node.identifier = identifier;
        node.next = type->func;
        type->func = &node;

        return extended_meta_factory<Type, std::integral_constant<decltype(Candidate), Candidate>>{&node.prop};
    }

    /**
     * @brief Resets a meta type and all its parts.
     *
     * This function resets a meta type and all its data members, member
     * functions and properties, as well as its constructors, destructors and
     * conversion functions if any.<br/>
     * Base classes aren't reset but the link between the two types is removed.
     */
    void reset() ENTT_NOEXCEPT {
        internal::meta_info<Type>::reset();
    }
};


/**
 * @brief Extended meta factory to be used for reflection purposes.
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type, typename...>
class extended_meta_factory: public meta_factory<Type> {
    /*! @brief A meta factory is allowed to _extend_ itself. */
    friend class meta_factory<Type>;

    bool duplicate(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node && (node->key() == key || duplicate(key, node->next));
    }

    template<auto Key, auto Value>
    void prop(property<Key, Value>) {
        static internal::meta_prop_node node{
            *props,
            []() -> meta_any {
                return Key;
            },
            []() -> meta_any {
                return Value;
            }
        };

        ENTT_ASSERT(!duplicate(meta_any{Key}, *props));
        *props = &node;
    }

    extended_meta_factory(entt::internal::meta_prop_node **target)
        : props{target}
    {}

public:
    /**
     * @brief Assigns properties to the last meta object created.
     * @tparam Property Properties to assign to the meta object.
     * @return A meta factory for the parent type.
     */
    template<typename... Property>
    auto prop() {
        ENTT_ASSERT(props);
        (prop(Property{}), ...);
        return *this;
    }

private:
    entt::internal::meta_prop_node **props{nullptr};
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
inline meta_factory<Type> meta() ENTT_NOEXCEPT {
    return meta_factory<Type>{};
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
 * @brief Returns the meta type associated with a given identifier.
 * @param identifier Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
inline meta_type resolve(const ENTT_ID_TYPE identifier) ENTT_NOEXCEPT {
    return internal::find_if([identifier](auto *node) {
        return node->identifier == identifier;
    }, *internal::meta_info<>::ctx);
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
inline std::enable_if_t<std::is_invocable_v<Op, meta_type>, void>
resolve(Op op) ENTT_NOEXCEPT {
    internal::iterate<meta_type>(std::move(op), *internal::meta_info<>::ctx);
}


}


#endif // ENTT_META_FACTORY_HPP
