#ifndef ENTT_META_INFO_HPP
#define ENTT_META_INFO_HPP


#include <memory>
#include <cstddef>
#include <utility>
#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


struct MetaAny;
class MetaProp;
class MetaCtor;
class MetaDtor;
class MetaData;
class MetaFunc;
class MetaType;


namespace internal {


struct MetaPropNode;
struct MetaCtorNode;
struct MetaDtorNode;
struct MetaDataNode;
struct MetaFuncNode;
struct MetaTypeNode;


struct MetaInfo {
    template<typename...>
    static MetaTypeNode *type;

    template<typename...>
    static MetaCtorNode *ctor;

    template<typename...>
    static MetaDtorNode *dtor;

    template<typename...>
    static MetaDataNode *data;

    template<typename...>
    static MetaFuncNode *func;

    template<typename...>
    static MetaPropNode *prop;

    template<typename Type>
    static internal::MetaTypeNode * resolve() ENTT_NOEXCEPT;
};


template<typename...>
MetaTypeNode * MetaInfo::type = nullptr;


template<typename...>
MetaCtorNode * MetaInfo::ctor = nullptr;


template<typename...>
MetaDtorNode * MetaInfo::dtor = nullptr;


template<typename...>
MetaDataNode * MetaInfo::data = nullptr;


template<typename...>
MetaFuncNode * MetaInfo::func = nullptr;


template<typename...>
MetaPropNode * MetaInfo::prop = nullptr;


struct MetaPropNode final {
    MetaProp * const meta;
    MetaPropNode * const next;
    MetaAny(* const key)() ENTT_NOEXCEPT;
    MetaAny(* const value)() ENTT_NOEXCEPT;
};


struct MetaCtorNode final {
    using size_type = std::size_t;
    MetaCtor * const meta;
    MetaCtorNode * const next;
    MetaPropNode * const prop;
    const size_type size;
    MetaTypeNode *(* const arg)(size_type) ENTT_NOEXCEPT;
    bool(* const accept)(const MetaTypeNode ** const) ENTT_NOEXCEPT;
    MetaAny(* const invoke)(const MetaAny * const);
};


struct MetaDtorNode final {
    MetaDtor * const meta;
    MetaPropNode * const prop;
    void(* const invoke)(void *);
};


struct MetaDataNode final {
    MetaData * const meta;
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
    MetaFunc * const meta;
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
    MetaType * const meta;
    const HashedString name;
    MetaTypeNode * const next;
    MetaPropNode * const prop;
    MetaCtorNode *ctor;
    MetaDtorNode *dtor;
    MetaDataNode *data;
    MetaFuncNode *func;
};


struct Holder {
    virtual ~Holder() = default;

    virtual MetaType * meta() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;
    virtual bool operator==(const Holder &) const ENTT_NOEXCEPT = 0;

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const Holder *>(this)->data());
    }
};


template<typename Type>
struct HolderType: public Holder {
    template<typename Object>
    static auto compare(int, const Object &lhs, const Object &rhs)
    -> decltype(lhs == rhs, bool{})
    {
        return lhs == rhs;
    }

    template<typename Object>
    static bool compare(char, const Object &, const Object &) {
        return false;
    }

public:
    template<typename... Args>
    HolderType(Args &&... args)
        : storage{}
    {
        new (&storage) Type{std::forward<Args>(args)...};
    }

    ~HolderType() {
        reinterpret_cast<Type *>(&storage)->~Type();
    }

    MetaType * meta() const ENTT_NOEXCEPT override {
        return MetaInfo::resolve<Type>()->meta;
    }

    inline const void * data() const ENTT_NOEXCEPT override {
        return &storage;
    }

    bool operator==(const Holder &other) const ENTT_NOEXCEPT override {
        return meta() == other.meta() && compare(0, *reinterpret_cast<const Type *>(&storage), *static_cast<const Type *>(other.data()));
    }

private:
    typename std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


}


