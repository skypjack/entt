#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <tuple>
#include <array>
#include <memory>
#include <cassert>
#include <cstddef>
#include <utility>
#include <iterator>
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
struct HolderImpl: public Holder {
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
    HolderImpl(Args &&... args)
        : storage{}
    {
        new (&storage) Type{std::forward<Args>(args)...};
    }

    ~HolderImpl() {
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
        : actual{std::make_unique<internal::HolderImpl<std::decay_t<Type>>>(std::forward<Type>(type))}
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


class MetaCtor {
    virtual internal::MetaCtorNode * node() const ENTT_NOEXCEPT = 0;
    virtual bool accept(const MetaType ** const, std::size_t) const ENTT_NOEXCEPT = 0;
    virtual MetaAny execute(const MetaAny * const, std::size_t) const = 0;

public:
    using size_type = std::size_t;

    virtual ~MetaCtor() = default;
    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaType * arg(size_type) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaType *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
        return accept(types.data(), sizeof...(Args));
    }

    template<typename... Args>
    inline MetaAny invoke(Args &&... args) const {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return execute(params.data(), params.size());
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop);
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};


class MetaDtor {
    virtual internal::MetaDtorNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaDtor() = default;
    virtual void invoke(void *) = 0;

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop);
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};

class MetaData {
    virtual internal::MetaDataNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaData() = default;
    virtual MetaType * type() const ENTT_NOEXCEPT = 0;
    virtual bool constant() const ENTT_NOEXCEPT = 0;
    virtual MetaAny get(const void *) const ENTT_NOEXCEPT = 0;
    virtual void set(void *, const MetaAny &) = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename Type>
    bool accept() const ENTT_NOEXCEPT {
        return type() == internal::meta<Type>();
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop);
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};


class MetaFunc {
    virtual internal::MetaFuncNode * node() const ENTT_NOEXCEPT = 0;
    virtual bool accept(const MetaType ** const, std::size_t) const ENTT_NOEXCEPT = 0;
    virtual MetaAny execute(const void *, const MetaAny *, std::size_t) const = 0;
    virtual MetaAny execute(void *, const MetaAny *, std::size_t) = 0;

public:
    using size_type = std::size_t;

    virtual ~MetaFunc() = default;
    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaType * ret() const ENTT_NOEXCEPT = 0;
    virtual MetaType * arg(size_type) const ENTT_NOEXCEPT = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename Type>
    inline bool ret() const ENTT_NOEXCEPT {
        return ret() == internal::meta<std::decay_t<Type>>();
    }

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaType *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
        return accept(types.data(), sizeof...(Args));
    }

    template<typename... Args>
    inline MetaAny invoke(const void *instance, Args &&... args) const {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return execute(instance, params.data(), params.size());
    }

    template<typename... Args>
    inline MetaAny invoke(void *instance, Args &&... args) {
        const std::array<const MetaAny, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return execute(instance, params.data(), params.size());
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop);
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};


class MetaType {
    virtual internal::MetaTypeNode * node() const ENTT_NOEXCEPT = 0;

    template<typename Node, typename Op>
    inline void all(Node *curr, Op op) const ENTT_NOEXCEPT {
        while(curr) {
            op(curr->meta);
            curr = curr->next;
        }
    }

public:
    using size_type = std::size_t;

    virtual ~MetaType() = default;
    virtual void destroy(void *) = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename... Args>
    MetaCtor * ctor() const ENTT_NOEXCEPT {
        auto *curr = node()->ctor;

        while(curr && !curr->meta->accept<Args...>()) {
            curr = curr->next;
        }

        assert(curr);
        return curr->meta;
    }

    inline MetaDtor * dtor() const ENTT_NOEXCEPT {
        return node()->dtor ? node()->dtor->meta : nullptr;
    }

    inline MetaData * data(const char *str) const ENTT_NOEXCEPT {
        return internal::meta<MetaData>(str, node()->data);
    }

    inline MetaFunc * func(const char *str) const ENTT_NOEXCEPT {
        return internal::meta<MetaFunc>(str, node()->func);
    }

