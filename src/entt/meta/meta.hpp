#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <tuple>
#include <array>
#include <memory>
#include <cassert>
#include <cstddef>
#include <utility>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "info.hpp"


namespace entt {


namespace internal {


template<typename Type>
MetaType * meta() ENTT_NOEXCEPT {
    const auto *type = internal::MetaInfo<std::decay_t<Type>>::type;
    return type ? type->meta : nullptr;
}


template<typename Meta, typename Key, typename Node>
inline Meta * meta(const Key &key, Node *node) ENTT_NOEXCEPT {
    return node ? (node->key == key ? node->meta : meta<Meta>(key, node->next)) : nullptr;
}


template<typename Op, typename Node>
inline void prop(Op op, Node *curr) ENTT_NOEXCEPT {
    for(; curr; curr = curr->next) {
        op(curr->meta);
    }
}


struct Holder {
    virtual ~Holder() = default;

    virtual const MetaType * meta() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;
    virtual bool operator==(const Holder &) const ENTT_NOEXCEPT = 0;

    inline MetaType * meta() ENTT_NOEXCEPT {
        return const_cast<MetaType *>(const_cast<const Holder *>(this)->meta());
    }

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
        return entt::internal::meta<Type>();
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

    const MetaType * meta() const ENTT_NOEXCEPT {
        return actual ? actual->meta() : nullptr;
    }

    MetaType * meta() ENTT_NOEXCEPT {
        return const_cast<MetaType *>(const_cast<const MetaAny *>(this)->meta());
    }

    template<typename Type>
    inline const Type & value() const ENTT_NOEXCEPT {
        return *static_cast<const Type *>(actual->data());
    }

    template<typename Type>
    inline Type & value() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const MetaAny *>(this)->value<Type>());
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
        return actual == other.actual || (actual && other.actual && *actual == *other.actual);
    }

private:
    std::unique_ptr<internal::Holder> actual;
};


class MetaProp {
    virtual internal::MetaPropNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaProp() ENTT_NOEXCEPT = default;

    inline const MetaAny & key() const ENTT_NOEXCEPT {
        return node()->key;
    }

    inline const MetaAny & value() const ENTT_NOEXCEPT {
        return node()->value;
    }
};


struct MetaCtor {
    using size_type = typename internal::MetaCtorNode::size_type;

    inline MetaCtor(internal::MetaCtorNode *node)
        : node{node}
    {}

    inline size_type size() const ENTT_NOEXCEPT {
        return node->size();
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index) : nullptr;
    }

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaType *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
        return sizeof...(Args) == size() && node->accept(types.data());
    }

    template<typename... Args>
    inline MetaAny invoke(Args &&... args) const {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(params.data()) : MetaAny{};
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node->prop());
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node->prop());
    }

    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

private:
    internal::MetaCtorNode *node;
};


struct MetaDtor {
    inline MetaDtor(internal::MetaDtorNode *node)
        : node{node}
    {}

    inline void invoke(void *instance) {
        node->invoke(instance);
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node->prop());
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node->prop());
    }

    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

private:
    internal::MetaDtorNode *node;
};


struct MetaData {
    inline MetaData(internal::MetaDataNode *node)
        : node{node}
    {}

    inline MetaType * type() const ENTT_NOEXCEPT {
        return node->type();
    }

    inline bool constant() const ENTT_NOEXCEPT {
        return node->constant();
    }

    inline MetaAny get(const void *instance) const ENTT_NOEXCEPT {
        return node->get(instance);
    }

    template<typename Type>
    inline void set(void *instance, Type &&type) {
        if(accept<Type>()) {
            node->set(instance, MetaAny{std::forward<Type>(type)});
        }
    }

    inline const char * name() const ENTT_NOEXCEPT {
        return node->key;
    }

    template<typename Type>
    inline bool accept() const ENTT_NOEXCEPT {
        return node->accept(internal::meta<Type>());
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node->prop());
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node->prop());
    }

    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

private:
    internal::MetaDataNode *node;
};


struct MetaFunc {
    using size_type = typename internal::MetaFuncNode::size_type;

    inline MetaFunc(internal::MetaFuncNode *node)
        : node{node}
    {}

    inline size_type size() const ENTT_NOEXCEPT {
        return node->size();
    }

