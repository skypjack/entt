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


struct MetaProp;
class MetaCtor;
struct MetaData;
class MetaFunc;
class MetaClass;


namespace internal {


struct MetaPropNode;
struct MetaCtorNode;
struct MetaDataNode;
struct MetaFuncNode;
struct MetaClassNode;


template<typename> struct MetaNode;
template<> struct MetaNode<MetaProp> { using type = MetaPropNode; };
template<> struct MetaNode<MetaCtor> { using type = MetaCtorNode; };
template<> struct MetaNode<MetaData> { using type = MetaDataNode; };
template<> struct MetaNode<MetaFunc> { using type = MetaFuncNode; };
template<> struct MetaNode<MetaClass> { using type = MetaClassNode; };


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


template<typename... Args, std::size_t... Indexes>
inline bool acceptable(const Any *args, std::index_sequence<Indexes...>) {
    bool res = true;
    using accumulator_type = bool[];
    accumulator_type accumulator = { res, (res = res && (args+Indexes)->type() == Any::type<Args>())... };
    (void)accumulator;
    return res;
}


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


}


template<typename Meta>
class Range {
    using node_type = typename internal::MetaNode<Meta>::type;

    class Iterator {
        friend class Range<Meta>;

        Iterator(node_type *node)
            : node{node}
        {}

    public:
        using difference_type = std::size_t;
        using value_type = Meta;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::forward_iterator_tag;

        Iterator() ENTT_NOEXCEPT = default;

        Iterator(const Iterator &) ENTT_NOEXCEPT = default;
        Iterator & operator=(const Iterator &) ENTT_NOEXCEPT = default;

        Iterator & operator++() ENTT_NOEXCEPT {
            node = node->next;
            return *this;
        }

        Iterator operator++(int) ENTT_NOEXCEPT {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const Iterator &other) const ENTT_NOEXCEPT {
            return other.node == node;
        }

        inline bool operator!=(const Iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return node->meta;
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        node_type *node;
    };

public:
    using iterator_type = Iterator;
    using const_iterator_type = Iterator;

    Range(node_type *node)
        : node{node}
    {}

    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        return Iterator{node};
    }

    inline const_iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    inline iterator_type begin() ENTT_NOEXCEPT {
        return cbegin();
    }

    const_iterator_type cend() const ENTT_NOEXCEPT {
        return Iterator{nullptr};
    }

    inline const_iterator_type end() const ENTT_NOEXCEPT {
        return cend();
    }

    inline iterator_type end() ENTT_NOEXCEPT {
        return cend();
    }

private:
    node_type * const node;
};


struct MetaProp {
    virtual ~MetaProp() ENTT_NOEXCEPT = default;
    virtual const Any & key() const ENTT_NOEXCEPT = 0;
    virtual const Any & value() const ENTT_NOEXCEPT = 0;
};


class MetaCtor {
    virtual Any run(const Any * const, std::size_t) const = 0;