    template<typename Op>
    inline auto ctor(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaCtor *>()), void())
    {
        all(node()->ctor, std::move(op));
    }

    template<typename Op>
    inline auto dtor(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaDtor *>()), void())
    {
        op(node()->dtor);
    }

    template<typename Op>
    inline auto data(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaData *>()), void())
    {
        all(node()->data, std::move(op));
    }

    template<typename Op>
    inline auto func(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaFunc *>()), void())
    {
        all(node()->func, std::move(op));
    }

    template<typename... Args>
    MetaAny construct(Args &&... args) const {
        return ctor<Args...>()->invoke(std::forward<Args>(args)...);
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        return internal::prop(std::move(op), node()->prop);
    }

    inline MetaProp * prop(const MetaAny &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};


namespace internal {


template<typename... Args>
struct Accept {
    template<typename>
    using ArgType = const MetaType *;

    static bool types(ArgType<Args>... values) {
        bool res = true;
        using accumulator_type = bool[];
        accumulator_type accumulator = { res, (res = res && values == meta<Args>())... };
        (void)accumulator;
        return res;
    }

    template<std::size_t... Indexes>
    inline static bool types(const MetaType ** const values, std::index_sequence<Indexes...>) {
        return types(*(values+Indexes)...);
    }

    inline static bool types(const MetaType ** const values) {
        return types(values, std::make_index_sequence<sizeof...(Args)>{});
    }

    template<std::size_t... Indexes>
    inline static bool types(const MetaAny * const values, std::index_sequence<Indexes...>) {
        return types((values+Indexes)->meta()...);
    }

    inline static bool types(const MetaAny * const values) {
        return types(values, std::make_index_sequence<sizeof...(Args)>{});
    }
};


template<std::size_t Index, typename Class, typename... Args>
class MetaCtorPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, void(*Func)(Class &)>
class MetaDtorPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Dtor<Func>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, Type Class:: *Member>
class MetaMemberPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, Type *Func>
class MetaFunctionPropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class>
class MetaTypePropImpl final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template prop<Index>;
    }
};


template<typename Class, typename... Args>
class MetaCtorImpl final: public MetaCtor {
    template<std::size_t... Indexes>
    static auto ctor(const MetaAny *args, std::index_sequence<Indexes...>) {
        return MetaAny{Class{(args+Indexes)->value<std::decay_t<Args>>()...}};
    }

    template<std::size_t... Indexes>
    inline static MetaType * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaType *, sizeof...(Indexes)>{{meta<std::decay_t<Args>>()...}}[index];
    }

    internal::MetaCtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::ctor;
    }

    bool accept(const MetaType ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
        return sz == sizeof...(Args) && Accept<std::decay_t<Args>...>::types(types);
    }

    MetaAny execute(const MetaAny *args, std::size_t sz) const override {
        return sz == sizeof...(Args) && Accept<std::decay_t<Args>...>::types(args)
                  ? ctor(args, std::make_index_sequence<sizeof...(Args)>{}) : MetaAny{};
    }

public:
    size_type size() const ENTT_NOEXCEPT override {
        return sizeof...(Args);
    }

    MetaType * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? arg(index, std::make_index_sequence<sizeof...(Args)>{}) : nullptr;
    }
};


template<typename Class, void(*Func)(Class &)>
class MetaDtorImpl final: public MetaDtor {
    internal::MetaDtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Dtor<Func>::dtor;
    }

public:
    void invoke(void *instance) override {
        (*Func)(*static_cast<Class *>(instance));
    }
};


template<typename Class, typename Type, Type Class:: *Member>
class MetaDataImpl final: public MetaData {
    static_assert(std::is_member_object_pointer<Type Class:: *>::value, "!");

    template<bool Const>
    inline static std::enable_if_t<!Const>
    set(void *instance, const MetaAny &any) {
        assert(internal::meta<std::decay_t<Type>>() == any.meta());
        static_cast<Class *>(instance)->*Member = any.value<std::decay_t<Type>>();
    }

    template<bool Const>
    inline static std::enable_if_t<Const>
    set(void *, const MetaAny &) { assert(false); }

    internal::MetaDataNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::member;
    }

public:
    MetaType * type() const ENTT_NOEXCEPT override {
        return meta<Type>();
    }

    bool constant() const ENTT_NOEXCEPT override {
        return std::is_const<Type>::value;
    }

    MetaAny get(const void *instance) const ENTT_NOEXCEPT override {
        return MetaAny{static_cast<const Class *>(instance)->*Member};
    }

    void set(void *instance, const MetaAny &any) override {
        return set<std::is_const<Type>::value>(instance, any);
    }
};


template<typename Class, typename Type>
class MetaFuncBaseImpl;


