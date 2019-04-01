#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/hashed_string.hpp"
#include "meta.hpp"


namespace entt {


template<typename>
class meta_factory;


template<typename Type, typename... Property>
meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;


template<typename Type>
bool unregister() ENTT_NOEXCEPT;


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
    static_assert(std::is_object_v<Type> && !(std::is_const_v<Type> || std::is_volatile_v<Type>));

    template<auto Data>
    static std::enable_if_t<std::is_member_object_pointer_v<decltype(Data)>, decltype(std::declval<Type>().*Data)>
    actual_type();

    template<auto Data>
    static std::enable_if_t<std::is_pointer_v<decltype(Data)>, decltype(*Data)>
    actual_type();

    template<auto Data>
    using data_type = std::remove_reference_t<decltype(meta_factory::actual_type<Data>())>;

    template<auto Func>
    using func_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;

    template<typename Node>
    inline bool duplicate(const hashed_string &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline bool duplicate(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename>
    internal::meta_prop_node * properties() {
        return nullptr;
    }

    template<typename Owner, typename Property, typename... Other>
    internal::meta_prop_node * properties(Property &&property, Other &&... other) {
        static std::decay_t<Property> prop{};

        static internal::meta_prop_node node{
            nullptr,
            []() -> meta_any {
                return std::get<0>(prop);
            },
            []() -> meta_any {
                return std::get<1>(prop);
            },
            []() -> meta_prop {
                return &node;
            }
        };

        prop = std::forward<Property>(property);
        node.next = properties<Owner>(std::forward<Other>(other)...);
        ENTT_ASSERT(!duplicate(meta_any{std::get<0>(prop)}, node.next));
        return &node;
    }

    template<typename... Property>
    meta_factory type(hashed_string name, Property &&... property) ENTT_NOEXCEPT {
        static internal::meta_type_node node{
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_function_v<Type>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            []() -> meta_type {
                return internal::meta_info<std::remove_pointer_t<Type>>::resolve();
            },
            &internal::destroy<Type>,
            []() -> meta_type {
                return &node;
            }
        };

        node.name = name;
        node.next = internal::meta_info<>::type;
        node.prop = properties<Type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(name, node.next));
        ENTT_ASSERT(!internal::meta_info<Type>::type);
        internal::meta_info<Type>::type = &node;
        internal::meta_info<>::type = &node;

        return *this;
    }

    void unregister_prop(internal::meta_prop_node **prop) {
        while(*prop) {
            auto *node = *prop;
            *prop = node->next;
            node->next = nullptr;
        }
    }

    void unregister_dtor() {
        if(auto node = internal::meta_info<Type>::type->dtor; node) {
            internal::meta_info<Type>::type->dtor = nullptr;
            *node->underlying = nullptr;
        }
    }

    template<auto Member>
    auto unregister_all(int) -> decltype((internal::meta_info<Type>::type->*Member)->prop, void()) {
        while(internal::meta_info<Type>::type->*Member) {
            auto node = internal::meta_info<Type>::type->*Member;
            internal::meta_info<Type>::type->*Member = node->next;
            unregister_prop(&node->prop);
            node->next = nullptr;
            *node->underlying = nullptr;
        }
    }

    template<auto Member>
    void unregister_all(char) {
        while(internal::meta_info<Type>::type->*Member) {
            auto node = internal::meta_info<Type>::type->*Member;
            internal::meta_info<Type>::type->*Member = node->next;
            node->next = nullptr;
            *node->underlying = nullptr;
        }
    }

    bool unregister() ENTT_NOEXCEPT {
        const auto registered = internal::meta_info<Type>::type;

        if(registered) {
            if(auto *curr = internal::meta_info<>::type; curr == internal::meta_info<Type>::type) {
                internal::meta_info<>::type = internal::meta_info<Type>::type->next;
            } else {
                while(curr && curr->next != internal::meta_info<Type>::type) {
                    curr = curr->next;
                }

                if(curr) {
                    curr->next = internal::meta_info<Type>::type->next;
                }
            }

            unregister_prop(&internal::meta_info<Type>::type->prop);
            unregister_all<&internal::meta_type_node::base>(0);
            unregister_all<&internal::meta_type_node::conv>(0);
            unregister_all<&internal::meta_type_node::ctor>(0);
            unregister_all<&internal::meta_type_node::data>(0);
            unregister_all<&internal::meta_type_node::func>(0);
            unregister_dtor();

            internal::meta_info<Type>::type->name = {};
            internal::meta_info<Type>::type->next = nullptr;
            internal::meta_info<Type>::type = nullptr;
        }

        return registered;
    }

    meta_factory() ENTT_NOEXCEPT = default;

public:
    template<typename Other, typename... Property>
    friend meta_factory<Other> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;

    template<typename Other>
    friend bool unregister() ENTT_NOEXCEPT;

    /**
     * @brief Assigns a meta base to a meta type.
     *
     * A reflected base class must be a real base class of the reflected type.
     *
     * @tparam Base Type of the base class to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename Base>
    meta_factory base() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<Base, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_base_node node{
            &internal::meta_info<Type>::template base<Base>,
            type,
            nullptr,
            &internal::meta_info<Base>::resolve,
            [](void *instance) -> void * {
                return static_cast<Base *>(static_cast<Type *>(instance));
            },
            []() -> meta_base {
                return &node;
            }
        };

        node.next = type->base;
        ENTT_ASSERT((!internal::meta_info<Type>::template base<Base>));
        internal::meta_info<Type>::template base<Base> = &node;
        type->base = &node;

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
    meta_factory conv() ENTT_NOEXCEPT {
        static_assert(std::is_convertible_v<Type, std::decay_t<To>>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_conv_node node{
            &internal::meta_info<Type>::template conv<To>,
            type,
            nullptr,
            &internal::meta_info<To>::resolve,
            [](void *instance) -> meta_any {
                return static_cast<std::decay_t<To>>(*static_cast<Type *>(instance));
            },
            []() -> meta_conv {
                return &node;
            }
        };

        node.next = type->conv;
        ENTT_ASSERT((!internal::meta_info<Type>::template conv<To>));
        internal::meta_info<Type>::template conv<To> = &node;
        type->conv = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of
     * constructors. All that is required is that they return an instance of the
     * underlying type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or a free function.
     *
     * @tparam Func The actual function to use as a constructor.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;
        static_assert(std::is_same_v<typename helper_type::return_type, Type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            &internal::meta_info<Type>::template ctor<typename helper_type::args_type>,
            type,
            nullptr,
            nullptr,
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::invoke<Type, Func>(nullptr, any, std::make_index_sequence<helper_type::size>{});
            },
            []() -> meta_ctor {
                return &node;
            }
        };

        node.next = type->ctor;
        node.prop = properties<typename helper_type::args_type>(std::forward<Property>(property)...);
        ENTT_ASSERT((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

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
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<typename... Args, typename... Property>
    meta_factory ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<Type(Args...)>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_ctor_node node{
            &internal::meta_info<Type>::template ctor<typename helper_type::args_type>,
            type,
            nullptr,
            nullptr,
            helper_type::size,
            &helper_type::arg,
            [](meta_any * const any) {
                return internal::construct<Type, Args...>(any, std::make_index_sequence<helper_type::size>{});
            },
            []() -> meta_ctor {
                return &node;
            }
        };

        node.next = type->ctor;
        node.prop = properties<typename helper_type::args_type>(std::forward<Property>(property)...);
        ENTT_ASSERT((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of destructors.
     * The signature of the function should identical to the following:
     *
     * @code{.cpp}
     * void(Type *);
     * @endcode
     *
     * From a client's point of view, nothing changes if the destructor of a
     * meta type is the default one or a custom one.
     *
     * @tparam Func The actual function to use as a destructor.
     * @return A meta factory for the parent type.
     */
    template<auto *Func>
    meta_factory dtor() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_v<decltype(Func), Type *>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_dtor_node node{
            &internal::meta_info<Type>::template dtor<Func>,
            type,
            [](meta_handle handle) {
                return handle.type() == internal::meta_info<Type>::resolve()->meta()
                        ? ((*Func)(static_cast<Type *>(handle.data())), true)
                        : false;
            },
            []() -> meta_dtor {
                return &node;
            }
        };

        ENTT_ASSERT(!internal::meta_info<Type>::type->dtor);
        ENTT_ASSERT((!internal::meta_info<Type>::template dtor<Func>));
        internal::meta_info<Type>::template dtor<Func> = &node;
        internal::meta_info<Type>::type->dtor = &node;

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
     * @tparam Property Types of properties to assign to the meta data.
     * @param str The name to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Data, typename... Property>
    meta_factory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::resolve();
        internal::meta_data_node *curr = nullptr;

        if constexpr(std::is_same_v<Type, decltype(Data)>) {
            static internal::meta_data_node node{
                &internal::meta_info<Type>::template data<Data>,
                {},
                type,
                nullptr,
                nullptr,
                true,
                true,
                &internal::meta_info<Type>::resolve,
                [](meta_handle, meta_any &) { return false; },
                [](meta_handle) -> meta_any { return Data; },
                []() -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<Type, Data>>(std::forward<Property>(property)...);
            curr = &node;
        } else {
            static internal::meta_data_node node{
                &internal::meta_info<Type>::template data<Data>,
                {},
                type,
                nullptr,
                nullptr,
                std::is_const_v<data_type<Data>>,
                !std::is_member_object_pointer_v<decltype(Data)>,
                &internal::meta_info<data_type<Data>>::resolve,
                &internal::setter<std::is_const_v<data_type<Data>>, Type, Data>,
                &internal::getter<Type, Data>,
                []() -> meta_data {
                    return &node;
                }
            };

            node.prop = properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...);
            curr = &node;
        }

        curr->name = hashed_string{str};
        curr->next = type->data;
        ENTT_ASSERT(!duplicate(hashed_string{str}, curr->next));
        ENTT_ASSERT((!internal::meta_info<Type>::template data<Data>));
        internal::meta_info<Type>::template data<Data> = curr;
        type->data = curr;

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     *
     * Setters and getters can be either free functions, member functions or a
     * mix of them.<br/>
     * In case of free functions, setters and getters must accept a pointer to
     * an instance of the parent type as their first argument. A setter has then
     * an extra argument of a type convertible to that of the parameter to
     * set.<br/>
     * In case of member functions, getters have no arguments at all, while
     * setters has an argument of a type convertible to that of the parameter to
     * set.
     *
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Property Types of properties to assign to the meta data.
     * @param str The name to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename... Property>
    meta_factory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::tuple<std::integral_constant<decltype(Setter), Setter>, std::integral_constant<decltype(Getter), Getter>>;
        using underlying_type = std::invoke_result_t<decltype(Getter), Type *>;
        static_assert(std::is_invocable_v<decltype(Setter), Type *, underlying_type>);
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_data_node node{
            &internal::meta_info<Type>::template data<Setter, Getter>,
            {},
            type,
            nullptr,
            nullptr,
            false,
            false,
            &internal::meta_info<underlying_type>::resolve,
            &internal::setter<false, Type, Setter>,
            &internal::getter<Type, Getter>,
            []() -> meta_data {
                return &node;
            }
        };

        node.name = hashed_string{str};
        node.next = type->data;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(hashed_string{str}, node.next));
        ENTT_ASSERT((!internal::meta_info<Type>::template data<Setter, Getter>));
        internal::meta_info<Type>::template data<Setter, Getter> = &node;
        type->data = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta funcion to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Func The actual function to attach to the meta type.
     * @tparam Property Types of properties to assign to the meta function.
     * @param str The name to assign to the meta function.
     * @param property Properties to assign to the meta function.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        using owner_type = std::integral_constant<decltype(Func), Func>;
        auto * const type = internal::meta_info<Type>::resolve();

