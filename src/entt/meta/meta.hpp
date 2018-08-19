#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <tuple>
#include <array>
#include <cassert>
#include <cstddef>
#include <utility>
#include <iterator>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "any.hpp"


namespace entt {


class MetaProp;
class MetaCtor;
struct MetaData;
class MetaFunc;
class MetaClass;


namespace internal {


struct MetaPropNode final {
    const Any key;
    const Any value;
    MetaPropNode * const next;
    MetaProp * const meta;
};


struct MetaCtorNode final {
    const HashedString key;
    MetaCtorNode * const next;
    MetaCtor * const meta;
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
    MetaDataNode *data{nullptr};
    MetaFuncNode *func{nullptr};
};


template<typename>
struct FuncType;

template<typename Ret, typename... Args>
struct FuncType<Ret(Args...)> {
    static constexpr auto size = sizeof...(Args);
    using args_type = std::tuple<Args...>;
    using return_type = Ret;
};

template<typename Ret, typename... Args>
struct FuncType<Ret(Args...) const>: FuncType<Ret(Args...)> {};


template<typename, typename>
struct ExtendedFunc;

template<typename Class, typename Ret, typename... Args>
struct ExtendedFunc<Class, Ret(Args...)> {
    using type = Ret(const Class &, Args...);
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

    template<typename Type, Type Class:: *>
    struct Member {
        static std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *member;

        template<std::size_t>
        static MetaPropNode *prop;
    };

    template<typename Type, typename ExtendedFunc<Class, Type>::type *>
    struct FreeFunc {
        static MetaFuncNode *func;

        template<std::size_t>
        static MetaPropNode *prop;
    };

    template<typename Func>
    struct Functor {
        static MetaFuncNode * functor;

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
template<typename Type, Type Class:: *Member>
std::conditional_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFuncNode, MetaDataNode> *
MetaInfo<Class>::Member<Type, Member>::member = nullptr;


template<typename Class>
template<typename Type, Type Class:: *Member>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::Member<Type, Member>::prop = nullptr;


template<typename Class>
template<typename Type, typename ExtendedFunc<Class, Type>::type *Func>
MetaFuncNode * MetaInfo<Class>::FreeFunc<Type, Func>::func = nullptr;


template<typename Class>
template<typename Type, typename ExtendedFunc<Class, Type>::type *Func>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::FreeFunc<Type, Func>::prop = nullptr;


template<typename Class>
template<typename Func>
MetaFuncNode * MetaInfo<Class>::Functor<Func>::functor = nullptr;


template<typename Class>
template<typename Func>
template<std::size_t>
MetaPropNode * MetaInfo<Class>::Functor<Func>::prop = nullptr;


template<typename... Args>
struct Accept {
    template<typename>
    using ArgType = Any::any_type;

    inline static bool types(ArgType<Args>... values) {
        bool res = true;
        using accumulator_type = bool[];
        accumulator_type accumulator = { res, (res = res && values == Any::type<Args>())... };
        (void)accumulator;
        return res;
    }

    template<std::size_t... Indexes>
    inline static bool types(Any::any_type * const values, std::index_sequence<Indexes...>) {
        return types(*(values+Indexes)...);
    }

    inline static bool types(Any::any_type * const values) {
        return types(values, std::make_index_sequence<sizeof...(Args)>{});
    }

    template<std::size_t... Indexes>
    inline static bool types(const Any * const values, std::index_sequence<Indexes...>) {
        return types((values+Indexes)->type()...);
    }

    inline static bool types(const Any * const values) {
        return types(values, std::make_index_sequence<sizeof...(Args)>{});
    }
};


template<typename... Args>
struct Accept<std::tuple<Args...>>: Accept<Args...> {};


template<typename Type>
std::enable_if_t<!std::is_class<Type>::value, MetaClass *>
meta() ENTT_NOEXCEPT { return nullptr; }


template<typename Type>
std::enable_if_t<std::is_class<Type>::value, MetaClass *>
meta() ENTT_NOEXCEPT { return internal::MetaInfo<std::decay_t<Type>>::clazz->meta; }


template<typename Meta, typename Key, typename Node>
inline Meta * meta(const Key &key, Node *node) ENTT_NOEXCEPT {
    return node ? (node->key == key ? node->meta : meta<Meta>(key, node->next)) : nullptr;
}


template<typename Node>
struct MetaBase {
    virtual ~MetaBase() = default;
    virtual Node * node() const ENTT_NOEXCEPT = 0;

