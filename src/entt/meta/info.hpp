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
    MetaPropNode *prop{nullptr};
};


struct MetaDtorNode final {
    void(* const invoke)(void *);
    MetaPropNode *prop{nullptr};
};


struct MetaDataNode final {
    const HashedString key;
    MetaDataNode * const next;
    MetaType *(* const type)() ENTT_NOEXCEPT;
    bool(* const constant)() ENTT_NOEXCEPT;
    MetaAny(* const get)(const void *) ENTT_NOEXCEPT;
    void(* const set)(void *, const MetaAny &);
    bool(* const accept)(const MetaType * const) ENTT_NOEXCEPT;
    MetaPropNode *prop{nullptr};
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
    MetaPropNode *prop{nullptr};
};


struct MetaTypeNode final {
    const HashedString key;
    MetaTypeNode * const next;
    MetaType * const meta;
    MetaPropNode *prop{nullptr};
    MetaCtorNode *ctor{nullptr};
    MetaDtorNode *dtor{nullptr};
    MetaDataNode *data{nullptr};
    MetaFuncNode *func{nullptr};
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

    template<std::size_t>
    static MetaPropNode *prop;

    template<typename...>
    struct Ctor {
        static MetaCtorNode *ctor;

        template<std::size_t>
        static MetaPropNode *prop;
    };

    template<void(*)(Class &)>
    struct Dtor {
        static MetaDtorNode *dtor;

        template<std::size_t>
        static MetaPropNode *prop;
    };

    template<typename Type, Type Class:: *>
    struct Member {
        static std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *member;

        template<std::size_t>
        static MetaPropNode *prop;
    };

    template<typename Type, Type *>
    struct FreeFunc {
        static MetaFuncNode *func;

        template<std::size_t>
        static MetaPropNode *prop;
    };
};


template<typename Class>
MetaTypeNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::type = nullptr;


template<typename Class>
template<std::size_t>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::prop = nullptr;


template<typename Class>
template<typename... Args>
MetaCtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Ctor<Args...>::ctor = nullptr;


template<typename Class>
template<typename... Args>
template<std::size_t>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Ctor<Args...>::prop = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
MetaDtorNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Dtor<Func>::dtor = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
template<std::size_t>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Dtor<Func>::prop = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *
MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Member<Type, Member>::member = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
template<std::size_t>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::Member<Type, Member>::prop = nullptr;


template<typename Class>
template<typename Type, Type *Func>
MetaFuncNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::FreeFunc<Type, Func>::func = nullptr;


template<typename Class>
template<typename Type, Type *Func>
template<std::size_t>
MetaPropNode * MetaInfo<Class, std::enable_if_t<std::is_class<Class>::value>>::FreeFunc<Type, Func>::prop = nullptr;


}


}


#endif // ENTT_META_INFO_HPP
