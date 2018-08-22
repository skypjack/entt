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


namespace entt {


struct MetaAny;
class MetaProp;
class MetaCtor;
class MetaDtor;
class MetaData;
class MetaFunc;
class MetaClass;


namespace internal {


struct MetaPropNode final {
    const MetaAny &key;
    const MetaAny &value;
    MetaPropNode * const next;
    MetaProp * const meta;
};


struct MetaCtorNode final {
    MetaCtorNode * const next;
    MetaCtor * const meta;
    MetaPropNode *prop{nullptr};
};


struct MetaDtorNode final {
    MetaDtor * const meta;
    MetaPropNode *prop{nullptr};
};


struct MetaDataNode final {
    const HashedString key;
    MetaDataNode * const next;
    MetaData * const meta;
    MetaPropNode *prop{nullptr};
};


struct MetaFuncNode final {
    const HashedString key;
    MetaFuncNode * const next;
    MetaFunc * const meta;
    MetaPropNode *prop{nullptr};
};


struct MetaClassNode final {
    const HashedString key;
    MetaClassNode * const next;
    MetaClass * const meta;
    MetaPropNode *prop{nullptr};
    MetaCtorNode *ctor{nullptr};
    MetaDtorNode *dtor{nullptr};
    MetaDataNode *data{nullptr};
    MetaFuncNode *func{nullptr};
};


template<typename...>
struct MetaInfo final {
    static MetaClassNode *clazz;
};


template<typename... Type>
MetaClassNode * MetaInfo<Type...>::clazz = nullptr;


template<typename Class>
struct MetaInfo<Class> final {
    static_assert(std::is_class<Class>::value, "!");

    static MetaClassNode *clazz;

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
MetaClassNode * MetaInfo<Class>::clazz = nullptr;


template<typename Class>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::prop = nullptr;


template<typename Class>
template<typename... Args>
MetaCtorNode * MetaInfo<Class>::Ctor<Args...>::ctor = nullptr;


template<typename Class>
template<typename... Args>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::Ctor<Args...>::prop = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
MetaDtorNode * MetaInfo<Class>::Dtor<Func>::dtor = nullptr;


template<typename Class>
template<void(*Func)(Class &)>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::Dtor<Func>::prop = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *
MetaInfo<Class>::Member<Type, Member>::member = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::Member<Type, Member>::prop = nullptr;


template<typename Class>
template<typename Type, Type *Func>
MetaFuncNode * MetaInfo<Class>::FreeFunc<Type, Func>::func = nullptr;


template<typename Class>
template<typename Type, Type *Func>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::FreeFunc<Type, Func>::prop = nullptr;


template<typename Type>
std::enable_if_t<!std::is_class<Type>::value, MetaClass *>
meta() ENTT_NOEXCEPT { return nullptr; }


template<typename Type>
std::enable_if_t<std::is_class<Type>::value, MetaClass *>
meta() ENTT_NOEXCEPT {
    const auto *clazz = internal::MetaInfo<std::decay_t<Type>>::clazz;
    return clazz ? clazz->meta : nullptr;
}


template<typename Meta, typename Key, typename Node>
inline Meta * meta(const Key &key, Node *node) ENTT_NOEXCEPT {
    return node ? (node->key == key ? node->meta : meta<Meta>(key, node->next)) : nullptr;
}


template<typename Op, typename Node>
inline void prop(Op op, Node *curr) ENTT_NOEXCEPT {
    while(curr) {
        op(curr->meta);
        curr = curr->next;
    }
}


struct Holder {
    virtual ~Holder() = default;

    virtual const MetaClass * meta() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;
    virtual bool operator==(const Holder &) const ENTT_NOEXCEPT = 0;