    inline MetaType * ret() const ENTT_NOEXCEPT {
        return node->ret();
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index) : nullptr;
    }

    inline const char * name() const ENTT_NOEXCEPT {
        return node->key;
    }

    template<typename Type>
    inline bool ret() const ENTT_NOEXCEPT {
        return ret() == internal::meta<std::decay_t<Type>>();
    }

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaType *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
        return sizeof...(Args) == size() && node->accept(types.data());
    }

    template<typename... Args>
    inline MetaAny invoke(const void *instance, Args &&... args) const {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->cinvoke(instance, params.data()) : MetaAny{};
    }

    template<typename... Args>
    inline MetaAny invoke(void *instance, Args &&... args) {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(instance, params.data()) : MetaAny{};
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node->prop());
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node->prop());
    }

    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

private:
    internal::MetaFuncNode *node;
};


class MetaType {
    virtual internal::MetaTypeNode * node() const ENTT_NOEXCEPT = 0;

public:
    using size_type = std::size_t;

    virtual ~MetaType() = default;
    virtual void destroy(void *) = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename... Args>
    MetaCtor ctor() const ENTT_NOEXCEPT {
        std::array<const MetaType *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
        auto *curr = node()->ctor();

        while(curr && curr->size() != types.size() && !curr->accept(types.data())) {
            curr = curr->next;
        }

        assert(curr);
        return MetaCtor{curr};
    }

    inline MetaDtor dtor() const ENTT_NOEXCEPT {
        return MetaDtor{node()->dtor()};
    }

    inline MetaData data(const char *str) const ENTT_NOEXCEPT {
        auto *curr = node()->data();
        for(; curr && curr->key != str; curr = curr->next);
        return MetaData{curr};
    }

    inline MetaFunc func(const char *str) const ENTT_NOEXCEPT {
        auto *curr = node()->func();
        for(; curr && curr->key != str; curr = curr->next);
        return MetaFunc{curr};
    }

    template<typename Op>
    inline auto ctor(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaCtor>()), void())
    {
        auto *curr = node()->ctor();

        while(curr) {
            op(MetaCtor{curr});
            curr = curr->next;
        }
    }

    template<typename Op>
    inline auto dtor(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaDtor>()), void())
    {
        op(MetaDtor{node()->dtor()});
    }

    template<typename Op>
    inline auto data(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaData>()), void())
    {
        auto *curr = node()->data();

        while(curr) {
            op(MetaData{curr});
            curr = curr->next;
        }
    }

    template<typename Op>
    inline auto func(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaFunc>()), void())
    {
        auto *curr = node()->func();

        while(curr) {
            op(MetaFunc{curr});
            curr = curr->next;
        }
    }

    template<typename... Args>
    MetaAny construct(Args &&... args) const {
        return ctor<Args...>().invoke(std::forward<Args>(args)...);
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop());
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop());
    }
};


namespace internal {


template<std::size_t Index, typename Class, typename... Args>
class MetaCtorPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::prop;
    }
};


template<std::size_t Index, typename Class, void(*Func)(Class &)>
class MetaDtorPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Dtor<Func>::prop;
    }
};


template<std::size_t Index, typename Class, typename Type, Type Class:: *Member>
class MetaMemberPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::prop;
    }
};


template<std::size_t Index, typename Class, typename Type, Type *Func>
class MetaFunctionPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop;
    }
};


template<std::size_t Index, typename Type>
class MetaTypePropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Type>::prop;
    }
};


template<typename Type>
class MetaTypeImpl final: public MetaType {
    MetaTypeNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Type>::type;
    }

public:
    void destroy(void *instance) override {
        return internal::MetaInfo<Type>::type->dtor()
                ? internal::MetaInfo<Type>::type->dtor()->invoke(instance)
                : static_cast<Type *>(instance)->~Type();
    }
};


}


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


struct MetaFooBar {};