    inline const char * name() const ENTT_NOEXCEPT {
        return node()->key;
    }

    template<typename Op>
    inline auto prop(Op op) const ENTT_NOEXCEPT
    -> decltype(op(std::declval<MetaProp *>()), void())
    {
        auto *curr = node()->prop;

        while(curr) {
            op(curr->meta);
            curr = curr->next;
        }
    }

    inline MetaProp * prop(const Any &key) const ENTT_NOEXCEPT {
        return internal::meta<MetaProp>(key, node()->prop);
    }
};


}


class MetaProp {
    virtual internal::MetaPropNode * node() const ENTT_NOEXCEPT = 0;

public:
    virtual ~MetaProp() ENTT_NOEXCEPT = default;

    inline const Any & key() const ENTT_NOEXCEPT {
        return node()->key;
    }

    const Any & value() const ENTT_NOEXCEPT {
        return node()->value;
    }
};


class MetaCtor: public internal::MetaBase<internal::MetaCtorNode> {
    virtual bool accept(Any::any_type * const, std::size_t) const = 0;
    virtual Any execute(const Any * const, std::size_t) const = 0;

public:
    using size_type = std::size_t;

    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline bool accept() const {
        std::array<Any::any_type, sizeof...(Args)> types{{Any::type<Args>()...}};
        return accept(types.data(), sizeof...(Args));
    }

    template<typename... Args>
    inline Any invoke(Args &&... args) const {
        const std::array<const Any, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return execute(params.data(), params.size());
    }
};


struct MetaData: public internal::MetaBase<internal::MetaDataNode> {
    virtual MetaClass * type() const ENTT_NOEXCEPT = 0;
    virtual bool constant() const ENTT_NOEXCEPT = 0;
    virtual Any get(const void *) const ENTT_NOEXCEPT = 0;
    virtual void set(void *, const Any &) = 0;
};


class MetaFunc: public internal::MetaBase<internal::MetaFuncNode> {
    virtual bool accept(Any::any_type * const, std::size_t) const = 0;
    virtual Any execute(const void *, const Any *, std::size_t) const = 0;
    virtual Any execute(void *, const Any *, std::size_t) = 0;

public:
    using size_type = std::size_t;

    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * ret() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline bool accept() const {
        std::array<Any::any_type, sizeof...(Args)> types{{Any::type<Args>()...}};
        return accept(types.data(), sizeof...(Args));
    }

    template<typename Instance, typename... Args>
    inline Any invoke(Instance *instance, Args &&... args) const {
        const std::array<const Any, sizeof...(Args)> params{{std::forward<Args>(args)...}};
        return execute(instance, params.data(), params.size());
    }
};


class MetaClass: public internal::MetaBase<internal::MetaClassNode> {
    template<typename Node, typename Op>
    inline void all(Node *curr, Op op) const ENTT_NOEXCEPT {
        while(curr) {
            op(curr->meta);
            curr = curr->next;
        }
    }

public:
    using size_type = std::size_t;

    virtual void destroy(void *) = 0;

    inline MetaCtor * ctor(const char *str) const ENTT_NOEXCEPT {
        return internal::meta<MetaCtor>(str, node()->ctor);
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
    Any construct(Args &&... args) const {
        auto *curr = node()->ctor;
        Any any;

        while(curr && !curr->meta->accept<Args...>()) {
            curr = curr->next;
        }

        assert(curr);

        return curr->meta->invoke(std::forward<Args>(args)...);
    }
};


namespace internal {


template<std::size_t Index, typename Class, typename... Args>
class MetaCtorPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, Type Class:: *Member>
class MetaMemberPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, typename ExtendedFunc<Class, Type>::type *Func>
class MetaFunctionPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>;
    }
};


template<std::size_t Index, typename Class, typename Type, typename Func>
class MetaFunctorPropType final: public MetaProp {
    internal::MetaPropNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Functor<Type, Func>::template prop<Index>;
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
    static auto ctor(const Any *args, std::index_sequence<Indexes...>) {
        return Any{Class{(args+Indexes)->value<std::decay_t<Args>>()...}};
    }

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{{meta<std::decay_t<Args>>()...}}[index];
    }