    inline MetaClass * meta() ENTT_NOEXCEPT {
        return const_cast<MetaClass *>(const_cast<const Holder *>(this)->meta());
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

    MetaClass * meta() const ENTT_NOEXCEPT override {
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

    const MetaClass * meta() const ENTT_NOEXCEPT {
        return actual ? actual->meta() : nullptr;
    }

    MetaClass * meta() ENTT_NOEXCEPT {
        return const_cast<MetaClass *>(const_cast<const MetaAny *>(this)->meta());
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
    virtual bool accept(const MetaClass ** const, std::size_t) const ENTT_NOEXCEPT = 0;
    virtual MetaAny execute(const MetaAny * const, std::size_t) const = 0;

public:
    using size_type = std::size_t;

    virtual ~MetaCtor() = default;
    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaClass *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
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
    virtual MetaClass * type() const ENTT_NOEXCEPT = 0;
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
    virtual bool accept(const MetaClass ** const, std::size_t) const ENTT_NOEXCEPT = 0;
    virtual MetaAny execute(const void *, const MetaAny *, std::size_t) const = 0;
    virtual MetaAny execute(void *, const MetaAny *, std::size_t) = 0;

public:
    using size_type = std::size_t;

    virtual ~MetaFunc() = default;
    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * ret() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename Type>
    inline bool ret() const ENTT_NOEXCEPT {
        return ret() == internal::meta<std::decay_t<Type>>();
    }

    template<typename... Args>
    inline bool accept() const ENTT_NOEXCEPT {
        std::array<const MetaClass *, sizeof...(Args)> types{{internal::meta<std::decay_t<Args>>()...}};
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


class MetaClass {
    virtual internal::MetaClassNode * node() const ENTT_NOEXCEPT = 0;

    template<typename Node, typename Op>
    inline void all(Node *curr, Op op) const ENTT_NOEXCEPT {
        while(curr) {
            op(curr->meta);
            curr = curr->next;
        }
    }

public:
    using size_type = std::size_t;

    virtual ~MetaClass() = default;
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
    using ArgType = const MetaClass *;

    static bool types(ArgType<Args>... values) {
        bool res = true;
        using accumulator_type = bool[];
        accumulator_type accumulator = { res, (res = res && values == meta<Args>())... };
        (void)accumulator;
        return res;
    }

    template<std::size_t... Indexes>
    inline static bool types(const MetaClass ** const values, std::index_sequence<Indexes...>) {
        return types(*(values+Indexes)...);
    }

    inline static bool types(const MetaClass ** const values) {
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
class MetaCtorPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, void(*Func)(Class &)>
class MetaDtorPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Dtor<Func>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, Type Class:: *Member>
class MetaMemberPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, Type *Func>
class MetaFunctionPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class>
class MetaClassPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template prop<Index>;
    }
};


template<typename Class, typename... Args>
class MetaCtorType final: public MetaCtor {
    template<std::size_t... Indexes>
    static auto ctor(const MetaAny *args, std::index_sequence<Indexes...>) {
        return MetaAny{Class{(args+Indexes)->value<std::decay_t<Args>>()...}};
    }

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{{meta<std::decay_t<Args>>()...}}[index];
    }

    internal::MetaCtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::ctor;
    }

    bool accept(const MetaClass ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
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

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? arg(index, std::make_index_sequence<sizeof...(Args)>{}) : nullptr;
    }
};


template<typename Class, void(*Func)(Class &)>
class MetaDtorType: public MetaDtor {
    internal::MetaDtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Dtor<Func>::dtor;
    }

public:
    void invoke(void *instance) override {
        (*Func)(*static_cast<Class *>(instance));
    }
};


template<typename Class, typename Type, Type Class:: *Member>
class MetaDataType final: public MetaData {
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
    MetaClass * type() const ENTT_NOEXCEPT override {
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
class MetaFuncBaseType;


template<typename Class, typename Ret, typename... Args>
class MetaFuncBaseType<Class, Ret(Args...)>: public MetaFunc {
    bool accept(const MetaClass ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
        return sz == sizeof...(Args) && Accept<Args...>::types(types);
    }

public:
    size_type size() const ENTT_NOEXCEPT override {
        return sizeof...(Args);
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<std::decay_t<Ret>>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? std::array<MetaClass *, sizeof...(Args)>{{meta<std::decay_t<Args>>()...}}[index] : nullptr;
    }
};


template<typename Class, typename Type, Type Class:: *>
class MetaFuncType;


template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
class MetaFuncType<Class, Ret(Args...), Member>: public MetaFuncBaseType<Class, Ret(Args...)> {
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
class MetaFuncType<Class, Ret(Args...) const, Member>: public MetaFuncBaseType<Class, Ret(Args...)> {
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
struct MetaFreeFuncType;


template<typename Class, typename Instance, typename Ret, typename... Args, Ret(*Func)(Instance &, Args...)>
struct MetaFreeFuncType<Class, Ret(Instance &, Args...), Func>: public MetaFunc {
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

    bool accept(const MetaClass ** const types, std::size_t sz) const ENTT_NOEXCEPT override {
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

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<std::decay_t<Ret>>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < sizeof...(Args) ? std::array<MetaClass *, sizeof...(Args)>{{meta<std::decay_t<Args>>()...}}[index] : nullptr;
    }
};


template<typename Class>
class MetaClassType final: public MetaClass {
    MetaClassNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::clazz;
    }

public:
    void destroy(void *instance) override {
        return internal::MetaInfo<Class>::clazz->dtor
                ? internal::MetaInfo<Class>::clazz->dtor->meta->invoke(instance)
                : static_cast<Class *>(instance)->~Class();
    }
};


}


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


template<typename Class>
class MetaFactory final {
    template<std::size_t Index, typename Property>
    static void clazz(const Property &property) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static internal::MetaClassPropType<Index, Class> meta{};
        static internal::MetaPropNode node{key, value, internal::MetaInfo<Class>::clazz->prop, &meta};
        assert(!internal::meta<MetaProp>(key, internal::MetaInfo<Class>::clazz->prop));
        assert(!internal::MetaInfo<Class>::template prop<Index>);
        internal::MetaInfo<Class>::template prop<Index> = &node;
        internal::MetaInfo<Class>::clazz->prop = &node;
    }

    template<std::size_t... Indexes, typename... Property>
    static void clazz(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (clazz<Indexes>(std::forward<Property>(property)), 0)... };
        (void)accumulator;
    }

    template<std::size_t Index, typename... Args, typename Property>
    static void ctor(const Property &property) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static internal::MetaCtorPropType<Index, Class, Args...> meta{};
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
        static internal::MetaDtorPropType<Index, Class, Func> meta{};
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
        static internal::MetaMemberPropType<Index, Class, Type, Member> meta{};
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
        static internal::MetaFunctionPropType<Index, Class, Type, Func> meta{};
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
        static internal::MetaClassType<Class> meta;
        static internal::MetaClassNode node{HashedString{str}, internal::MetaInfo<>::clazz, &meta};
        assert(!internal::meta<MetaClass>(HashedString{str}, internal::MetaInfo<>::clazz));
        assert(!internal::MetaInfo<Class>::clazz);
        internal::MetaInfo<Class>::clazz = &node;
        internal::MetaInfo<>::clazz = &node;
        clazz(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory{};
    }

    template<typename... Args, typename... Property>
    static MetaFactory ctor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaCtorType<Class, Args...> meta{};
        static internal::MetaCtorNode node{internal::MetaInfo<Class>::clazz->ctor, &meta};
        assert(!internal::MetaInfo<Class>::template Ctor<Args...>::ctor);
        internal::MetaInfo<Class>::template Ctor<Args...>::ctor = &node;
        internal::MetaInfo<Class>::clazz->ctor = &node;
        ctor<Args...>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<void(*Func)(Class &), typename... Property>
    static MetaFactory dtor(Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDtorType<Class, Func> meta{};
        static internal::MetaDtorNode node{&meta};
        assert(!internal::MetaInfo<Class>::clazz->dtor);
        assert(!internal::MetaInfo<Class>::template Dtor<Func>::dtor);
        internal::MetaInfo<Class>::template Dtor<Func>::dtor = &node;
        internal::MetaInfo<Class>::clazz->dtor = &node;
        dtor<Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type Class:: *Member, typename... Property>
    static MetaFactory data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaDataType<Class, Type, Member> meta{};
        static internal::MetaDataNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->data, &meta};
        assert(!internal::meta<MetaData>(HashedString{str}, internal::MetaInfo<Class>::clazz->data));
        assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
        internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
        internal::MetaInfo<Class>::clazz->data = &node;
        member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type Class:: *Member, typename... Property>
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFuncType<Class, Type, Member> meta{};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::clazz->func));
        assert((!internal::MetaInfo<Class>::template Member<Type, Member>::member));
        internal::MetaInfo<Class>::template Member<Type, Member>::member = &node;
        internal::MetaInfo<Class>::clazz->func = &node;
        member<Type, Member>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, Type *Func, typename... Property>
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFreeFuncType<Class, Type, Func> meta{};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::clazz->func));
        assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func));
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func = &node;
        internal::MetaInfo<Class>::clazz->func = &node;
        func<Type, Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }
};


template<typename Class, typename... Property>
inline MetaFactory<Class> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return MetaFactory<Class>::reflect(str, std::forward<Property>(property)...);
}


template<typename Class>
inline MetaClass * meta() ENTT_NOEXCEPT {
    return internal::meta<Class>();
}


inline MetaClass * meta(const char *str) ENTT_NOEXCEPT {
    return internal::meta<MetaClass>(HashedString{str}, internal::MetaInfo<>::clazz);
}


}


#endif // ENTT_META_META_HPP
