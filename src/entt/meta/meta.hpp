#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


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
    const MetaAny &(* const key)() ENTT_NOEXCEPT;
    const MetaAny &(* const value)() ENTT_NOEXCEPT;
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

    virtual MetaTypeNode * node() const ENTT_NOEXCEPT = 0;
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

    MetaTypeNode * node() const ENTT_NOEXCEPT override {
        return MetaInfo::resolve<Type>();
    }

    inline const void * data() const ENTT_NOEXCEPT override {
        return &storage;
    }

    bool operator==(const Holder &other) const ENTT_NOEXCEPT override {
        return node() == other.node() && compare(0, *reinterpret_cast<const Type *>(&storage), *static_cast<const Type *>(other.data()));
    }

private:
    typename std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


struct Utils {
    template<typename Meta, typename Op, typename Node>
    static auto iterate(Op op, const Node *curr) ENTT_NOEXCEPT
    -> decltype(op(std::declval<Meta *>()))
    {
        while(curr) {
            op(curr->meta);
            curr = curr->next;
        }
    }

    template<typename Key>
    static MetaProp * property(Key &&key, const MetaPropNode *curr) {
        MetaProp *prop = nullptr;

        iterate<MetaProp>([&prop, key = std::forward<Key>(key)](auto *curr) {
            prop = (curr->key().template convertible<Key>() && curr->key() == key) ? curr : prop;
        }, curr);

        return prop;
    }

    template<typename Node>
    static auto meta(HashedString name, const Node *curr) {
        while(curr && curr->name != name) {
            curr = curr->next;
        }

        return curr ? curr->meta : nullptr;
    }
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
        return actual ? actual->node()->meta : nullptr;
    }

    template<typename Type>
    bool convertible() const ENTT_NOEXCEPT {
        return internal::MetaInfo::resolve<std::decay_t<Type>>() == actual->node();
    }

    template<typename Type>
    inline const Type & to() const ENTT_NOEXCEPT {
        return *static_cast<const Type *>(actual->data());
    }

    template<typename Type>
    inline Type & to() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const MetaAny *>(this)->to<Type>());
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

    inline const MetaAny & key() const ENTT_NOEXCEPT {
        return node()->key();
    }

    inline const MetaAny & value() const ENTT_NOEXCEPT {
        return node()->value();
    }
};


class MetaCtor {
    virtual internal::MetaCtorNode * node() const ENTT_NOEXCEPT = 0;

public:
    using size_type = typename internal::MetaCtorNode::size_type;

    virtual ~MetaCtor() ENTT_NOEXCEPT = default;

    inline size_type size() const ENTT_NOEXCEPT {
        return node()->size;
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node()->arg(index)->meta : nullptr;
    }

    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
        return sizeof...(Args) == size() ? node()->accept(args.data()) : false;
    }

    template<typename... Args>
    MetaAny invoke(Args &&... args) {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node()->invoke(any.data()) : MetaAny{};
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node()->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node()->prop);
    }
};


class MetaDtor {
    virtual internal::MetaDtorNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaDtor() ENTT_NOEXCEPT = default;

    inline void invoke(void *instance) {
        node()->invoke(instance);
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node()->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node()->prop);
    }
};


class MetaData {
    virtual internal::MetaDataNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaData() ENTT_NOEXCEPT = default;

    inline const char * name() const ENTT_NOEXCEPT {
        const char *str = node()->name;
        return str ? str : "";
    }

    inline bool constant() const ENTT_NOEXCEPT {
        return node()->constant;
    }

    inline MetaType * type() const ENTT_NOEXCEPT {
        return node()->type()->meta;
    }

    template<typename Arg>
    bool accept() const ENTT_NOEXCEPT {
        return node()->accept(internal::MetaInfo::resolve<Arg>());
    }

    template<typename Arg>
    void set(void *instance, Arg &&arg) {
        return accept<Arg>() ? node()->set(instance, MetaAny{std::forward<Arg>(arg)}) : void();
    }

    inline MetaAny get(const void *instance) const ENTT_NOEXCEPT {
        return instance ? node()->get(instance) : MetaAny{};
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node()->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node()->prop);
    }
};


