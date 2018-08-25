#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <array>
#include <cassert>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "any.hpp"
#include "info.hpp"


namespace entt {


/**
 * TODO
 * - drop meta info
 * - add a layer that abstracts from nodes and make nodes private
 * - props aren't unique if one can define multiple times something (ctor, dtor, etc)
 * - next non-const?
 */


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


class MetaSystem final {
    template<typename Name, typename Node>
    inline static bool duplicate(const Name &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline static bool duplicate(const MetaAny &key, const MetaPropNode *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename...>
    static MetaPropNode * properties() {
        return nullptr;
    }

    template<typename... Dispatch, typename Property, typename... Other>
    static MetaPropNode * properties(const Property &property, const Other &... other) {
        static const Property prop{property};
        static MetaPropNode node{
            properties<Dispatch...>(other...),
            +[]() ENTT_NOEXCEPT {
                return MetaAny{prop.first};
            },
            +[]() ENTT_NOEXCEPT {
                return MetaAny{prop.second};
            }
        };

        assert(!duplicate(MetaAny{prop.first}, node.next));
        return &node;
    }

    template<typename, typename = void>
    struct MetaFactory final {};

    template<typename Class>
    class MetaFactory<Class, std::enable_if_t<std::is_class<Class>::value>> {
        template<typename... Args, std::size_t... Indexes>
        static MetaAny constructor(const MetaAny * const any, std::index_sequence<Indexes...>) {
            return MetaAny{Class{(any+Indexes)->get<std::decay_t<Args>>()...}};
        }

        template<bool Const, typename Type, Type Class:: *Member>
        static std::enable_if_t<Const>
        setter(void *, const MetaAny &) {
            assert(false);
        }

        template<bool Const, typename Type, Type Class:: *Member>
        static std::enable_if_t<!Const>
        setter(void *instance, const MetaAny &any) {
            static_cast<Class *>(instance)->*Member = any.get<std::decay_t<Type>>();
        }

        template<typename>
        struct CommonFuncHelper;

        template<typename Ret, typename... Args>
        struct CommonFuncHelper<Ret(Args...)> {
            using return_type = Ret;
            static constexpr auto size = sizeof...(Args);

            static auto arg(typename MetaFuncNode::size_type index) ENTT_NOEXCEPT {
                return std::array<MetaTypeNode *, sizeof...(Args)>{{meta<Args>()...}}[index];
            }

            static auto accept(const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                std::array<MetaTypeNode *, sizeof...(Args)> args{{meta<Args>()...}};
                return std::equal(args.cbegin(), args.cend(), types);
            }
        };

        template<typename Type, Type Class:: *>
        struct MemberFuncHelper;

        template<typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
        struct MemberFuncHelper<Ret(Args...), Member>: CommonFuncHelper<Ret(Args...)> {
            template<std::size_t... Indexes>
            static auto invoke(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
            -> decltype(MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...)}, MetaAny{})
            {
                return MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...)};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
                (static_cast<Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...);
                return MetaAny{};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
                assert(false);
                return MetaAny{};
            }

            static auto invoke(void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }

            static auto cinvoke(const void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }
        };