        static internal::meta_func_node node{
            &internal::meta_info<Type>::template func<Func>,
            {},
            type,
            nullptr,
            nullptr,
            func_type<Func>::size,
            func_type<Func>::is_const,
            func_type<Func>::is_static,
            &internal::meta_info<typename func_type<Func>::return_type>::resolve,
            &func_type<Func>::arg,
            [](meta_handle handle, meta_any *any) {
                return internal::invoke<Type, Func>(handle, any, std::make_index_sequence<func_type<Func>::size>{});
            },
            []() -> meta_func {
                return &node;
            }
        };

        node.name = hashed_string{str};
        node.next = type->func;
        node.prop = properties<owner_type>(std::forward<Property>(property)...);
        ENTT_ASSERT(!duplicate(hashed_string{str}, node.next));
        ENTT_ASSERT((!internal::meta_info<Type>::template func<Func>));
        internal::meta_info<Type>::template func<Func> = &node;
        type->func = &node;

        return *this;
    }
};


/**
 * @brief Basic function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @tparam Property Types of properties to assign to the reflected type.
 * @param str The name to assign to the reflected type.
 * @param property Properties to assign to the reflected type.
 * @return A meta factory for the given type.
 */
template<typename Type, typename... Property>
inline meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return meta_factory<Type>{}.type(hashed_string{str}, std::forward<Property>(property)...);
}