template<typename Class, typename Ret, typename... Args>
class MetaFuncBaseImpl<Class, Ret(Args...)>: public MetaFunc {
    bool accept(const MetaType ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
        return sz == sizeof...(Args) && Accept<Args...>::types(types);
    }

public:
    size_type size() const ENTT_NOEXCEPT override {
        return sizeof...(Args);
    }

    MetaType * ret() const ENTT_NOEXCEPT override {
        return meta<std::decay_t<Ret>>();
    }

    MetaType * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? std::array<MetaType *, sizeof...(Args)>{{meta<std::decay_t<Args>>()...}}[index] : nullptr;
    }
};


template<typename Class, typename Type, Type Class:: *>
class MetaFuncImpl;


template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
class MetaFuncImpl<Class, Ret(Args...), Member> final: public MetaFuncBaseImpl<Class, Ret(Args...)> {
    template<std::size_t... Indexes>
    static auto invoke(int, Class *instance, const MetaAny *args, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static MetaAny invoke(char, Class *instance, const MetaAny *args, std::index_sequence<Indexes...>) {
        (instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...);
        return MetaAny{};
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Ret(Args...), Member>::member;
    }

    MetaAny execute(const void *, const MetaAny *, std::size_t) const override {
        assert(false);
        return MetaAny{};
    }

    MetaAny execute(void *instance, const MetaAny *args, std::size_t sz) override {
        return sz == sizeof...(Args) && Accept<Args...>::types(args)
                ? invoke(0, static_cast<Class *>(instance), args, std::make_index_sequence<sizeof...(Args)>{})
                : MetaAny{};
    }
};


template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...) const>
class MetaFuncImpl<Class, Ret(Args...) const, Member> final: public MetaFuncBaseImpl<Class, Ret(Args...)> {
    template<std::size_t... Indexes>
    static auto invoke(int, const Class *instance, const MetaAny *args, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static MetaAny invoke(char, const Class *instance, const MetaAny *args, std::index_sequence<Indexes...>) {
        (instance->*Member)((args+Indexes)->value<std::decay_t<Args>>()...);
        return MetaAny{};
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Ret(Args...) const, Member>::member;
    }

    MetaAny execute(const void *instance, const MetaAny *args, std::size_t sz) const override {
        return sz == sizeof...(Args) && Accept<Args...>::types(args)
                ? invoke(0, static_cast<const Class *>(instance), args, std::make_index_sequence<sizeof...(Args)>{})
                : MetaAny{};
    }

    MetaAny execute(void *instance, const MetaAny *args, std::size_t sz) override {
        return execute(static_cast<const void *>(instance), args, sz);
    }
};


template<typename, typename Type, Type *>
struct MetaFreeFuncImpl;


template<typename Class, typename Instance, typename Ret, typename... Args, Ret(*Func)(Instance &, Args...)>
struct MetaFreeFuncImpl<Class, Ret(Instance &, Args...), Func> final: public MetaFunc {
    template<std::size_t... Indexes>
    static auto invoke(int, Instance *instance, const MetaAny *args, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(*Func)(*instance, (args+Indexes)->value<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(*Func)(*instance, (args+Indexes)->value<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static MetaAny invoke(char, Instance *instance, const MetaAny *args, std::index_sequence<Indexes...>) {
        (*Func)(*instance, (args+Indexes)->value<std::decay_t<Args>>()...);
        return MetaAny{};
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Ret(Instance &, Args...), Func>::func;
    }

    bool accept(const MetaType ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
        return sz == sizeof...(Args) && Accept<Args...>::types(types);
    }

    template<bool Const>
    std::enable_if_t<Const, MetaAny>
    execute(const void *instance, const MetaAny *args, size_type sz) const {
        return sz == sizeof...(Args) && Accept<Args...>::types(args)
                ? invoke(0, static_cast<const Class *>(instance), args, std::make_index_sequence<sizeof...(Args)>{})
                : MetaAny{};
    }

    template<bool Const>
    std::enable_if_t<!Const, MetaAny>
    execute(const void *, const MetaAny *, size_type) const {
        assert(false);
        return MetaAny{};
    }

    MetaAny execute(const void *instance, const MetaAny *args, size_type sz) const override {
        return execute<std::is_const<Instance>::value>(instance, args, sz);
    }

    MetaAny execute(void *instance, const MetaAny *args, size_type sz) override {
        return sz == sizeof...(Args) && Accept<Args...>::types(args)
                ? invoke(0, static_cast<Class *>(instance), args, std::make_index_sequence<sizeof...(Args)>{})
                : MetaAny{};
    }
public:
    size_type size() const ENTT_NOEXCEPT override {
        return sizeof...(Args);
    }

    MetaType * ret() const ENTT_NOEXCEPT override {
        return meta<std::decay_t<Ret>>();
    }

    MetaType * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? std::array<MetaType *, sizeof...(Args)>{{meta<std::decay_t<Args>>()...}}[index] : nullptr;
    }
};


template<typename Class>
class MetaTypeImpl final: public MetaType {
    MetaTypeNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::type;
    }

public:
    void destroy(void *instance) override {
        return internal::MetaInfo<Class>::type->dtor
                ? internal::MetaInfo<Class>::type->dtor->meta->invoke(instance)
                : static_cast<Class *>(instance)->~Class();
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
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::type->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::type->prop));
        assert(!internal::MetaInfo<Class>::template prop<Index>);
        internal::MetaInfo<Class>::template prop<Index> = &node;
        internal::MetaInfo<Class>::type->prop = &node;
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
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop));
        assert(!internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>);
        internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop = &node;
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
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Dtor<Func>::dtor->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Dtor<Func>::dtor->prop));
        assert(!internal::MetaInfo<Class>::template Dtor<Func>::template prop<Index>);
        internal::MetaInfo<Class>::template Dtor<Func>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template Dtor<Func>::dtor->prop = &node;
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
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop));
        assert((!internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>));
        internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template member<Type, Member>->prop = &node;
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
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop));
        assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>));
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop = &node;
    }

    template<typename Type, Type *Func, std::size_t... Indexes, typename... Property>
    static void func(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (func<Indexes, Type, Func>(std::forward<Property>(property)), 0)... };
        (void)accumulator;
    }