        template<typename Ret, typename... Args, Ret(Class:: *Member)(Args...) const>
        struct MemberFuncHelper<Ret(Args...) const, Member>: CommonFuncHelper<Ret(Args...)> {
            template<std::size_t... Indexes>
            static auto invoke(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
            -> decltype(MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...)}, MetaAny{})
            {
                return MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...)};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
                (static_cast<const Class *>(instance)->*Member)((any+Indexes)->get<std::decay_t<Args>>()...);
                return MetaAny{};
            }

            static auto invoke(void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }

            static auto cinvoke(const void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }
        };

        template<typename Type, Type *>
        struct FreeFuncHelper;

        template<typename Ret, typename... Args, Ret(*Func)(Class &, Args...)>
        struct FreeFuncHelper<Ret(Class &, Args...), Func>: CommonFuncHelper<Ret(Args...)> {
            template<std::size_t... Indexes>
            static auto invoke(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
            -> decltype(MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...)}, MetaAny{})
            {
                return MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...)};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
                (*Func)(*static_cast<Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...);
                return MetaAny{};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
                assert(false);
                return MetaAny{};
            }

            static auto invoke(void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }

            static auto cinvoke(const void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }
        };

        template<typename Ret, typename... Args, Ret(*Func)(const Class &, Args...)>
        struct FreeFuncHelper<Ret(const Class &, Args...), Func>: CommonFuncHelper<Ret(Args...)> {
            template<std::size_t... Indexes>
            static auto invoke(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
            -> decltype(MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...)}, MetaAny{})
            {
                return MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...)};
            }

            template<std::size_t... Indexes>
            static auto invoke(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
                (*Func)(*static_cast<const Class *>(instance), (any+Indexes)->get<std::decay_t<Args>>()...);
                return MetaAny{};
            }

            static auto invoke(void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }

            static auto cinvoke(const void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }
        };

    public:
        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            static bool watchdog = true;
            static MetaCtorNode node{
                MetaTypeNode::type<Class>->ctor,
                properties<Class, Args...>(std::forward<Property>(property)...),
                sizeof...(Args),
                +[](typename MetaCtorNode::size_type index) ENTT_NOEXCEPT {
                    return std::array<MetaTypeNode *, sizeof...(Args)>{{meta<Args>()...}}[index];
                },
                +[](const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                    std::array<MetaTypeNode *, sizeof...(Args)> args{{meta<Args>()...}};
                    return std::equal(args.cbegin(), args.cend(), types);
                },
                +[](const MetaAny * const any) {
                    return constructor<Args...>(any, std::make_index_sequence<sizeof...(Args)>{});
                }
            };

            assert(watchdog);
            watchdog = !watchdog;
            MetaTypeNode::type<Class>->ctor = &node;
            return MetaFactory<Class>{};
        }

        template<void(*Func)(Class &), typename... Property>
        static auto dtor(Property &&... property) ENTT_NOEXCEPT {
            static bool watchdog = true;
            static MetaDtorNode node{
                properties<Class, std::integral_constant<void(*)(Class &), Func>>(std::forward<Property>(property)...),
                +[](void *instance) {
                    (*Func)(*static_cast<Class *>(instance));
                }
            };

            assert(watchdog);
            watchdog = !watchdog;
            assert(!MetaTypeNode::type<Class>->dtor);
            MetaTypeNode::type<Class>->dtor = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static bool watchdog = true;
            static MetaDataNode node{
                HashedString{str},
                MetaTypeNode::type<Class>->data,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                std::is_const<Type>::value,
                &meta<Type>,
                &setter<std::is_const<Type>::value, Type, Member>,
                +[](const void *instance) ENTT_NOEXCEPT {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                +[](const MetaTypeNode * const other) ENTT_NOEXCEPT {
                    return other == meta<Type>();
                }
            };

            assert(watchdog);
            watchdog = !watchdog;
            assert(!duplicate(HashedString{str}, MetaTypeNode::type<Class>->data));
            MetaTypeNode::type<Class>->data = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static bool watchdog = true;
            static MetaFuncNode node{
                HashedString{str},
                MetaTypeNode::type<Class>->func,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                MemberFuncHelper<Type, Member>::size,
                &meta<typename MemberFuncHelper<Type, Member>::return_type>,
                &MemberFuncHelper<Type, Member>::arg,
                &MemberFuncHelper<Type, Member>::accept,
                &MemberFuncHelper<Type, Member>::cinvoke,
                &MemberFuncHelper<Type, Member>::invoke
            };

            assert(watchdog);
            watchdog = !watchdog;
            assert(!duplicate(HashedString{str}, MetaTypeNode::type<Class>->func));
            MetaTypeNode::type<Class>->func = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type *Func, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static bool watchdog = true;
            static MetaFuncNode node{
                HashedString{str},
                MetaTypeNode::type<Class>->func,
                properties<Class, std::integral_constant<Type *, Func>>(std::forward<Property>(property)...),
                FreeFuncHelper<Type, Func>::size,
                &meta<typename FreeFuncHelper<Type, Func>::return_type>,
                &FreeFuncHelper<Type, Func>::arg,
                &FreeFuncHelper<Type, Func>::accept,
                &FreeFuncHelper<Type, Func>::cinvoke,
                &FreeFuncHelper<Type, Func>::invoke
            };

            assert(watchdog);
            watchdog = !watchdog;
            assert(!duplicate(HashedString{str}, MetaTypeNode::type<Class>->func));
            MetaTypeNode::type<Class>->func = &node;
            return MetaFactory<Class>{};
        }
    };

    template<typename Type, typename... Property>
    static MetaFactory<Type> reflect(HashedString hs, Property &&... property) ENTT_NOEXCEPT {
        static bool watchdog = true;
        static MetaTypeNode node{
            hs,
            MetaTypeNode::type<>,
            properties<Type>(std::forward<Property>(property)...),
            nullptr,
            nullptr,
            nullptr,
            nullptr
        };

        assert(watchdog);
        watchdog = !watchdog;
        assert(!duplicate(hs, MetaTypeNode::type<>));
        assert(!MetaTypeNode::type<Type>);
        MetaTypeNode::type<Type> = &node;
        MetaTypeNode::type<> = &node;
        return MetaFactory<Type>{};
    }

public:
    template<typename Type>
    using factory_type = MetaFactory<Type>;

    template<typename Type, typename... Property>
    static factory_type<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        return reflect<Type>(HashedString{str}, std::forward<Property>(property)...);
    }

    template<typename Type>
    static MetaTypeNode * meta() ENTT_NOEXCEPT {
        if(!MetaTypeNode::type<std::decay_t<Type>>) {
            reflect<std::decay_t<Type>>(HashedString{});
        }

        return MetaTypeNode::type<std::decay_t<Type>>;
    }
};


}


#endif // ENTT_META_META_HPP