    template<typename... Args>
    inline auto execute(int, Args &&... args) const
    -> decltype(run(std::forward<Args>(args)...), Any{})
    {
        return run(std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline Any execute(char, Args &&... args) const {
        const std::array<const Any, sizeof...(Args)> params{std::forward<Args>(args)...};
        return run(params.data(), params.size());
    }

public:
    using size_type = std::size_t;

    virtual ~MetaCtor() ENTT_NOEXCEPT = default;
    virtual const char * name() const ENTT_NOEXCEPT = 0;
    virtual size_type size() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    virtual Range<MetaProp> prop() const ENTT_NOEXCEPT = 0;
    virtual MetaProp * prop(const Any &) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline Any invoke(Args &&... args) const {
        return execute(0, std::forward<Args>(args)...);
    }
};


struct MetaData {
    virtual ~MetaData() ENTT_NOEXCEPT = default;
    virtual const char * name() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * type() const ENTT_NOEXCEPT = 0;
    virtual bool constant() const ENTT_NOEXCEPT = 0;
    virtual Any get(const void *) const ENTT_NOEXCEPT = 0;
    virtual void set(void *, const Any &) = 0;
    virtual Range<MetaProp> prop() const ENTT_NOEXCEPT = 0;
    virtual MetaProp * prop(const Any &) const ENTT_NOEXCEPT = 0;
};


class MetaFunc {
    virtual Any run(const void *, const Any *, std::size_t) const = 0;
    virtual Any run(void *, const Any *, std::size_t) = 0;

    template<typename... Args>
    inline auto execute(int, Args &&... args) const
    -> decltype(run(std::forward<Args>(args)...), Any{})
    {
        return run(std::forward<Args>(args)...);
    }

    template<typename Instance, typename... Args>
    inline Any execute(char, Instance &&instance, Args &&... args) const {
        const std::array<const Any, sizeof...(Args)> params{std::forward<Args>(args)...};
        return run(instance, params.data(), params.size());
    }

public:
    using size_type = std::size_t;

    virtual ~MetaFunc() ENTT_NOEXCEPT = default;
    virtual const char * name() const ENTT_NOEXCEPT = 0;
    virtual size_type size() const ENTT_NOEXCEPT = 0;

    virtual MetaClass * ret() const ENTT_NOEXCEPT = 0;
    virtual MetaClass * arg(size_type) const ENTT_NOEXCEPT = 0;

    virtual Range<MetaProp> prop() const ENTT_NOEXCEPT = 0;
    virtual MetaProp * prop(const Any &) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    inline Any invoke(Args &&... args) const {
        return execute(0, std::forward<Args>(args)...);
    }
};


class MetaClass {
    virtual Any run(const Any *, std::size_t) const = 0;

    template<typename... Args>
    inline auto execute(int, Args &&... args) const
    -> decltype(run(std::forward<Args>(args)...), Any{})
    {
        return run(std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline Any execute(char, Args &&... args) const {
        const std::array<const Any, sizeof...(Args)> params{std::forward<Args>(args)...};
        return run(params.data(), params.size());
    }

public:
    using size_type = std::size_t;

    virtual ~MetaClass() ENTT_NOEXCEPT = default;
    virtual const char * name() const ENTT_NOEXCEPT = 0;
    virtual void destroy(void *) = 0;

    virtual Range<MetaProp> prop() const ENTT_NOEXCEPT = 0;
    virtual MetaProp * prop(const Any &) const ENTT_NOEXCEPT = 0;

    virtual Range<MetaCtor> ctor() const ENTT_NOEXCEPT = 0;
    virtual MetaCtor * ctor(const char *str) const ENTT_NOEXCEPT = 0;

    virtual Range<MetaData> data() const ENTT_NOEXCEPT = 0;
    virtual MetaData * data(const char *str) const ENTT_NOEXCEPT = 0;

    virtual Range<MetaFunc> func() const ENTT_NOEXCEPT = 0;
    virtual MetaFunc * func(const char *str) const ENTT_NOEXCEPT = 0;

    template<typename... Args>
    Any construct(Args &&... args) const {
        return execute(0, std::forward<Args>(args)...);
    }
};


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


template<std::size_t Index, typename Class, typename... Args>
struct MetaCtorPropType: MetaProp {
    const Any & key() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>->key;
    }

    const Any & value() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::template prop<Index>->value;
    }
};


template<std::size_t Index, typename Class, typename Type, Type Class:: *Member>
struct MetaMemberPropType: MetaProp {
    const Any & key() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>->key;
    }

    const Any & value() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::template prop<Index>->value;
    }
};


template<std::size_t Index, typename Class, typename Type, typename ExtendedFunc<Class, Type>::type *Func>
struct MetaFunctionPropType: MetaProp {
    const Any & key() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>->key;
    }

    const Any & value() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::template prop<Index>->value;
    }
};


template<std::size_t Index, typename Class, typename Type, typename Func>
struct MetaFunctorPropType: MetaProp {
    const Any & key() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Functor<Type, Func>::template prop<Index>->key;
    }

    const Any & value() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Functor<Type, Func>::template prop<Index>->value;
    }
};


template<std::size_t Index, typename Class>
struct MetaClassPropType: MetaProp {
    const Any & key() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template prop<Index>->key;
    }

    const Any & value() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template prop<Index>->value;
    }
};


