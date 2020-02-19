#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <cstddef>
#include <utility>
#include <functional>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"


namespace entt {


class meta_any;
class meta_type;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_type_node;


struct meta_prop_node {
    meta_prop_node * next;
    meta_any(* const key)();
    meta_any(* const value)();
};


struct meta_base_node {
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    void *(* const cast)(void *) ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    meta_any(* const conv)(const void *);
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
};


struct meta_dtor_node {
    meta_type_node * const parent;
    void(* const invoke)(void *);
};


struct meta_data_node {
    ENTT_ID_TYPE alias;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_any, meta_any, meta_any);
    meta_any(* const get)(meta_any, meta_any);
};


struct meta_func_node {
    using size_type = std::size_t;
    ENTT_ID_TYPE alias;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)() ENTT_NOEXCEPT;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any, meta_any *);
};


struct meta_type_node {
    using size_type = std::size_t;
    const ENTT_ID_TYPE type_id;
    ENTT_ID_TYPE alias;
    meta_type_node * next;
    meta_prop_node * prop;
    const bool is_void;
    const bool is_integral;
    const bool is_floating_point;
    const bool is_array;
    const bool is_enum;
    const bool is_union;
    const bool is_class;
    const bool is_pointer;
    const bool is_function_pointer;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const size_type extent;
    bool(* const compare)(const void *, const void *);
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_dtor_node *dtor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
};


template<typename Type, typename Op, typename Node>
void visit(Op op, Node *node) {
    while(node) {
        op(Type{node});
        node = node->next;
    }
}


template<auto Member, typename Type, typename Op>
void visit(Op op, const internal::meta_type_node *node) {
    if(node) {
        internal::visit<Type>(op, node->*Member);
        auto *next = node->base;

        while(next) {
            visit<Member, Type>(op, next->type());
            next = next->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(Op op, Node *node) {
    while(node && !op(node)) {
        node = node->next;
    }

    return node;
}


template<auto Member, typename Op>
auto find_if(Op op, const meta_type_node *node)
-> decltype(find_if(op, node->*Member)) {
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *next = node->base;

        while(next && !ret) {
            ret = find_if<Member>(op, next->type());
            next = next->next;
        }
    }

    return ret;
}


template<typename Type>
bool compare(const void *lhs, const void *rhs) {
    if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
        return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
    } else {
        return lhs == rhs;
    }
}


template<typename... Type>
struct meta_node {
    static_assert(std::is_same_v<Type..., std::remove_cv_t<std::remove_reference_t<Type>>...>);

    inline static meta_type_node * resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            type_info<Type...>::id(),
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type...>,
            std::is_integral_v<Type...>,
            std::is_floating_point_v<Type...>,
            std::is_array_v<Type...>,
            std::is_enum_v<Type...>,
            std::is_union_v<Type...>,
            std::is_class_v<Type...>,
            std::is_pointer_v<Type...>,
            std::is_pointer_v<Type...> && std::is_function_v<std::remove_pointer_t<Type>...>,
            std::is_member_object_pointer_v<Type...>,
            std::is_member_function_pointer_v<Type...>,
            std::extent_v<Type...>,
            &compare<Type...>, // workaround for an issue with VS2017
            &meta_node<std::remove_const_t<std::remove_pointer_t<Type>>...>::resolve,
            &meta_node<std::remove_const_t<std::remove_extent_t<Type>>...>::resolve
        };

        return &node;
    }
};


template<>
struct meta_node<> {
    inline static meta_type_node *local = nullptr;
    inline static meta_type_node **global = &local;
};


template<typename... Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>...> {};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Opaque container for a meta context. */
struct meta_ctx {
    /**
     * @brief Binds the meta system to the given context.
     * @param other A valid context to which to bind.
     */
    static void bind(meta_ctx other) ENTT_NOEXCEPT {
        internal::meta_info<>::global = other.ctx;
    }

private:
    internal::meta_type_node **ctx{&internal::meta_info<>::local};
};


/**
 * @brief Opaque container for values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to get rid
 * of memory allocations if possible. This should improve overall performance.
 */
class meta_any {
    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using copy_fn_type = void(meta_any &, const meta_any &);
    using steal_fn_type = void(meta_any &, meta_any &);
    using destroy_fn_type = void(meta_any &);