class MetaFunc {
    virtual internal::MetaFuncNode * node() const ENTT_NOEXCEPT = 0;

public:
    using size_type = typename internal::MetaCtorNode::size_type;

    virtual ~MetaFunc() ENTT_NOEXCEPT = default;

    inline const char * name() const ENTT_NOEXCEPT {
        const char *str = node()->name;
        return str ? str : "";
    }

    inline size_type size() const ENTT_NOEXCEPT {
        return node()->size;
    }

    inline MetaType * ret() const ENTT_NOEXCEPT {
        return node()->ret()->meta;
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node()->arg(index)->meta : nullptr;
    }

    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
        return sizeof...(Args) == size() ? node()->accept(args.data()) : false;
    }

    template<typename... Args>
    MetaAny invoke(const void *instance, Args &&... args) const {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return instance && accept<Args...>() ? node()->cinvoke(instance, any.data()) : MetaAny{};
    }

    template<typename... Args>
    MetaAny invoke(void *instance, Args &&... args) {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return instance && accept<Args...>() ? node()->invoke(instance, any.data()) : MetaAny{};
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node()->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node()->prop);
    }
};


class MetaType {
    virtual internal::MetaTypeNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaType() ENTT_NOEXCEPT = default;

    inline const char * name() const ENTT_NOEXCEPT {
        const char *str = node()->name;
        return str ? str : "";
    }

    template<typename Op>
    inline void ctor(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaCtor>(std::move(op), node()->ctor);
    }

    template<typename... Args>
    inline MetaCtor * ctor() const ENTT_NOEXCEPT {
        MetaCtor *meta = nullptr;

        ctor([&meta](MetaCtor *curr) {
            meta = curr->accept<Args...>() ? curr : meta;
        });

        return meta;
    }

    template<typename Op>
    inline void dtor(Op op) const ENTT_NOEXCEPT {
        op(node()->dtor->meta);
    }

    inline MetaDtor * dtor() const ENTT_NOEXCEPT {
        return node()->dtor->meta;
    }

    template<typename Op>
    inline void data(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaData>(std::move(op), node()->data);
    }

    inline MetaData * data(const char *str) const ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, node()->data);
    }

    template<typename Op>
    inline void func(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaFunc>(std::move(op), node()->func);
    }

    inline MetaFunc * func(const char *str) const ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, node()->func);
    }

    template<typename... Args>
    MetaAny construct(Args &&... args) const {
        auto *curr = node()->ctor;

        while(curr && !curr->meta->accept<Args...>()) {
            curr = curr->next;
        }

        return curr ? curr->meta->invoke(std::forward<Args>(args)...) : MetaAny{};
    }

    inline void destroy(void *instance) {
        return instance ? node()->dtor->invoke(instance) : void();
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node()->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node()->prop);
    }
};


namespace internal {


template<typename... Info>
class MetaPropImpl: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::prop<Info...>;
    }
};


template<typename... Info>
class MetaCtorImpl: public MetaCtor {
    internal::MetaCtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::ctor<Info...>;
    }
};


template<typename... Info>
class MetaDtorImpl: public MetaDtor {
    internal::MetaDtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::dtor<Info...>;
    }
};


template<typename... Info>
class MetaDataImpl: public MetaData {
    internal::MetaDataNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::data<Info...>;
    }
};


template<typename... Info>
class MetaFuncImpl: public MetaFunc {
    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::func<Info...>;
    }
};


template<typename... Info>
class MetaTypeImpl: public MetaType {
    internal::MetaTypeNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo::type<Info...>;
    }
};


template<typename Type>
MetaTypeNode * MetaInfo::resolve() ENTT_NOEXCEPT {
    using actual_type = std::decay_t<Type>;

    if(!type<actual_type>) {
        static MetaTypeImpl<actual_type> impl;
        static MetaTypeNode node{&impl, {}, MetaInfo::type<>};
        type<actual_type> = &node;
        type<> = &node;
    }

    return type<std::decay_t<actual_type>>;
}


}


}


#endif // ENTT_META_META_HPP