template<typename Class>
class MetaFactory final {
    template<std::size_t Index, typename Property>
    static void type(const Property &property) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static internal::MetaTypePropImpl<Index, Class> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::prop));
        internal::MetaInfo<Class>::prop = &node;
    }

    template<std::size_t... Indexes, typename... Property>
    static void type(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (type<Indexes>(std::forward<Property>(property)), 0)... };
        (void)accumulator;
    }

    template<std::size_t Index, typename... Args, typename Property>
    static void ctor(const Property &property) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static internal::MetaCtorPropImpl<Index, Class, Args...> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Ctor<Args...>::prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Ctor<Args...>::prop));
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
        static internal::MetaDtorPropImpl<Index, Class, Func> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Dtor<Func>::prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Dtor<Func>::prop));
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
        static internal::MetaMemberPropImpl<Index, Class, Type, Member> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Member<Type, Member>::prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Member<Type, Member>::prop));
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
        static internal::MetaFunctionPropImpl<Index, Class, Type, Func> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::prop));
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
        return MetaAny{Class{(any+Indexes)->value<std::decay_t<Args>>()...}};
    }

    template<bool Const, typename Type, Type Class:: *Member>
    static std::enable_if_t<Const>
    setter(void *, const MetaAny &) {
        assert(false);
    }

    template<bool Const, typename Type, Type Class:: *Member>
    static std::enable_if_t<!Const>
    setter(void *instance, const MetaAny &any) {
        static_cast<Class *>(instance)->*Member = any.value<std::decay_t<Type>>();
    }

    template<typename Type, Type Class:: *>
    struct MemberFuncHelper;

    template<typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
    struct MemberFuncHelper<Ret(Args...), Member> {
        static typename internal::MetaFuncNode::size_type size() ENTT_NOEXCEPT {
            return sizeof...(Args);
        }

        static auto ret() ENTT_NOEXCEPT {
            return internal::meta<std::decay_t<Ret>>();
        }

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<MetaType *, sizeof...(Args)>{{internal::meta<std::decay_t<Args>>()...}}[index];
        }

        static auto accept(const MetaType ** const types) ENTT_NOEXCEPT {
            std::array<MetaType *, sizeof...(Args)> args{{internal::meta<std::decay_t<Args>>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }

        template<std::size_t... Indexes>
        static auto invoke(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto invoke(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...);
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
    struct MemberFuncHelper<Ret(Args...) const, Member> {
        static typename internal::MetaFuncNode::size_type size() ENTT_NOEXCEPT {
            return sizeof...(Args);
        }

        static auto ret() ENTT_NOEXCEPT {
            return internal::meta<std::decay_t<Ret>>();
        }

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<MetaType *, sizeof...(Args)>{{internal::meta<std::decay_t<Args>>()...}}[index];
        }

        static auto accept(const MetaType ** const types) ENTT_NOEXCEPT {
            std::array<MetaType *, sizeof...(Args)> args{{internal::meta<std::decay_t<Args>>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }

        template<std::size_t... Indexes>
        static auto invoke(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto invoke(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<const Class *>(instance)->*Member)((any+Indexes)->value<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        static auto invoke(const void *instance, const MetaAny *any) {
            return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Type, Type *>
    struct FreeFuncHelper;

    template<typename Ret, typename... Args, Ret(*Func)(Class &, Args...)>
    struct FreeFuncHelper<Ret(Class &, Args...), Func> {
        static typename internal::MetaFuncNode::size_type size() ENTT_NOEXCEPT {
            return sizeof...(Args);
        }

        static auto ret() ENTT_NOEXCEPT {
            return internal::meta<std::decay_t<Ret>>();
        }

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<MetaType *, sizeof...(Args)>{{internal::meta<std::decay_t<Args>>()...}}[index];
        }

        static auto accept(const MetaType ** const types) ENTT_NOEXCEPT {
            std::array<MetaType *, sizeof...(Args)> args{{internal::meta<std::decay_t<Args>>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }

        template<std::size_t... Indexes>
        static auto invoke(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto invoke(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...);
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
    struct FreeFuncHelper<Ret(const Class &, Args...), Func> {
        static typename internal::MetaFuncNode::size_type size() ENTT_NOEXCEPT {
            return sizeof...(Args);
        }

        static auto ret() ENTT_NOEXCEPT {
            return internal::meta<std::decay_t<Ret>>();
        }

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<MetaType *, sizeof...(Args)>{{internal::meta<std::decay_t<Args>>()...}}[index];
        }

        static auto accept(const MetaType ** const types) ENTT_NOEXCEPT {
            std::array<MetaType *, sizeof...(Args)> args{{internal::meta<std::decay_t<Args>>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }

        template<std::size_t... Indexes>
        static auto invoke(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto invoke(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<const Class *>(instance), (any+Indexes)->value<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        static auto invoke(const void *instance, const MetaAny *any) {
            return invoke(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Key, typename Node>
    inline static bool duplicate(const Key &key, Node *node) ENTT_NOEXCEPT {
        return node ? node->key == key || duplicate(key, node->next) : false;
    }

public:
    template<typename... Property>
    static MetaFactory reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static_assert(std::is_class<Class>::value, "!");
        static internal::MetaTypeImpl<Class> meta;

        static internal::MetaTypeNode node{
            HashedString{str},
            internal::MetaInfo<MetaFooBar>::type,
            &meta,
            +[]() {
                return internal::MetaInfo<Class>::ctor;
            },
            +[]() {
                return internal::MetaInfo<Class>::dtor;
            },
            +[]() {
                return internal::MetaInfo<Class>::data;
            },
            +[]() {
                return internal::MetaInfo<Class>::func;
            },
            +[]() {
                return internal::MetaInfo<Class>::prop;
            }
        };

        assert(!internal::meta<MetaType>(HashedString{str}, internal::MetaInfo<MetaFooBar>::type));
        assert(!internal::MetaInfo<Class>::type);
        internal::MetaInfo<Class>::type = &node;
        internal::MetaInfo<MetaFooBar>::type = &node;
        type(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory{};
    }

    template<typename... Args, typename... Property>
    static MetaFactory ctor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaCtorNode node{
            internal::MetaInfo<Class>::ctor,
            +[]() ENTT_NOEXCEPT {
                return sizeof...(Args);
            },
            +[](typename internal::MetaCtorNode::size_type index) ENTT_NOEXCEPT {
                return std::array<MetaType *, sizeof...(Args)>{{internal::meta<std::decay_t<Args>>()...}}[index];
            },
            +[](const MetaType ** const types) ENTT_NOEXCEPT {
                std::array<MetaType *, sizeof...(Args)> args{{internal::meta<std::decay_t<Args>>()...}};
                return std::equal(args.cbegin(), args.cend(), types);
            },
            +[](const MetaAny * const any) {
                return constructor<Args...>(any, std::make_index_sequence<sizeof...(Args)>{});
            },
            +[]() {
                return internal::MetaInfo<Class>::template Ctor<Args...>::prop;
            }
        };

        assert(!internal::MetaInfo<Class>::template Ctor<Args...>::ctor);
        internal::MetaInfo<Class>::template Ctor<Args...>::ctor = &node;
        internal::MetaInfo<Class>::ctor = &node;
        ctor<Args...>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<void(*Func)(Class &), typename... Property>
    static MetaFactory dtor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDtorNode node{
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
    static MetaFactory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDataNode node{
            HashedString{str},
            internal::MetaInfo<Class>::data,
            +[]() ENTT_NOEXCEPT {
                return internal::meta<std::decay_t<Type>>();
            },
            +[]() ENTT_NOEXCEPT {
                return std::is_const<Type>::value;
            },
            +[](const void *instance) {
                return MetaAny{static_cast<const Class *>(instance)->*Member};
            },
            +[](void *instance, const MetaAny &any) {
                setter<std::is_const<Type>::value, Type, Member>(instance, any);
            },
            +[](const MetaType * const type) ENTT_NOEXCEPT {
                return type == internal::meta<std::decay_t<Type>>();
            },
            +[]() {
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
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaFuncNode node{
            HashedString{str},
            internal::MetaInfo<Class>::func,
            +[]() {
                return MemberFuncHelper<Type, Member>::size();
            },
            +[]() {
                return MemberFuncHelper<Type, Member>::ret();
            },
            +[](typename internal::MetaFuncNode::size_type index) {
                return MemberFuncHelper<Type, Member>::arg(index);
            },
            +[](const MetaType ** const types) {
                return MemberFuncHelper<Type, Member>::accept(types);
            },
            +[](const void *instance, const MetaAny *any) {
                return MemberFuncHelper<Type, Member>::invoke(instance, any);
            },
            +[](void *instance, const MetaAny *any) {
                return MemberFuncHelper<Type, Member>::invoke(instance, any);
            },
            +[]() {
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
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaFuncNode node{
            HashedString{str},
            internal::MetaInfo<Class>::func,
            +[]() {
                return FreeFuncHelper<Type, Func>::size();
            },
            +[]() {
                return FreeFuncHelper<Type, Func>::ret();
            },
            +[](typename internal::MetaFuncNode::size_type index) {
                return FreeFuncHelper<Type, Func>::arg(index);
            },
            +[](const MetaType ** const types) {
                return FreeFuncHelper<Type, Func>::accept(types);
            },
            +[](const void *instance, const MetaAny *any) {
                return FreeFuncHelper<Type, Func>::invoke(instance, any);
            },
            +[](void *instance, const MetaAny *any) {
                return FreeFuncHelper<Type, Func>::invoke(instance, any);
            },
            +[]() {
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


template<typename Class, typename... Property>
inline MetaFactory<Class> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return MetaFactory<Class>::reflect(str, std::forward<Property>(property)...);
}


template<typename Class>
inline MetaType * meta() ENTT_NOEXCEPT {
    return internal::meta<Class>();
}


inline MetaType * meta(const char *str) ENTT_NOEXCEPT {
    return internal::meta<MetaType>(HashedString{str}, internal::MetaInfo<MetaFooBar>::type);
}


}


#endif // ENTT_META_META_HPP