    template<typename Type, typename = std::void_t<>>
    struct type_traits {
        template<typename... Args>
        static void instance(meta_any &any, Args &&... args) {
            any.instance = new Type{std::forward<Args>(args)...};
            new (&any.storage) Type *{static_cast<Type *>(any.instance)};
        }

        static void destroy(meta_any &any) {
            const auto * const node = internal::meta_info<Type>::resolve();
            if(node->dtor) { node->dtor->invoke(any.instance); }
            delete static_cast<Type *>(any.instance);
        }

        static void copy(meta_any &to, const meta_any &from) {
            auto *instance = new Type{*static_cast<const Type *>(from.instance)};
            new (&to.storage) Type *{instance};
            to.instance = instance;
        }

        static void steal(meta_any &to, meta_any &from) {
            new (&to.storage) Type *{static_cast<Type *>(from.instance)};
            to.instance = from.instance;
        }
    };

    template<typename Type>
    struct type_traits<Type, std::enable_if_t<sizeof(Type) <= sizeof(void *) && std::is_nothrow_move_constructible_v<Type>>> {
        template<typename... Args>
        static void instance(meta_any &any, Args &&... args) {
            any.instance = new (&any.storage) Type{std::forward<Args>(args)...};
        }

        static void destroy(meta_any &any) {
            const auto * const node = internal::meta_info<Type>::resolve();
            if(node->dtor) { node->dtor->invoke(any.instance); }
            static_cast<Type *>(any.instance)->~Type();
        }

        static void copy(meta_any &to, const meta_any &from) {
            to.instance = new (&to.storage) Type{*static_cast<const Type *>(from.instance)};
        }

        static void steal(meta_any &to, meta_any &from) {
            to.instance = new (&to.storage) Type{std::move(*static_cast<Type *>(from.instance))};
            destroy(from);
        }
    };

