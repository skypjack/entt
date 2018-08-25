#ifndef ENTT_META_INFO_HPP
#define ENTT_META_INFO_HPP


#include <cstddef>
#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


struct MetaAny;
struct MetaTypeNode;


struct MetaPropNode final {
    MetaPropNode * const next;
    MetaAny(* const key)() ENTT_NOEXCEPT;
    MetaAny(* const value)() ENTT_NOEXCEPT;
};


struct MetaCtorNode final {
    using size_type = std::size_t;
    MetaCtorNode * const next;
    MetaPropNode * const prop;
    const size_type size;
    MetaTypeNode *(* const arg)(size_type) ENTT_NOEXCEPT;
    bool(* const accept)(const MetaTypeNode ** const) ENTT_NOEXCEPT;
    MetaAny(* const invoke)(const MetaAny * const);
};


struct MetaDtorNode final {
    MetaPropNode * const prop;
    void(* const invoke)(void *);
};


struct MetaDataNode final {
    const HashedString name;
    MetaDataNode * const next;
    MetaPropNode * const prop;
    const bool constant;
    MetaTypeNode *(* const type)() ENTT_NOEXCEPT;
    void(* const set)(void *, const MetaAny &);
    MetaAny(* const get)(const void *) ENTT_NOEXCEPT;
    bool(* const accept)(const MetaTypeNode * const) ENTT_NOEXCEPT;
};


struct MetaFuncNode final {
    using size_type = std::size_t;
    const HashedString name;
    MetaFuncNode * const next;
    MetaPropNode * const prop;
    const size_type size;
    MetaTypeNode *(* const ret)() ENTT_NOEXCEPT;
    MetaTypeNode *(* const arg)(size_type) ENTT_NOEXCEPT;
    bool(* const accept)(const MetaTypeNode ** const) ENTT_NOEXCEPT;
    MetaAny(* const cinvoke)(const void *, const MetaAny *);
    MetaAny(* const invoke)(void *, const MetaAny *);
};


struct MetaTypeNode final {
    template<typename...>
    static MetaTypeNode *type;

    const HashedString name;
    MetaTypeNode * const next;
    MetaPropNode * const prop;
    MetaCtorNode *ctor;
    MetaDtorNode *dtor;
    MetaDataNode *data;
    MetaFuncNode *func;
};


template<typename...>
MetaTypeNode * MetaTypeNode::type = nullptr;


}


#endif // ENTT_META_INFO_HPP