    internal::MetaCtorNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::ctor;
    }

    bool accept(Any::any_type * const types, std::size_t sz) const override {
        return sz == sizeof...(Args) && Accept<std::decay_t<Args>...>::types(types);
    }

    Any execute(const Any *args, std::size_t sz) const override {
        return sz == sizeof...(Args) && Accept<std::decay_t<Args>...>::types(args)
                  ? ctor(args, std::make_index_sequence<sizeof...(Args)>{}) : Any{};
    }

public:
    size_type size() const ENTT_NOEXCEPT {
        return sizeof...(Args);
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT {
        return index < sizeof...(Args) ? arg(index, std::make_index_sequence<sizeof...(Args)>{}) : nullptr;
    }
};


template<typename Class, typename Type, Type Class:: *Member>
class MetaDataType final: public MetaData {
    static_assert(std::is_member_object_pointer<Type Class:: *>::value, "!");

    template<bool Const>
    inline static std::enable_if_t<!Const>
    set(void *instance, const Any &any) {
        assert(Any::type<Type>() == any.type());
        static_cast<Class *>(instance)->*Member = any.value<std::decay_t<Type>>();
    }

    template<bool Const>
    inline static std::enable_if_t<Const>
    set(void *, const Any &) { assert(false); }

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

    Any get(const void *instance) const ENTT_NOEXCEPT override {
        return Any{static_cast<const Class *>(instance)->*Member};
    }

    void set(void *instance, const Any &any) override {
        return set<std::is_const<Type>::value>(instance, any);
    }
};


template<typename Class, typename Type, Type Class:: *Member>
class MetaFuncType final: public MetaFunc {
    static_assert(std::is_member_function_pointer<Type Class:: *>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename... Args>
    struct Invoker<std::tuple<Args...>> {
        template<typename Ret, typename Instance, std::size_t... Indexes>
        static auto invoke(int, Instance *instance, const Any *args, std::index_sequence<Indexes...>)
        -> std::enable_if_t<std::is_void<Ret>::value, decltype((std::declval<std::conditional_t<std::is_const<Instance>::value, const Class, Class>>().*Member)(std::declval<Args>()...), Any{})>
        {
            auto *object = static_cast<std::conditional_t<std::is_const<Instance>::value, const Class, Class> *>(instance);
            (object->*Member)((args+Indexes)->value<std::decay_t<Args>>()...);
            return Any{};
        }

        template<typename Ret, typename Instance, std::size_t... Indexes>
        static auto invoke(int, Instance *instance, const Any *args, std::index_sequence<Indexes...>)
        -> std::enable_if_t<!std::is_void<Ret>::value, decltype((std::declval<std::conditional_t<std::is_const<Instance>::value, const Class, Class>>().*Member)(std::declval<Args>()...), Any{})>
        {
            auto *object = static_cast<std::conditional_t<std::is_const<Instance>::value, const Class, Class> *>(instance);
            return Any{(object->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)};
        }

        template<typename, typename Instance, std::size_t... Indexes>
        static Any invoke(char, Instance *, const Any *, std::index_sequence<Indexes...>) {
            assert(false);
            return Any{};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}}[index];
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::member;
    }

    bool accept(Any::any_type * const types, std::size_t sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(types);
    }

    Any execute(const void *instance, const Any *args, std::size_t sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(args)
                ? Invoker<typename func_type::args_type>::template invoke<typename func_type::return_type>(0, instance, args, std::make_index_sequence<func_type::size>{})
                : Any{};
    }