    meta_any(const internal::meta_type_node *curr, void *ref) ENTT_NOEXCEPT
        : meta_any{}
    {
        node = curr;
        instance = ref;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{},
          node{},
          destroy_fn{},
          copy_fn{},
          steal_fn{}
    {}

    /**
     * @brief Constructs a meta any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the container.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : meta_any{}
    {
        node = internal::meta_info<Type>::resolve();

        if constexpr(!std::is_void_v<Type>) {
            static_assert(std::is_copy_constructible_v<Type>);
            using traits_type = type_traits<std::remove_cv_t<std::remove_reference_t<Type>>>;
            traits_type::instance(*this, std::forward<Args>(args)...);
            destroy_fn = &traits_type::destroy;
            copy_fn = &traits_type::copy;
            steal_fn = &traits_type::steal;
        }
    }

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type>
    meta_any(std::reference_wrapper<Type> value)
        : meta_any{internal::meta_info<Type>::resolve(), &value.get()}
    {}

    /**
     * @brief Constructs a meta any from a given value.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other)
        : meta_any{}
    {
        node = other.node;
        (other.copy_fn ? other.copy_fn : [](meta_any &to, const meta_any &from) { to.instance = from.instance; })(*this, other);
        destroy_fn = other.destroy_fn;
        copy_fn = other.copy_fn;
        steal_fn = other.steal_fn;
    }

    /**
     * @brief Move constructor.
     *
     * After move construction, instances that have been moved from are placed
     * in a valid but unspecified state.
     *
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other)
        : meta_any{}
    {
        swap(*this, other);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy_fn) {
            destroy_fn(*this);
        }
    }

    /**
     * @brief Assignment operator.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     * @return This meta any object.
     */
    template<typename Type>
    meta_any & operator=(Type &&value) {
        return (*this = meta_any{std::forward<Type>(value)});
    }

    /**
     * @brief Assignment operator.
     * @param other The instance to assign from.
     * @return This meta any object.
     */
    meta_any & operator=(meta_any other) {
        swap(other, *this);
        return *this;
    }

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    /*! @copydoc data */
    void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Tries to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return A (possibly null) pointer to the contained instance.
     */
    template<typename Type>
    const Type * try_cast() const {
        void *ret = nullptr;

        if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node && node->type_id == type_id) {
            ret = instance;
        } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); base) {
            ret = base->cast(instance);
        }

        return static_cast<const Type *>(ret);
    }

    /*! @copydoc try_cast */
    template<typename Type>
    Type * try_cast() {
        return const_cast<Type *>(std::as_const(*this).try_cast<Type>());
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the cast is possible.
     *
     * @warning
     * Attempting to perform a cast that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the cast is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    const Type & cast() const {
        auto * const actual = try_cast<Type>();
        ENTT_ASSERT(actual);
        return *actual;
    }

    /*! @copydoc cast */
    template<typename Type>
    Type & cast() {
        return const_cast<Type &>(std::as_const(*this).cast<Type>());
    }

    /**
     * @brief Tries to convert an instance to a given type and returns it.
     * @tparam Type Type to which to convert the instance.
     * @return A valid meta any object if the conversion is possible, an invalid
     * one otherwise.
     */
    template<typename Type>
    meta_any convert() const {
        meta_any any{};

        if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node && node->type_id == type_id) {
            any = *this;
        } else if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); conv) {
            any = conv->conv(instance);
        }

        return any;
    }

    /**
     * @brief Tries to convert an instance to a given type.
     * @tparam Type Type to which to convert the instance.
     * @return True if the conversion is possible, false otherwise.
     */
    template<typename Type>
    bool convert() {
        bool valid = (node && node->type_id == internal::meta_info<Type>::resolve()->type_id);

        if(!valid) {
            if(auto any = std::as_const(*this).convert<Type>(); any) {
                swap(any, *this);
                valid = true;
            }
        }

        return valid;
    }

    /**
     * @brief Replaces the contained object by initializing a new instance
     * directly.
     * @tparam Type Type of object to use to initialize the container.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = meta_any{std::in_place_type_t<Type>{}, std::forward<Args>(args)...};
    }

    /**
     * @brief Indirection operator for aliasing construction.
     * @return An alias to the contained object.
     */
    meta_any operator *() const ENTT_NOEXCEPT {
        return meta_any{node, instance};
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    bool operator==(const meta_any &other) const {
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id && node->compare(instance, other.instance));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) {
        if(lhs.steal_fn && rhs.steal_fn) {
            meta_any buffer{};
            lhs.steal_fn(buffer, lhs);
            rhs.steal_fn(lhs, rhs);
            lhs.steal_fn(rhs, buffer);
        } else if(lhs.steal_fn) {
            lhs.steal_fn(rhs, lhs);
        } else if(rhs.steal_fn) {
            rhs.steal_fn(lhs, rhs);
        } else {
            std::swap(lhs.instance, rhs.instance);
        }

        std::swap(lhs.node, rhs.node);
        std::swap(lhs.destroy_fn, rhs.destroy_fn);
        std::swap(lhs.copy_fn, rhs.copy_fn);
        std::swap(lhs.steal_fn, rhs.steal_fn);
    }

private:
    storage_type storage;
    void *instance;
    const internal::meta_type_node *node;
    destroy_fn_type *destroy_fn;
    copy_fn_type *copy_fn;
    steal_fn_type *steal_fn;
};


/**
 * @brief Opaque pointers to instances of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance.<br/>
 * Handles are used mainly to gnerate aliases for actual objects when needed.
 */
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle()
        : any{}
    {}

    /**
     * @brief Creates an alias for the actual object.
     * @tparam Type Type of object to use to initialize the container.
     * @param value An instance of an object to use to initialize the container.
     */
    template<typename Type>
    meta_handle(Type &&value) ENTT_NOEXCEPT
        : meta_handle{}
    {
        if constexpr(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>) {
            any = *value;
        } else {
            static_assert(std::is_lvalue_reference_v<Type>);
            any = std::ref(value);
        }
    }

    /*! @copydoc meta_any::operator* */
    meta_any operator *() const {
        return any;
    }

private:
    meta_any any;
};


/**
 * @brief Checks if two containers differ in their content.
 * @param lhs A meta any object, either empty or not.
 * @param rhs A meta any object, either empty or not.
 * @return True if the two containers differ in their content, false otherwise.
 */