/**
 * @brief Basic function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @return A meta factory for the given type.
 */
template<typename Type>
inline meta_factory<Type> reflect() ENTT_NOEXCEPT {
    return meta_factory<Type>{};
}


/**
 * @brief Basic function to unregister a type.
 *
 * This function unregisters a type and all its data members, member functions
 * and properties, as well as its constructors, destructors and conversion
 * functions if any.<br/>
 * Base classes aren't unregistered but the link between the two types is
 * removed.
 *
 * @tparam Type Type to unregister.
 * @return True if the type to unregister exists, false otherwise.
 */
template<typename Type>
inline bool unregister() ENTT_NOEXCEPT {
    return meta_factory<Type>().unregister();
}


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve()->meta();
}


/**
 * @brief Returns the meta type associated with a given name.
 * @param str The name to use to search for a meta type.
 * @return The meta type associated with the given name, if any.
 */
inline meta_type resolve(const char *str) ENTT_NOEXCEPT {
    const auto *curr = internal::find_if([name = hashed_string{str}](auto *node) {
        return node->name == name;
    }, internal::meta_info<>::type);

    return curr ? curr->meta() : meta_type{};
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
void resolve(Op op) ENTT_NOEXCEPT {
    internal::iterate([op = std::move(op)](auto *node) {
        op(node->meta());
    }, internal::meta_info<>::type);
}


}


#endif // ENTT_META_FACTORY_HPP