    Any execute(void *instance, const Any *args, std::size_t sz) override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(args)
                ? Invoker<typename func_type::args_type>::template invoke<typename func_type::return_type>(0, instance, args, std::make_index_sequence<func_type::size>{})
                : Any{};
    }

public:
    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }
};


template<typename Class, typename Type, typename ExtendedFunc<Class, Type>::type *Func>
class MetaFreeFuncType final: public MetaFunc {
    static_assert(std::is_function<Type>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename... Args>
    struct Invoker<std::tuple<Args...>> {
        template<typename Ret, std::size_t... Indexes>
        static std::enable_if_t<std::is_void<Ret>::value, Any>
        invoke(const void *instance, const Any *args, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...);
            return Any{};
        }

        template<typename Ret, std::size_t... Indexes>
        static std::enable_if_t<!std::is_void<Ret>::value, Any>
        invoke(const void *instance, const Any *args, std::index_sequence<Indexes...>) {
            return Any{(*Func)(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...)};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}}[index];
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func;
    }

    bool accept(Any::any_type * const types, std::size_t sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(types);
    }

    Any execute(const void *instance, const Any *args, size_type sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(args)
                ? Invoker<typename func_type::args_type>::template invoke<typename func_type::return_type>(instance, args, std::make_index_sequence<func_type::size>{})
                : Any{};
    }

    Any execute(void *instance, const Any *args, size_type sz) override {
        return execute(static_cast<const void *>(instance), args, sz);
    }

public:
    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }
};


template<typename Class, typename Type, typename Func>
class MetaFunctorType final: public MetaFunc, public Func {
    static_assert(std::is_function<Type>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename... Args>
    struct Invoker<std::tuple<Args...>> {
        template<typename Ret, typename Op, std::size_t... Indexes>
        static std::enable_if_t<std::is_void<Ret>::value, Any>
        invoke(Op &op, const void *instance, const Any *args, std::index_sequence<Indexes...>) {
            op(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...);
            return Any{};
        }

        template<typename Ret, typename Op, std::size_t... Indexes>
        static std::enable_if_t<!std::is_void<Ret>::value, Any>
        invoke(Op &op, const void *instance, const Any *args, std::index_sequence<Indexes...>) {
            return Any{op(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...)};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}}[index];
    }

    internal::MetaFuncNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Functor<Func>::functor;
    }

    bool accept(Any::any_type * const types, std::size_t sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(types);
    }

    Any execute(const void *instance, const Any *args, size_type sz) const override {
        return sz == func_type::size && Accept<typename func_type::args_type>::types(args)
                ? Invoker<typename func_type::args_type>::template invoke<typename func_type::return_type>(*this, instance, args, std::make_index_sequence<func_type::size>{})
                : Any{};
    }

    Any execute(void *instance, const Any *args, size_type sz) override {
        return execute(static_cast<const void *>(instance), args, sz);
    }

public:
    MetaFunctorType(Func func)
        : Func{std::move(func)}
    {}

    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }
};


template<typename Class>
class MetaClassType final: public MetaClass {
    MetaClassNode * node() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::clazz;
    }