struct MetaAny final {
    MetaAny() ENTT_NOEXCEPT = default;

    template<typename Type>
    MetaAny(Type &&type)
        : actual{std::make_unique<internal::HolderType<std::decay_t<Type>>>(std::forward<Type>(type))}
    {}

    MetaAny(const MetaAny &) = delete;
    MetaAny(MetaAny &&) = default;

    MetaAny & operator=(const MetaAny &other) = delete;
    MetaAny & operator=(MetaAny &&) = default;

    inline bool valid() const ENTT_NOEXCEPT {
        return *this;
    }

    MetaType * meta() const ENTT_NOEXCEPT {
        return actual ? actual->meta() : nullptr;
    }

    template<typename Type>
    inline const Type & get() const ENTT_NOEXCEPT {
        return *static_cast<const Type *>(actual->data());
    }

    template<typename Type>
    inline Type & get() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const MetaAny *>(this)->get<Type>());
    }

    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        return actual ? static_cast<const Type *>(actual->data()) : nullptr;
    }

    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const MetaAny *>(this)->data<Type>());
    }

    inline const void * data() const ENTT_NOEXCEPT {
        return *this;
    }

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline operator const void *() const ENTT_NOEXCEPT {
        return actual ? actual->data() : nullptr;
    }

    inline operator void *() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(actual);
    }

    inline bool operator==(const MetaAny &other) const ENTT_NOEXCEPT {
        return actual == other.actual || (meta() == other.meta() && actual && other.actual && *actual == *other.actual);
    }

private:
    std::unique_ptr<internal::Holder> actual;
};


class MetaProp {
    virtual internal::MetaPropNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaProp() ENTT_NOEXCEPT = default;

    inline MetaAny key() const ENTT_NOEXCEPT {
        return node()->key();
    }

    inline MetaAny value() const ENTT_NOEXCEPT {
        return node()->value();
    }
};


class MetaCtor {
    // TODO
};


class MetaDtor {
    virtual internal::MetaDtorNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaDtor() ENTT_NOEXCEPT = default;

    inline void invoke(void *instance) {
        node()->invoke(instance);
    }

    template<typename Op>
    inline auto properties(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()))
    {
        for(auto *curr = node()->prop; curr && (op(curr->meta), true); curr = curr->next);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) {
        const MetaAny any{std::forward<Key>(key)};
        auto curr = node()->prop;
        for(; curr && curr->key() != any; curr = curr->next);
        return curr ? curr->meta : nullptr;
    }
};


class MetaData {
    // TODO
};


class MetaFunc {
    // TODO
};


class MetaType {
    // TODO
};


namespace internal {


template<typename... Info>
class MetaPropImpl: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT {
        return internal::MetaInfo::prop<Info...>;
    }
};


template<typename... Info>
class MetaCtorImpl: public MetaCtor {
    // TODO
};


template<typename... Info>
class MetaDtorImpl: public MetaDtor {
    internal::MetaDtorNode * node() const ENTT_NOEXCEPT {
        return internal::MetaInfo::dtor<Info...>;
    }
};


template<typename... Info>
class MetaDataImpl: public MetaData {
    // TODO
};


template<typename... Info>
class MetaFuncImpl: public MetaFunc {
    // TODO
};


template<typename... Info>
class MetaTypeImpl: public MetaType {
    // TODO
};


template<typename Type>
MetaTypeNode * MetaInfo::resolve() ENTT_NOEXCEPT {
    if(!type<std::decay_t<Type>>) {
        static MetaTypeImpl<Type> impl;
        static MetaTypeNode node{&impl, {}, MetaInfo::type<>};
        MetaInfo::type<Type> = &node;
        MetaInfo::type<> = &node;
    }

    return MetaInfo::type<std::decay_t<Type>>;
}


}


}


#endif // ENTT_META_INFO_HPP
