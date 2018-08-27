#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <array>
#include <cassert>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "meta.hpp"


namespace entt {


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


class Meta final {
    template<typename Type, typename = void>
    struct MetaFactory {
        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaCtorNode node{
                internal::MetaInfo::type<Type>->ctor,
                properties<Type, Args...>(std::forward<Property>(property)...),
                sizeof...(Args),
                [](typename internal::MetaCtorNode::size_type index) ENTT_NOEXCEPT {
                    return std::array<internal::MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo::resolve<Args>()...}}[index];
                },
                [](const internal::MetaTypeNode ** const types) ENTT_NOEXCEPT {
                    std::array<internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
                    return std::equal(args.cbegin(), args.cend(), types);
                },
                [](const MetaAny * const any) {
                    return constructor<Type, Args...>(any, std::make_index_sequence<sizeof...(Args)>{});
                },
                []() ENTT_NOEXCEPT {
                    static MetaCtor meta{&node};
                    return &meta;
                }
            };

            assert((!internal::MetaInfo::ctor<Type, Args...>));
            internal::MetaInfo::ctor<Type, Args...> = &node;
            internal::MetaInfo::type<Type>->ctor = &node;
            return MetaFactory<Type>{};
        }

        template<void(*Func)(Type &), typename... Property>
        static auto dtor(Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaDtorNode node{
                properties<Type, std::integral_constant<void(*)(Type &), Func>>(std::forward<Property>(property)...),
                [](void *instance) {
                    (*Func)(*static_cast<Type *>(instance));
                },
                []() ENTT_NOEXCEPT {
                    static MetaDtor meta{&node};
                    return &meta;
                }
            };

            assert((internal::MetaInfo::type<Type>->dtor == internal::MetaInfo::dtor<Type, std::integral_constant<void(*)(Type &), &destroy<Type>>>));
            assert((!internal::MetaInfo::dtor<Type, std::integral_constant<void(*)(Type &), Func>>));
            internal::MetaInfo::dtor<Type, std::integral_constant<void(*)(Type &), Func>> = &node;
            internal::MetaInfo::type<Type>->dtor = &node;
            return MetaFactory<Type>{};
        }

        template<typename Func, Func *Ptr, typename... Property>
        static auto ext(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = FreeFuncHelper<Type, Func, Ptr>;

            static internal::MetaFuncNode node{
                HashedString{str},
                internal::MetaInfo::type<Type>->func,
                properties<Type, std::integral_constant<Func *, Ptr>>(std::forward<Property>(property)...),
                helper_type::size,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
                &helper_type::arg,
                &helper_type::accept,
                &helper_type::cinvoke,
                &helper_type::invoke,
                []() ENTT_NOEXCEPT {
                    static MetaFunc meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Type>->func));
            assert((!internal::MetaInfo::func<Type, std::integral_constant<Func *, Ptr>>));
            internal::MetaInfo::func<Type, std::integral_constant<Func *, Ptr>> = &node;
            internal::MetaInfo::type<Type>->func = &node;
            return MetaFactory<Type>{};
        }
    };

    template<typename Class>
    struct MetaFactory<Class, std::enable_if_t<std::is_class<Class>::value>>: MetaFactory<Class, Class> {
        template<typename Type, Type Class:: *Member, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaDataNode node{
                HashedString{str},
                internal::MetaInfo::type<Class>->data,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                std::is_const<Type>::value,
                &internal::MetaInfo::resolve<Type>,
                &setter<Class, Type, Member>,
                [](const void *instance) ENTT_NOEXCEPT {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                [](const internal::MetaTypeNode * const other) ENTT_NOEXCEPT {
                    return other == internal::MetaInfo::resolve<Type>();
                },
                []() ENTT_NOEXCEPT {
                    static MetaData meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Class>->data));
            assert((!internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>> = &node;
            internal::MetaInfo::type<Class>->data = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = MemberFuncHelper<Class, Type, Member>;

            static internal::MetaFuncNode node{
                HashedString{str},
                internal::MetaInfo::type<Class>->func,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::size,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
                &helper_type::arg,
                &helper_type::accept,
                &helper_type::cinvoke,
                &helper_type::invoke,
                []() ENTT_NOEXCEPT {
                    static MetaFunc meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Class>->func));
            assert((!internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>> = &node;
            internal::MetaInfo::type<Class>->func = &node;
            return MetaFactory<Class>{};
        }
    };

    template<typename>
    struct CommonFuncHelper;

    template<typename Ret, typename... Args>
    struct CommonFuncHelper<Ret(Args...)> {
        using return_type = Ret;
        static constexpr auto size = sizeof...(Args);

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<internal::MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo::resolve<Args>()...}}[index];
        }

        static auto accept(const internal::MetaTypeNode ** const types) ENTT_NOEXCEPT {
            std::array<internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }
    };

    template<typename Class, typename Type, Type Class:: *>
    class MemberFuncHelper;

    template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
    class MemberFuncHelper<Class, Ret(Args...), Member>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
            assert(false);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...) const>
    class MemberFuncHelper<Class, Ret(Args...) const, Member>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename, typename Type, Type *>
    class FreeFuncHelper;

    template<typename Class, typename Ret, typename... Args, Ret(*Func)(Class &, Args...)>
    class FreeFuncHelper<Class, Ret(Class &, Args...), Func>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
            assert(false);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Class, typename Ret, typename... Args, Ret(*Func)(const Class &, Args...)>
    class FreeFuncHelper<Class, Ret(const Class &, Args...), Func>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Name, typename Node>
    inline static bool duplicate(const Name &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline static bool duplicate(const MetaAny &key, const internal::MetaPropNode *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename Class>
    static void destroy(Class &instance) {
        instance.~Class();
    }

    template<typename...>
    static internal::MetaPropNode * properties() {
        return nullptr;
    }

    template<typename... Dispatch, typename Property, typename... Other>
    static internal::MetaPropNode * properties(const Property &property, const Other &... other) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};

        static internal::MetaPropNode node{
            properties<Dispatch...>(other...),
            []() ENTT_NOEXCEPT -> const MetaAny & {
                return key;
            },
            []() ENTT_NOEXCEPT -> const MetaAny & {
                return value;
            },
            []() ENTT_NOEXCEPT {
                static MetaProp meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(key, node.next));
        assert((!internal::MetaInfo::prop<Dispatch..., Property, Other...>));
        internal::MetaInfo::prop<Dispatch..., Property, Other...> = &node;
        return &node;
    }

    template<typename Class, typename... Args, std::size_t... Indexes>
    static MetaAny constructor(const MetaAny * const any, std::index_sequence<Indexes...>) {
        return MetaAny{Class{(any+Indexes)->to<std::decay_t<Args>>()...}};
    }

    template<typename Class, typename Type, Type Class:: *Member>
    static std::enable_if_t<std::is_const<Type>::value>
    setter(void *, const MetaAny &) {
        assert(false);
    }

    template<typename Class, typename Type, Type Class:: *Member>
    static std::enable_if_t<!std::is_const<Type>::value>
    setter(void *instance, const MetaAny &any) {
        static_cast<Class *>(instance)->*Member = any.to<std::decay_t<Type>>();
    }

    template<typename Type, typename... Property>
    static MetaFactory<Type> reflect(HashedString name, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaTypeNode node{
            name,
            internal::MetaInfo::type<>,
            properties<Type>(std::forward<Property>(property)...),
            []() {
                static MetaType meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(name, internal::MetaInfo::type<>));
        assert(!internal::MetaInfo::type<Type>);
        internal::MetaInfo::type<Type> = &node;
        internal::MetaInfo::type<> = &node;
        return MetaFactory<Type>{}.template dtor<&destroy<Type>>();
    }

public:
    template<typename Type>
    using factory_type = MetaFactory<Type>;

    template<typename Type, typename... Property>
    inline static factory_type<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        return reflect<Type>(HashedString{str}, std::forward<Property>(property)...);
    }

    template<typename Type>
    inline static MetaType * resolve() ENTT_NOEXCEPT {
        return internal::MetaInfo::resolve<Type>()->meta();
    }

    inline static MetaType * resolve(const char *str) ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, internal::MetaInfo::type<>);
    }
};


}


#endif // ENTT_META_FACTORY_HPP