inline bool operator!=(const meta_any &lhs, const meta_any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/*! @brief Opaque container for meta properties of any type. */
struct meta_prop {
    /**
     * @brief Constructs an instance from a given node.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_prop(const internal::meta_prop_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    meta_any key() const {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    meta_any value() const {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_prop_node *node;
};


/*! @brief Opaque container for meta base classes. */
struct meta_base {
    /*! @copydoc meta_prop::meta_prop */
    meta_base(const internal::meta_base_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the meta type to which a meta object belongs.
     * @return The meta type to which the meta object belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Casts an instance from a parent type to a base type.
     * @param instance The instance to cast.
     * @return An opaque pointer to the base type.
     */
    void * cast(void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_base_node *node;
};


/*! @brief Opaque container for meta conversion functions. */
struct meta_conv {
    /*! @copydoc meta_prop::meta_prop */
    meta_conv(const internal::meta_conv_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Converts an instance to a given type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    meta_any convert(const void *instance) const {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_conv_node *node;
};


/*! @brief Opaque container for meta constructors. */
struct meta_ctor {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_ctor(const internal::meta_ctor_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta constructor.
     * @return The number of arguments accepted by the meta constructor.
     */
    size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta constructor.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta constructor, if any.
     */
    meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke([[maybe_unused]] Args &&... args) const {
        if constexpr(sizeof...(Args) == 0) {
            return sizeof...(Args) == size() ? node->invoke(nullptr) : meta_any{};
        } else {
            meta_any arguments[]{std::forward<Args>(args)...};
            return sizeof...(Args) == size() ? node->invoke(arguments) : meta_any{};
        }
    }

    /**
     * @brief Iterates all the properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_ctor_node *node;
};


/*! @brief Opaque container for meta data. */
struct meta_data {
    /*! @copydoc meta_prop::meta_prop */
    meta_data(const internal::meta_data_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::alias */
    ENTT_ID_TYPE alias() const ENTT_NOEXCEPT {
        return node->alias;
    }

    /*! @copydoc alias*/
    [[deprecated("Use ::alias instead")]]
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return alias();
    }

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Indicates whether a given meta data is constant or not.
     * @return True if the meta data is constant, false otherwise.
     */
    bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta data is static or not.
     * @return True if the meta data is static, false otherwise.
     */
    bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /*! @copydoc meta_any::type */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must be such that a cast or conversion to the type
     * of the variable is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param instance An opaque instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle instance, Type &&value) const {
        return node->set(*instance, {}, std::forward<Type>(value));
    }

    /**
     * @brief Sets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must be such that a cast or conversion to the array
     * type is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param instance An opaque instance of the underlying type.
     * @param index Position of the underlying element to set.
     * @param value Parameter to use to set the underlying element.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle instance, std::size_t index, Type &&value) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->set(*instance, index, std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    meta_any get(meta_handle instance) const {
        return node->get(*instance, {});
    }

    /**
     * @brief Gets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @param index Position of the underlying element to get.
     * @return A meta any containing the value of the underlying element.
     */
    meta_any get(meta_handle instance, std::size_t index) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->get(*instance, index);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_data_node *node;
};


/*! @brief Opaque container for meta functions. */
struct meta_func {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_func(const internal::meta_func_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::alias */
    ENTT_ID_TYPE alias() const ENTT_NOEXCEPT {
        return node->alias;
    }

    /*! @copydoc alias */
    [[deprecated("Use ::alias instead")]]
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return alias();
    }

    /*! @copydoc meta_base::parent */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta function.
     * @return The number of arguments accepted by the meta function.
     */
    size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Indicates whether a given meta function is constant or not.
     * @return True if the meta function is constant, false otherwise.
     */
    bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta function is static or not.
     * @return True if the meta function is static, false otherwise.
     */
    bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type of the return type of a meta function.
     * @return The meta type of the return type of the meta function.
     */
    inline meta_type ret() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of the i-th argument of a meta function.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta function, if any.
     */
    inline meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a meta function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&... args) const {
        meta_any arguments[]{*instance, std::forward<Args>(args)...};
        return sizeof...(Args) == size() ? node->invoke(arguments[0], &arguments[sizeof...(Args) != 0]) : meta_any{};
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<meta_prop>(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node->prop);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const internal::meta_func_node *node;
};


/*! @brief Opaque container for meta types. */
class meta_type {
    template<typename... Args, std::size_t... Indexes>
    auto ctor(std::index_sequence<Indexes...>) const {
        return internal::find_if([](const auto *candidate) {
            return candidate->size == sizeof...(Args) && ([](auto *from, auto *to) {
                return (from->type_id == to->type_id)
                        || internal::find_if<&internal::meta_type_node::base>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from)
                        || internal::find_if<&internal::meta_type_node::conv>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from);
            }(internal::meta_info<Args>::resolve(), candidate->arg(Indexes)) && ...);
        }, node->ctor);
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_type(const internal::meta_type_node *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the id of the underlying type.
     * @return The id of the underlying type.
     */
    ENTT_ID_TYPE id() const ENTT_NOEXCEPT {
        return node->type_id;
    }

    /**
     * @brief Returns the alias assigned to a given meta object.
     * @return The alias assigned to the meta object.
     */
    ENTT_ID_TYPE alias() const ENTT_NOEXCEPT {
        return node->alias;
    }

    /*! @copydoc alias */
    [[deprecated("Use ::alias instead")]]
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return alias();
    }

    /**
     * @brief Indicates whether a given meta type refers to void or not.
     * @return True if the underlying type is void, false otherwise.
     */
    bool is_void() const ENTT_NOEXCEPT {
        return node->is_void;
    }

    /**
     * @brief Indicates whether a given meta type refers to an integral type or
     * not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    bool is_integral() const ENTT_NOEXCEPT {
        return node->is_integral;
    }

    /**
     * @brief Indicates whether a given meta type refers to a floating-point
     * type or not.
     * @return True if the underlying type is a floating-point type, false
     * otherwise.
     */
    bool is_floating_point() const ENTT_NOEXCEPT {
        return node->is_floating_point;
    }

    /**
     * @brief Indicates whether a given meta type refers to an array type or
     * not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    bool is_array() const ENTT_NOEXCEPT {
        return node->is_array;
    }

    /**
     * @brief Indicates whether a given meta type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    bool is_enum() const ENTT_NOEXCEPT {
        return node->is_enum;
    }

    /**
     * @brief Indicates whether a given meta type refers to an union or not.
     * @return True if the underlying type is an union, false otherwise.
     */
    bool is_union() const ENTT_NOEXCEPT {
        return node->is_union;
    }

    /**
     * @brief Indicates whether a given meta type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    bool is_class() const ENTT_NOEXCEPT {
        return node->is_class;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    bool is_pointer() const ENTT_NOEXCEPT {
        return node->is_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a function pointer
     * or not.
     * @return True if the underlying type is a function pointer, false
     * otherwise.
     */
    bool is_function_pointer() const ENTT_NOEXCEPT {
        return node->is_function_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to data
     * member or not.
     * @return True if the underlying type is a pointer to data member, false
     * otherwise.
     */
    bool is_member_object_pointer() const ENTT_NOEXCEPT {
        return node->is_member_object_pointer;
    }

    /**
     * @brief Indicates whether a given meta type refers to a pointer to member
     * function or not.
     * @return True if the underlying type is a pointer to member function,
     * false otherwise.
     */
    bool is_member_function_pointer() const ENTT_NOEXCEPT {
        return node->is_member_function_pointer;
    }

    /**
     * @brief If a given meta type refers to an array type, provides the number
     * of elements of the array.
     * @return The number of elements of the array if the underlying type is an
     * array type, 0 otherwise.
     */
    size_type extent() const ENTT_NOEXCEPT {
        return node->extent;
    }

    /**
     * @brief Provides the meta type for which the pointer is defined.
     * @return The meta type for which the pointer is defined or this meta type
     * if it doesn't refer to a pointer type.
     */
    meta_type remove_pointer() const ENTT_NOEXCEPT {
        return node->remove_pointer();
    }

    /**
     * @brief Provides the meta type for which the array is defined.
     * @return The meta type for which the array is defined or this meta type
     * if it doesn't refer to an array type.
     */
    meta_type remove_extent() const ENTT_NOEXCEPT {
        return node->remove_extent();
    }

    /**
     * @brief Iterates all the meta bases of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_base>, void>
    base(Op op) const {
        internal::visit<&internal::meta_type_node::base, meta_base>(std::move(op), node);
    }

    /**
     * @brief Returns the meta base associated with a given alias.
     * @param alias Unique identifier.
     * @return The meta base associated with the given alias, if any.
     */
    meta_base base(const ENTT_ID_TYPE alias) const {
        return internal::find_if<&internal::meta_type_node::base>([alias](const auto *curr) {
            return curr->type()->alias == alias;
        }, node);
    }

    /**
     * @brief Iterates all the meta conversion functions of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void conv(Op op) const {
        internal::visit<&internal::meta_type_node::conv, meta_conv>(std::move(op), node);
    }

    /**
     * @brief Returns the meta conversion function associated with a given type.
     * @tparam Type The type to use to search for a meta conversion function.
     * @return The meta conversion function associated with the given type, if
     * any.
     */
    template<typename Type>
    meta_conv conv() const {
        return internal::find_if<&internal::meta_type_node::conv>([type_id = internal::meta_info<Type>::resolve()->type_id](const auto *curr) {
            return curr->type()->type_id == type_id;
        }, node);
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void ctor(Op op) const {
        internal::visit<meta_ctor>(std::move(op), node->ctor);
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    meta_ctor ctor() const {
        return ctor<Args...>(std::index_sequence_for<Args...>{});
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     *
     * The meta data of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_data>, void>
    data(Op op) const {
        internal::visit<&internal::meta_type_node::data, meta_data>(std::move(op), node);
    }

    /**
     * @brief Returns the meta data associated with a given alias.
     *
     * The meta data of the base classes will also be visited, if any.
     *
     * @param alias Unique identifier.
     * @return The meta data associated with the given alias, if any.
     */
    meta_data data(const ENTT_ID_TYPE alias) const {
        return internal::find_if<&internal::meta_type_node::data>([alias](const auto *curr) {
            return curr->alias == alias;
        }, node);
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     *
     * The meta functions of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_func>, void>
    func(Op op) const {
        internal::visit<&internal::meta_type_node::func, meta_func>(std::move(op), node);
    }

    /**
     * @brief Returns the meta function associated with a given alias.
     *
     * The meta functions of the base classes will also be visited, if any.
     *
     * @param alias Unique identifier.
     * @return The meta function associated with the given alias, if any.
     */
    meta_func func(const ENTT_ID_TYPE alias) const {
        return internal::find_if<&internal::meta_type_node::func>([alias](const auto *curr) {
            return curr->alias == alias;
        }, node);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any construct(Args &&... args) const {
        auto construct_if = [this](meta_any *params) {
            meta_any any{};

            internal::find_if<&internal::meta_type_node::ctor>([params, &any](const auto *curr) {
                return (curr->size == sizeof...(args)) && (any = curr->invoke(params));
            }, node);

            return any;
        };

        if constexpr(sizeof...(Args) == 0) {
            return construct_if(nullptr);
        } else {
            meta_any arguments[]{std::forward<Args>(args)...};
            return construct_if(arguments);
        }
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     *
     * The properties of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const {
        internal::visit<&internal::meta_type_node::prop, meta_prop>(std::move(op), node);
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * The properties of the base classes will also be visited, if any.
     *
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    meta_prop prop(meta_any key) const {
        return internal::find_if<&internal::meta_type_node::prop>([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two meta objects refer to the same type.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same type, false
     * otherwise.
     */
    bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id);
    }

private:
    const internal::meta_type_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same type.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return False if the two meta objects refer to the same node, true otherwise.
 */
inline bool operator!=(const meta_type &lhs, const meta_type &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


inline meta_type meta_any::type() const ENTT_NOEXCEPT {
    return node;
}


inline meta_type meta_base::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_base::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_conv::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_conv::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


inline meta_type meta_data::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_data::type() const ENTT_NOEXCEPT {
    return node->type();
}


inline meta_type meta_func::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


inline meta_type meta_func::ret() const ENTT_NOEXCEPT {
    return node->ret();
}


inline meta_type meta_func::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


}


#endif