public:
    void destroy(void *instance) override {
        static_cast<Class *>(instance)->~Class();
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
        static internal::MetaClassPropType<Index, Class> meta{};
        static internal::MetaPropNode node{property.first, property.second, internal::MetaInfo<Class>::clazz->prop, &meta};
        assert(!internal::meta<MetaProp>(property.first, internal::MetaInfo<Class>::clazz->prop));
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
        static internal::MetaCtorPropType<Index, Class, Args...> meta{};
        static internal::MetaPropNode node{property.first, property.second, internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop, &meta};
        assert(!internal::meta<MetaProp>(property.first, internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop));
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

    template<std::size_t Index, typename Type, Type Class:: *Member, typename Property>
    static void member(const Property &property) {
        static internal::MetaMemberPropType<Index, Class, Type, Member> meta{};
        static internal::MetaPropNode node{property.first, property.second, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop, &meta};
        assert(!internal::meta<MetaProp>(property.first, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop));
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

    template<std::size_t Index, typename Type, typename internal::ExtendedFunc<Class, Type>::type *Func, typename Property>
    static void attached(const Property &property) {
        static internal::MetaFunctionPropType<Index, Class, Type, Func> meta{};
        static internal::MetaPropNode node{property.first, property.second, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop, &meta};
        assert(!internal::meta<MetaProp>(property.first, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop));
        assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>));
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop = &node;
    }

    template<typename Type, typename internal::ExtendedFunc<Class, Type>::type *Func, std::size_t... Indexes, typename... Property>
    static void attached(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attached<Indexes, Type, Func>(std::forward<Property>(property)), 0)... };
        (void)accumulator;
    }

    template<std::size_t Index, typename Type, typename Func, typename Property>
    static void attached(const Property &property) {
        static internal::MetaFunctorPropType<Index, Class, Type, Func> meta{};
        static internal::MetaPropNode node{property.first, property.second, internal::MetaInfo<Class>::template Functor<Func>::functor->prop, &meta};
        assert(!internal::meta<MetaProp>(property.first, internal::MetaInfo<Class>::template Functor<Func>::functor->prop));
        assert((!internal::MetaInfo<Class>::template Functor<Type, Func>::template prop<Index>));
        internal::MetaInfo<Class>::template Functor<Type, Func>::template prop<Index> = &node;
        internal::MetaInfo<Class>::template Functor<Func>::functor->prop = &node;
    }

    template<typename Type, typename Func, std::size_t... Indexes, typename... Property>
    static void attached(std::index_sequence<Indexes...>, Property &&... property) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attached<Indexes, Type, Func>(std::forward<Property>(property)), 0)... };
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
    static MetaFactory ctor(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaCtorType<Class, Args...> meta{};
        static internal::MetaCtorNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->ctor, &meta};
        assert(!internal::meta<MetaCtor>(HashedString{str}, internal::MetaInfo<Class>::clazz->ctor));
        assert(!internal::MetaInfo<Class>::template Ctor<Args...>::ctor);
        internal::MetaInfo<Class>::template Ctor<Args...>::ctor = &node;
        internal::MetaInfo<Class>::clazz->ctor = &node;
        ctor<Args...>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
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

    template<typename Type, typename internal::ExtendedFunc<Class, Type>::type *Func, typename... Property>
    static MetaFactory func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFreeFuncType<Class, Type, Func> meta{};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::clazz->func));
        assert((!internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func));
        internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func = &node;
        internal::MetaInfo<Class>::clazz->func = &node;
        attached<Type, Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }

    template<typename Type, typename Func, typename... Property>
    static MetaFactory func(const char *str, Func func, Property &&... property) ENTT_NOEXCEPT {
        static typename internal::MetaFunctorType<Class, Type, Func> meta{std::move(func)};
        static internal::MetaFuncNode node{HashedString{str}, internal::MetaInfo<Class>::clazz->func, &meta};
        assert(!internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::clazz->func));
        assert((!internal::MetaInfo<Class>::template Functor<Func>::functor));
        internal::MetaInfo<Class>::template Functor<Func>::functor = &node;
        internal::MetaInfo<Class>::clazz->func = &node;
        attached<Type, Func>(std::make_index_sequence<sizeof...(Property)>{}, std::forward<Property>(property)...);
        return MetaFactory<Class>{};
    }
};


template<typename Class, typename... Property>
inline MetaFactory<Class> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return MetaFactory<Class>::reflect(str, std::forward<Property>(property)...);
}


template<typename Class>
inline MetaClass * meta() ENTT_NOEXCEPT {
    static_assert(std::is_class<Class>::value, "!");
    return internal::MetaInfo<Class>::clazz ? internal::MetaInfo<Class>::clazz->meta : nullptr;
}


inline MetaClass * meta(const char *str) ENTT_NOEXCEPT {
    return internal::meta<MetaClass>(HashedString{str}, internal::MetaInfo<>::clazz);
}


}


#endif // ENTT_META_META_HPP