public:
    template<typename... Property>
    static MetaFactory reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static_assert(std::is_class<Class>::value, "!");
        static internal::MetaTypeImpl<Class> meta;
        static internal::MetaTypeNode node{HashedString{str}, internal::MetaInfo<MetaFooBar>::type, &meta};
        assert(!internal::meta<MetaType>(HashedString{str}, internal::MetaInfo<MetaFooBar>::type));
        assert(!internal::MetaInfo<Class>::type);
        internal::MetaInfo<Class>::type = &node;
        internal::MetaInfo<MetaFooBar>::type = &node;
        type(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory{};
    }

    template<typename... Args, typename... Property>
    static MetaFactory ctor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaCtorImpl<Class, Args...> meta{};
        static internal::MetaCtorNode node{internal::MetaInfo<Class>::type->ctor, &meta};
        assert(!internal::MetaInfo<Class>::template Ctor<Args...>::ctor);
        internal::MetaInfo<Class>::template Ctor<Args...>::ctor = &node;
        internal::MetaInfo<Class>::type->ctor = &node;
        ctor<Args...>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<void(*Func)(Class &), typename... Property>
    static MetaFactory dtor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDtorImpl<Class, Func> meta{};
        static internal::MetaDtorNode node{&meta};
        assert(!internal::MetaInfo<Class>::type->dtor);
        assert(!internal::MetaInfo<Class>::template Dtor<Func>::dtor);
        internal::MetaInfo<Class>::template Dtor<Func>::dtor = &node;
        internal::MetaInfo<Class>::type->dtor = &node;
        dtor<Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type Class:: *Member, typename... Property>
    static MetaFactory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDataImpl<Class, Type, Member> meta{};
        static internal::MetaDataNode node{HashedString{str}, internal::MetaInfo<Class>::type->data, &meta};
        assert(!internal::meta<MetaData>(HashedString{str}, internal::MetaInfo<Class>::type->data));
        assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
        internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
        internal::MetaInfo<Class>::type->data = &node;
        member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type Class:: *Member, typename... Property>
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFuncImpl<Class, Type, Member> meta{};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::type->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::type->func));
        assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
        internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
        internal::MetaInfo<Class>::type->func = &node;
        member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type *Func, typename... Property>
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFreeFuncImpl<Class, Type, Func> meta{};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::type->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::type->func));
        assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func));
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func = &node;
        internal::MetaInfo<Class>::type->func = &node;
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