template<typename Class, typename... Args>
class MetaCtorType: public MetaCtor {
    template<std::size_t... Indexes>
    static auto ctor(const Any *args, std::index_sequence<Indexes...> indexes) {
        return acceptable<Args...>(args, indexes) ? Any{Class{(args+Indexes)->value<std::decay_t<Args>>()...}} : Any{};
    }

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{meta<std::decay_t<Args>>()...}[index];
    }

    Any run(const Any *args, std::size_t sz) const override {
        return sz == sizeof...(Args) ? ctor(args, std::make_index_sequence<sizeof...(Args)>{}) : Any{};
    }

public:
    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Ctor<Args...>::ctor->key;
    }

    size_type size() const ENTT_NOEXCEPT {
        return sizeof...(Args);
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT {
        return index < sizeof...(Args) ? arg(index, std::make_index_sequence<sizeof...(Args)>{}) : nullptr;
    }

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Ctor<Args...>::ctor->prop);
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

public:
    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::member->key;
    }

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

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::template Member<Type, Member>::member->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop);
    }
};


template<typename Class, typename Type, Type Class:: *Member>
class MetaFuncType: public MetaFunc {
    static_assert(std::is_member_function_pointer<Type Class:: *>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename Ret, typename... Args>
    struct Invoker<Ret(Args...)> {
        template<std::size_t... Indexes>
        inline static Any invoke(void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            return acceptable<Args...>(args, indexes)
                    ? Any{(static_cast<Class *>(instance)->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)}
                    : Any{};
        }

        template<std::size_t... Indexes>
        inline static Any invoke(const void *, const Any *, std::index_sequence<Indexes...>) {
            assert(false);
            return Any{};
        }
    };

    template<typename... Args>
    struct Invoker<void(Args...)> {
        template<std::size_t... Indexes>
        inline static Any invoke(void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            if(acceptable<Args...>(args, indexes)) {
                (static_cast<Class *>(instance)->*Member)((args+Indexes)->value<std::decay_t<Args>>()...);
            }

            return Any{};
        }

        template<std::size_t... Indexes>
        inline static Any invoke(const void *, const Any *, std::index_sequence<Indexes...>) {
            assert(false);
            return Any{};
        }
    };

    template<typename Ret, typename... Args>
    struct Invoker<Ret(Args...) const> {
        template<std::size_t... Indexes>
        inline static Any invoke(const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            return acceptable<Args...>(args, indexes)
                    ? Any{(static_cast<const Class *>(instance)->*Member)((args+Indexes)->value<std::decay_t<Args>>()...)}
                    : Any{};
        }
    };

    template<typename... Args>
    struct Invoker<void(Args...) const> {
        template<std::size_t... Indexes>
        inline static Any invoke(const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            if(acceptable<Args...>(args, indexes)) {
                (static_cast<const Class *>(instance)->*Member)((args+Indexes)->value<std::decay_t<Args>>()...);
            }

            return Any{};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}[index];
    }

    Any run(const void *instance, const Any *args, std::size_t sz) const override {
        return sz == func_type::size ? Invoker<Type>::invoke(instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

    Any run(void *instance, const Any *args, std::size_t sz) override {
        return sz == func_type::size ? Invoker<Type>::invoke(instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

public:
    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Member<Type, Member>::member->key;
    }

    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::template Member<Type, Member>::member->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Member<Type, Member>::member->prop);
    }

};


template<typename Class, typename Type, typename ExtendedFunc<Class, Type>::type *Func>
class MetaFreeFuncType: public MetaFunc {
    static_assert(std::is_function<Type>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename Ret, typename... Args>
    struct Invoker<Ret(Args...)> {
        template<std::size_t... Indexes>
        inline static Any invoke(const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            return acceptable<Args...>(args, indexes)
                    ? Any{(*Func)(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...)}
                    : Any{};
        }
    };

    template<typename... Args>
    struct Invoker<void(Args...)> {
        template<std::size_t... Indexes>
        inline static Any invoke(const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            if(acceptable<Args...>(args, indexes)) {
                (*Func)(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...);
            }

            return Any{};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}[index];
    }

    Any run(const void *instance, const Any *args, size_type sz) const override {
        return sz == func_type::size ? Invoker<Type>::invoke(instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

    Any run(void *instance, const Any *args, size_type sz) override {
        return sz == func_type::size ? Invoker<Type>::invoke(instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

public:
    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->key;
    }

    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template FreeFunc<Type, Func>::func->prop);
    }

};


template<typename Class, typename Type, typename Func>
class MetaFunctorType: public MetaFunc, public Func {
    static_assert(std::is_function<Type>::value, "!");

    using func_type = FuncType<Type>;

    template<typename>
    struct Invoker;

    template<typename Ret, typename... Args>
    struct Invoker<Ret(Args...)> {
        template<typename Op, std::size_t... Indexes>
        inline static Any invoke(Op &&op, const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            return acceptable<Args...>(args, indexes)
                    ? Any{op(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...)}
                    : Any{};
        }
    };

    template<typename... Args>
    struct Invoker<void(Args...)> {
        template<typename Op, std::size_t... Indexes>
        inline static Any invoke(Op &&op, const void *instance, const Any *args, std::index_sequence<Indexes...> indexes) {
            if(acceptable<Args...>(args, indexes)) {
                op(*static_cast<const Class *>(instance), (args+Indexes)->value<std::decay_t<Args>>()...);
            }

            return Any{};
        }
    };

    template<std::size_t... Indexes>
    inline static MetaClass * arg(size_type index, std::index_sequence<Indexes...>) {
        return std::array<MetaClass *, sizeof...(Indexes)>{meta<std::decay_t<std::tuple_element_t<Indexes, typename func_type::args_type>>>()...}[index];
    }

    Any run(const void *instance, const Any *args, size_type sz) const override {
        return sz == func_type::size ? Invoker<Type>::invoke(*this, instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

    Any run(void *instance, const Any *args, size_type sz) override {
        return sz == func_type::size ? Invoker<Type>::invoke(*this, instance, args, std::make_index_sequence<func_type::size>{}) : Any{};
    }

public:
    MetaFunctorType(Func func)
        : Func{std::move(func)}
    {}

    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::template Functor<Func>::functor->key;
    }

    size_type size() const ENTT_NOEXCEPT override {
        return func_type::size;
    }

    MetaClass * ret() const ENTT_NOEXCEPT override {
        return meta<typename func_type::return_type>();
    }

    MetaClass * arg(size_type index) const ENTT_NOEXCEPT override {
        return index < func_type::size ? arg(index, std::make_index_sequence<func_type::size>{}) : nullptr;
    }

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::template Functor<Func>::functor->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::template Functor<Func>::functor->prop);
    }

};


template<typename Class>
class MetaClassType final: public MetaClass {
    Any run(const Any *args, std::size_t sz) const override {
        const auto *curr = internal::MetaInfo<Class>::clazz->ctor;
        Any any{};

        while(curr && !any) {
            any = curr->meta->invoke(args, sz);
            curr = curr->next;
        }

        return any;
    }

public:
    using size_type = typename MetaClass::size_type;

    const char * name() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Class>::clazz->key;
    }

    void destroy(void *instance) override {
        static_cast<Class *>(instance)->~Class();
    }

    Range<MetaProp> prop() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::clazz->prop};
    }

    MetaProp * prop(const Any &key) const ENTT_NOEXCEPT override {
        return internal::meta<MetaProp>(key, internal::MetaInfo<Class>::clazz->prop);
    }

    Range<MetaCtor> ctor() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::clazz->ctor};
    }

    MetaCtor * ctor(const char *str) const ENTT_NOEXCEPT override {
        return internal::meta<MetaCtor>(HashedString{str}, internal::MetaInfo<Class>::clazz->ctor);
    }

    Range<MetaData> data() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::clazz->data};
    }

    MetaData * data(const char *str) const ENTT_NOEXCEPT override {
        return internal::meta<MetaData>(HashedString{str}, internal::MetaInfo<Class>::clazz->data);
    }

    Range<MetaFunc> func() const ENTT_NOEXCEPT override {
        return {internal::MetaInfo<Class>::clazz->func};
    }

    MetaFunc * func(const char *str) const ENTT_NOEXCEPT override {
        return internal::meta<MetaFunc>(HashedString{str}, internal::MetaInfo<Class>::clazz->func);
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
