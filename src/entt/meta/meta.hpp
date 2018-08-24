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


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


// TODO temporary workaround for tests purposes
template<typename...>
struct MetaSystemBase {
    static MetaTypeNode *chain;
};


template<typename... Types>
MetaTypeNode * MetaSystemBase<Types...>::chain = nullptr;


class MetaSystem final: MetaSystemBase<> {
    template<typename Key, typename Node>
    inline static bool duplicate(const Key &key, Node *node) ENTT_NOEXCEPT {
        return node ? node->key == key || duplicate(key, node->next) : false;
    }

    template<typename Type, std::size_t Index, typename Property>
    static void type(const Property &property) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static MetaPropNode node{key, value, internal::MetaInfo<Type>::prop};
        assert(!duplicate(key, internal::MetaInfo<Type>::prop));
        internal::MetaInfo<Type>::prop = &node;
    }

    template<typename Type, std::size_t... Indexes, typename... Property>
    static void type(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (type<Type, Indexes>(std::forward<Property>(property)), 0)... };
        (void)accumulator;
    }

    template<typename, typename = void>
    struct MetaFactory final {};

    template<typename Class>
    class MetaFactory<Class, std::enable_if_t<std::is_class<Class>::value>> {
        template<std::size_t Index, typename... Args, typename Property>
        static void ctor(const Property &property) {
            static const MetaAny key{property.first};
            static const MetaAny value{property.second};
            static MetaPropNode node{key, value, internal::MetaInfo<Class>::template Ctor<Args...>::prop};
            assert(!duplicate(key, internal::MetaInfo<Class>::template Ctor<Args...>::prop));
            internal::MetaInfo<Class>::template Ctor<Args...>::prop = &node;
        }

        template<typename... Args, std::size_t... Indexes, typename... Property>
        static void ctor(std::index_sequence<Indexes...>, Property &&... property) {
            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (ctor<Indexes, Args...>(std::forward<Property>(property)), 0)... };
            (void)accumulator;
        }

        template<std::size_t Index, void(*Func)(Class &), typename Property>
        static void dtor(const Property &property) {
            static const MetaAny key{property.first};
            static const MetaAny value{property.second};
            static MetaPropNode node{key, value, internal::MetaInfo<Class>::template Dtor<Func>::prop};
            assert(!duplicate(key, internal::MetaInfo<Class>::template Dtor<Func>::prop));
            internal::MetaInfo<Class>::template Dtor<Func>::prop = &node;
        }

        template<void(*Func)(Class &), std::size_t... Indexes, typename... Property>
        static void dtor(std::index_sequence<Indexes...>, Property &&... property) {
            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (dtor<Indexes, Func>(std::forward<Property>(property)), 0)... };
            (void)accumulator;
        }

        template<std::size_t Index, typename Type, Type Class:: *Member, typename Property>
        static void member(const Property &property) {
            static const MetaAny key{property.first};
            static const MetaAny value{property.second};
            static MetaPropNode node{key, value, internal::MetaInfo<Class>::template Member<Type, Member>::prop};
            assert(!duplicate(key, internal::MetaInfo<Class>::template Member<Type, Member>::prop));
            internal::MetaInfo<Class>::template Member<Type, Member>::prop = &node;
        }

        template<typename Type, Type Class:: *Member, std::size_t... Indexes, typename... Property>
        static void member(std::index_sequence<Indexes...>, Property &&... property) {
            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (member<Indexes, Type, Member>(std::forward<Property>(property)), 0)... };
            (void)accumulator;
        }

        template<std::size_t Index, typename Type, Type *Func, typename Property>
        static void func(const Property &property) {
            static const MetaAny key{property.first};
            static const MetaAny value{property.second};
            static MetaPropNode node{key, value, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop};
            assert(!duplicate(key, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop));
            internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop = &node;
        }

        template<typename Type, Type *Func, std::size_t... Indexes, typename... Property>
        static void func(std::index_sequence<Indexes...>, Property &&... property) {
            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (func<Indexes, Type, Func>(std::forward<Property>(property)), 0)... };
            (void)accumulator;
        }

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
            static typename MetaFuncNode::size_type size() ENTT_NOEXCEPT {
                return sizeof...(Args);
            }

            static auto ret() ENTT_NOEXCEPT {
                return internal::MetaInfo<Ret>::type;
            }

            static auto arg(typename MetaFuncNode::size_type index) ENTT_NOEXCEPT {
                return std::array<MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo<std::decay_t<Args>>::type...}}[index];
            }

            static auto accept(const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                std::array<MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo<std::decay_t<Args>>::type...}};
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

            static auto invoke(const void *instance, const MetaAny *any) {
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

            static auto invoke(const void *instance, const MetaAny *any) {
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

            static auto invoke(const void *instance, const MetaAny *any) {
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

            static auto invoke(const void *instance, const MetaAny *any) {
                return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
            }
        };

    public:
        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            static MetaCtorNode node{
                internal::MetaInfo<Class>::ctor,
                +[]() ENTT_NOEXCEPT {
                    return sizeof...(Args);
                },
                +[](typename MetaCtorNode::size_type index) ENTT_NOEXCEPT {
                    return std::array<MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo<std::decay_t<Args>>::type...}}[index];
                },
                +[](const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                    std::array<MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo<std::decay_t<Args>>::type...}};
                    return std::equal(args.cbegin(), args.cend(), types);
                },
                +[](const MetaAny * const any) {
                    return constructor<Args...>(any, std::make_index_sequence<sizeof...(Args)>{});
                },
                +[]() ENTT_NOEXCEPT {
                    return internal::MetaInfo<Class>::template Ctor<Args...>::prop;
                }
            };

            assert(!internal::MetaInfo<Class>::template Ctor<Args...>::ctor);
            internal::MetaInfo<Class>::template Ctor<Args...>::ctor = &node;
            internal::MetaInfo<Class>::ctor = &node;
            ctor<Class, Args...>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
            return MetaFactory<Class>{};
        }

        template<void(*Func)(Class &), typename... Property>
        static auto dtor(Property &&... property) ENTT_NOEXCEPT {
            static MetaDtorNode node{
                +[](void *instance) {
                    (*Func)(*static_cast<Class *>(instance));
                },
                +[]() {
                    return internal::MetaInfo<Class>::template Dtor<Func>::prop;
                }
            };

            assert(!internal::MetaInfo<Class>::dtor);
            assert(!internal::MetaInfo<Class>::template Dtor<Func>::dtor);
            internal::MetaInfo<Class>::template Dtor<Func>::dtor = &node;
            internal::MetaInfo<Class>::dtor = &node;
            dtor<Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static MetaDataNode node{
                HashedString{str},
                internal::MetaInfo<Class>::data,
                +[]() ENTT_NOEXCEPT {
                    return internal::MetaInfo<std::decay_t<Type>>::type;
                },
                +[]() ENTT_NOEXCEPT {
                    return std::is_const<Type>::value;
                },
                +[](const void *instance) ENTT_NOEXCEPT {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                +[](void *instance, const MetaAny &any) {
                    setter<std::is_const<Type>::value, Type, Member>(instance, any);
                },
                +[](const MetaTypeNode * const type) ENTT_NOEXCEPT {
                    return type == internal::MetaInfo<std::decay_t<Type>>::type;
                },
                +[]() ENTT_NOEXCEPT {
                    return internal::MetaInfo<Class>::template Member<Type, Member>::prop;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo<Class>::data));
            assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
            internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
            internal::MetaInfo<Class>::data = &node;
            member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static MetaFuncNode node{
                HashedString{str},
                internal::MetaInfo<Class>::func,
                +[]() ENTT_NOEXCEPT {
                    return MemberFuncHelper<Type, Member>::size();
                },
                +[]() ENTT_NOEXCEPT {
                    return MemberFuncHelper<Type, Member>::ret();
                },
                +[](typename MetaFuncNode::size_type index) ENTT_NOEXCEPT {
                    return MemberFuncHelper<Type, Member>::arg(index);
                },
                +[](const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                    return MemberFuncHelper<Type, Member>::accept(types);
                },
                +[](const void *instance, const MetaAny *any) {
                    return MemberFuncHelper<Type, Member>::invoke(instance, any);
                },
                +[](void *instance, const MetaAny *any) {
                    return MemberFuncHelper<Type, Member>::invoke(instance, any);
                },
                +[]() ENTT_NOEXCEPT {
                    return internal::MetaInfo<Class>::template Member<Type, Member>::prop;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo<Class>::func));
            assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
            internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
            internal::MetaInfo<Class>::func = &node;
            member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
            return MetaFactory<Class>{};
        }

        template<typename Type, Type *Func, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static MetaFuncNode node{
                HashedString{str},
                internal::MetaInfo<Class>::func,
                +[]() ENTT_NOEXCEPT {
                    return FreeFuncHelper<Type, Func>::size();
                },
                +[]() ENTT_NOEXCEPT {
                    return FreeFuncHelper<Type, Func>::ret();
                },
                +[](typename MetaFuncNode::size_type index) ENTT_NOEXCEPT {
                    return FreeFuncHelper<Type, Func>::arg(index);
                },
                +[](const MetaTypeNode ** const types) ENTT_NOEXCEPT {
                    return FreeFuncHelper<Type, Func>::accept(types);
                },
                +[](const void *instance, const MetaAny *any) {
                    return FreeFuncHelper<Type, Func>::invoke(instance, any);
                },
                +[](void *instance, const MetaAny *any) {
                    return FreeFuncHelper<Type, Func>::invoke(instance, any);
                },
                +[]() ENTT_NOEXCEPT {
                    return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop;
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo<Class>::func));
            assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func));
            internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func = &node;
            internal::MetaInfo<Class>::func = &node;
            func<Type, Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
            return MetaFactory<Class>{};
        }
    };

public:
    template<typename Type>
    using factory_type = MetaFactory<Type>;

    template<typename Type, typename... Property>
    static factory_type<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static MetaTypeNode node{
            HashedString{str},
            chain,
            +[]() ENTT_NOEXCEPT {
                return internal::MetaInfo<Type>::ctor;
            },
            +[]() ENTT_NOEXCEPT {
                return internal::MetaInfo<Type>::dtor;
            },
            +[]() ENTT_NOEXCEPT {
                return internal::MetaInfo<Type>::data;
            },
            +[]() ENTT_NOEXCEPT {
                return internal::MetaInfo<Type>::func;
            },
            +[]() ENTT_NOEXCEPT {
                return internal::MetaInfo<Type>::prop;
            }
        };

        assert(!duplicate(HashedString{str}, chain));
        assert(!internal::MetaInfo<Type>::type);
        internal::MetaInfo<Type>::type = &node;
        chain = &node;
        type<Type>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Type>{};
    }
};


}


#endif // ENTT_META_META_HPP
