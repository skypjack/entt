#ifndef ENTT_META_INFO_HPP
#define ENTT_META_INFO_HPP


#include <cstddef>
#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


struct MetaAny;
struct MetaProp;
class MetaType;


namespace internal {


struct MetaPropNode final {
    const MetaAny &key;
    const MetaAny &value;
    MetaPropNode * const next;
    MetaProp * const meta;
};


struct MetaCtorNode final {
    using size_type = std::size_t;
    MetaCtorNode * const next;
    std::size_t(* const size)() ENTT_NOEXCEPT;
    MetaType *(* const arg)(size_type) ENTT_NOEXCEPT;
    bool (* const accept)(const MetaType ** const) ENTT_NOEXCEPT;
    MetaAny(* const invoke)(const MetaAny * const);
    MetaPropNode *(* const prop)() ENTT_NOEXCEPT;
};


struct MetaDtorNode final {
    void(* const invoke)(void *);
    MetaPropNode *(* const prop)();
};


struct MetaDataNode final {
    const HashedString key;
    MetaDataNode * const next;
    MetaType *(* const type)() ENTT_NOEXCEPT;
    bool(* const constant)() ENTT_NOEXCEPT;
    MetaAny(* const get)(const void *) ENTT_NOEXCEPT;
    void(* const set)(void *, const MetaAny &);
    bool(* const accept)(const MetaType * const) ENTT_NOEXCEPT;
    MetaPropNode *(* const prop)();
};


struct MetaFuncNode final {
    using size_type = std::size_t;
    const HashedString key;
    MetaFuncNode * const next;
    size_type(* const size)() ENTT_NOEXCEPT;
    MetaType *(* const ret)() ENTT_NOEXCEPT;
    MetaType *(* const arg)(size_type) ENTT_NOEXCEPT;
    bool(* const accept)(const MetaType ** const) ENTT_NOEXCEPT;
    MetaAny(* const cinvoke)(const void *, const MetaAny *);
    MetaAny(* const invoke)(void *, const MetaAny *);
    MetaPropNode *(* const prop)();
};


struct MetaTypeNode final {
    const HashedString key;
    MetaTypeNode * const next;
    MetaType * const meta;
    MetaCtorNode *(* const ctor)();
    MetaDtorNode *(* const dtor)();
    MetaDataNode *(* const data)();
    MetaFuncNode *(* const func)();
    MetaPropNode *(* const prop)();
};


template<typename, typename = void>
struct MetaInfo final {
    static MetaTypeNode *type;
};


template<typename Type, typename Enabler>
MetaTypeNode * MetaInfo<Type, Enabler>::type = nullptr;


template<typename Class>
struct MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>> final {
    static MetaTypeNode *type;

    static MetaCtorNode *ctor;
    static MetaDtorNode *dtor;
    static MetaDataNode *data;
    static MetaFuncNode *func;
    static MetaPropNode *prop;

    template<typename...>
    struct Ctor {
        static MetaCtorNode *ctor;
        static MetaPropNode *prop;
    };

    template<void(*)(Class &)>
    struct Dtor {
        static MetaDtorNode *dtor;
        static MetaPropNode *prop;
    };

    template<typename Type, Type Class:: *>
    struct Member {
        static std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *member;
        static MetaPropNode *prop;
    };

    template<typename Type, Type *>
    struct FreeFunc {
        static MetaFuncNode *func;
        static MetaPropNode *prop;
    };
};


template<typename Class>
MetaTypeNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::type = nullptr;


template<typename Class>
MetaCtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::ctor = nullptr;


template<typename Class>
MetaDtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::dtor = nullptr;


template<typename Class>
MetaDataNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::data = nullptr;


template<typename Class>
MetaFuncNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::func = nullptr;


template<typename Class>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::prop = nullptr;


template<typename Class>
template<typename... Args>
MetaCtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Ctor<Args...>::ctor = nullptr;


template<typename Class>
template<typename... Args>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Ctor<Args...>::prop = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
MetaDtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Dtor<Func>::dtor = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Dtor<Func>::prop = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *
MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Member<Type, Member>::member = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Member<Type, Member>::prop = nullptr;


template<typename Class>
template<typename Type, Type *Func>
MetaFuncNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::FreeFunc<Type, Func>::func = nullptr;


template<typename Class>
template<typename Type, Type *Func>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::FreeFunc<Type, Func>::prop = nullptr;


}


}


#endif // ENTT_META_INFO_HPP
