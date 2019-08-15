#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <array>
#include <memory>
#include <cstring>
#include <cstddef>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "policy.hpp"


namespace entt {


class meta_any;
struct meta_handle;
class meta_prop;
class meta_base;
class meta_conv;
class meta_ctor;
class meta_dtor;
class meta_data;
class meta_func;
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
    meta_prop(* const meta)() ENTT_NOEXCEPT;
};


struct meta_base_node {
    meta_base_node ** const underlying;
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    void *(* const cast)(void *) ENTT_NOEXCEPT;
    meta_base(* const meta)() ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_conv_node ** const underlying;
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    meta_any(* const conv)(const void *);
    meta_conv(* const meta)() ENTT_NOEXCEPT;
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node ** const underlying;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
    meta_ctor(* const meta)() ENTT_NOEXCEPT;
};


struct meta_dtor_node {
    meta_dtor_node ** const underlying;
    meta_type_node * const parent;
    bool(* const invoke)(meta_handle);
    meta_dtor(* const meta)() ENTT_NOEXCEPT;
};


struct meta_data_node {
    meta_data_node ** const underlying;
    ENTT_ID_TYPE identifier;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_handle, meta_any, meta_any);
    meta_any(* const get)(meta_handle, meta_any);
    meta_data(* const meta)() ENTT_NOEXCEPT;
};


struct meta_func_node {
    using size_type = std::size_t;
    meta_func_node ** const underlying;
    ENTT_ID_TYPE identifier;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)() ENTT_NOEXCEPT;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_handle, meta_any *);
    meta_func(* const meta)() ENTT_NOEXCEPT;
};


struct meta_type_node {
    using size_type = std::size_t;
    ENTT_ID_TYPE identifier;
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
    const bool is_function;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const size_type extent;
    meta_type(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type(* const meta)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_dtor_node *dtor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
};


template<typename...>
struct meta_node {
    inline static meta_type_node *type = nullptr;
};


template<typename Type>
struct meta_node<Type> {
    inline static meta_type_node *type = nullptr;

    template<typename>
    inline static meta_base_node *base = nullptr;

    template<typename>
    inline static meta_conv_node *conv = nullptr;

    template<typename>
    inline static meta_ctor_node *ctor = nullptr;

    template<auto>
    inline static meta_dtor_node *dtor = nullptr;

    template<auto...>
    inline static meta_data_node *data = nullptr;

    template<auto>
    inline static meta_func_node *func = nullptr;

    inline static meta_type_node * resolve() ENTT_NOEXCEPT;
};


template<typename... Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>...> {};


template<typename Op, typename Node>
void iterate(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr) {
        op(curr);
        curr = curr->next;
    }
}


template<auto Member, typename Op>
void iterate(Op op, const meta_type_node *node) ENTT_NOEXCEPT {
    if(node) {
        auto *curr = node->base;
        iterate(op, node->*Member);

        while(curr) {
            iterate<Member>(op, curr->type());
            curr = curr->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr && !op(curr)) {
        curr = curr->next;
    }

    return curr;
}


template<auto Member, typename Op>
auto find_if(Op op, const meta_type_node *node) ENTT_NOEXCEPT
-> decltype(find_if(op, node->*Member)) {
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *curr = node->base;

        while(curr && !ret) {
            ret = find_if<Member>(op, curr->type());
            curr = curr->next;
        }
    }

    return ret;
}


template<typename Type>
const Type * try_cast(const meta_type_node *node, void *instance) ENTT_NOEXCEPT {
    const auto *type = meta_info<Type>::resolve();
    void *ret = nullptr;

    if(node == type) {
        ret = instance;
    } else {
        const auto *base = find_if<&meta_type_node::base>([type](auto *candidate) {
            return candidate->type() == type;
        }, node);

        ret = base ? base->cast(instance) : nullptr;
    }

    return static_cast<const Type *>(ret);
}


template<auto Member>
inline bool can_cast_or_convert(const meta_type_node *from, const meta_type_node *to) ENTT_NOEXCEPT {
    return (from == to) || find_if<Member>([to](auto *node) {
        return node->type() == to;
    }, from);
}


template<typename... Args, std::size_t... Indexes>
inline auto ctor(std::index_sequence<Indexes...>, const meta_type_node *node) ENTT_NOEXCEPT {
    return internal::find_if([](auto *candidate) {
        return candidate->size == sizeof...(Args) &&
                (([](auto *from, auto *to) {
                    return internal::can_cast_or_convert<&internal::meta_type_node::base>(from, to)
                            || internal::can_cast_or_convert<&internal::meta_type_node::conv>(from, to);
                }(internal::meta_info<Args>::resolve(), candidate->arg(Indexes))) && ...);
    }, node->ctor);
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Meta any object.
 *
 * A meta any is an opaque container for single values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to
 * completely eliminate the need to allocate memory, where possible.<br/>
 * From the user's point of view, nothing will change, but the elimination of
 * allocations will reduce the jumps in memory and therefore will avoid chasing
 * of pointers. This will greatly improve the use of the cache, thus increasing
 * the overall performance.
 */
class meta_any {
    /*! @brief A meta handle is allowed to _inherit_ from a meta any. */
    friend struct meta_handle;

    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using compare_fn_type = bool(const void *, const void *);
    using copy_fn_type = void *(storage_type &, const void *);
    using destroy_fn_type = void(void *);
    using steal_fn_type = void *(storage_type &, void *, destroy_fn_type *);

    template<typename Type, typename = std::void_t<>>
    struct type_traits {
        template<typename... Args>
        static void * instance(storage_type &storage, Args &&... args) {
            auto instance = std::make_unique<Type>(std::forward<Args>(args)...);
            new (&storage) Type *{instance.get()};
            return instance.release();
        }

        static void destroy(void *instance) {
            auto *node = internal::meta_info<Type>::resolve();
            auto *actual = static_cast<Type *>(instance);
            [[maybe_unused]] const bool destroyed = node->meta().destroy(*actual);
            ENTT_ASSERT(destroyed);
            delete actual;
        }

        static void * copy(storage_type &storage, const void *other) {
            auto instance = std::make_unique<Type>(*static_cast<const Type *>(other));
            new (&storage) Type *{instance.get()};
            return instance.release();
        }

        static void * steal(storage_type &to, void *from, destroy_fn_type *) {
            auto *instance = static_cast<Type *>(from);
            new (&to) Type *{instance};
            return instance;
        }
    };

    template<typename Type>
    struct type_traits<Type, std::enable_if_t<sizeof(Type) <= sizeof(void *) && std::is_nothrow_move_constructible_v<Type>>> {
        template<typename... Args>
        static void * instance(storage_type &storage, Args &&... args) {
            return new (&storage) Type{std::forward<Args>(args)...};
        }

        static void destroy(void *instance) {
            auto *node = internal::meta_info<Type>::resolve();
            auto *actual = static_cast<Type *>(instance);
            [[maybe_unused]] const bool destroyed = node->meta().destroy(*actual);
            ENTT_ASSERT(destroyed);
            actual->~Type();
        }

        static void * copy(storage_type &storage, const void *instance) {
            return new (&storage) Type{*static_cast<const Type *>(instance)};
        }

        static void * steal(storage_type &to, void *from, destroy_fn_type *destroy_fn) {
            void *instance = new (&to) Type{std::move(*static_cast<Type *>(from))};
            destroy_fn(from);
            return instance;
        }
    };

    template<typename Type>
    static auto compare(int, const Type &lhs, const Type &rhs)
    -> decltype(lhs == rhs, bool{}) {
        return lhs == rhs;
    }

    template<typename Type>
    static bool compare(char, const Type &lhs, const Type &rhs) {
        return &lhs == &rhs;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{nullptr},
          node{nullptr},
          destroy_fn{nullptr},
          compare_fn{nullptr},
          copy_fn{nullptr},
          steal_fn{nullptr}
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
            using traits_type = type_traits<std::remove_cv_t<std::remove_reference_t<Type>>>;
            instance = traits_type::instance(storage, std::forward<Args>(args)...);
            destroy_fn = &traits_type::destroy;
            copy_fn = &traits_type::copy;
            steal_fn = &traits_type::steal;

            compare_fn = [](const void *lhs, const void *rhs) {
                return compare(0, *static_cast<const Type *>(lhs), *static_cast<const Type *>(rhs));
            };
        }
    }

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type>
    explicit meta_any(as_alias_t, Type &type)
        : meta_any{}
    {
        node = internal::meta_info<Type>::resolve();
        instance = &type;

        compare_fn = [](const void *lhs, const void *rhs) {
            return compare(0, *static_cast<const Type *>(lhs), *static_cast<const Type *>(rhs));
        };
    }

    /**
     * @brief Constructs a meta any from a given value.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&type)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(type)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other)
        : meta_any{}
    {
        node = other.node;
        instance = other.copy_fn ? other.copy_fn(storage, other.instance) : other.instance;
        destroy_fn = other.destroy_fn;
        compare_fn = other.compare_fn;
        copy_fn = other.copy_fn;
        steal_fn = other.steal_fn;
    }

    /**
     * @brief Move constructor.
     *
     * After meta any move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other) ENTT_NOEXCEPT
        : meta_any{}
    {
        swap(*this, other);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy_fn) {
            destroy_fn(instance);
        }
    }

    /**
     * @brief Assignment operator.
     * @tparam Type Type of object to use to initialize the container.
     * @param type An instance of an object to use to initialize the container.
     * @return This meta any object.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any & operator=(Type &&type) {
        return (*this = meta_any{std::forward<Type>(type)});
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to assign.
     * @return This meta any object.
     */
    meta_any & operator=(const meta_any &other) {
        return (*this = meta_any{other});
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to assign.
     * @return This meta any object.
     */
    meta_any & operator=(meta_any &&other) ENTT_NOEXCEPT {
        meta_any any{std::move(other)};
        swap(any, *this);
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
    const Type * try_cast() const ENTT_NOEXCEPT {
        return internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc try_cast */
    template<typename Type>
    Type * try_cast() ENTT_NOEXCEPT {
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
    const Type & cast() const ENTT_NOEXCEPT {
        auto *actual = try_cast<Type>();
        ENTT_ASSERT(actual);
        return *actual;
    }

    /*! @copydoc cast */
    template<typename Type>
    Type & cast() ENTT_NOEXCEPT {
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

        if(const auto *type = internal::meta_info<Type>::resolve(); node == type) {
            any = *static_cast<const Type *>(instance);
        } else {
            const auto *conv = internal::find_if<&internal::meta_type_node::conv>([type](auto *other) {
                return other->type() == type;
            }, node);

            if(conv) {
                any = conv->conv(instance);
            }
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
        bool valid = (node == internal::meta_info<Type>::resolve());

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
    void emplace(Args&& ... args) {
        *this = meta_any{std::in_place_type_t<Type>{}, std::forward<Args>(args)...};
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    bool operator==(const meta_any &other) const ENTT_NOEXCEPT {
        return node == other.node && ((!compare_fn && !other.compare_fn) || compare_fn(instance, other.instance));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) ENTT_NOEXCEPT {
        if(lhs.steal_fn && rhs.steal_fn) {
            storage_type buffer;
            auto *temp = lhs.steal_fn(buffer, lhs.instance, lhs.destroy_fn);
            lhs.instance = rhs.steal_fn(lhs.storage, rhs.instance, rhs.destroy_fn);
            rhs.instance = lhs.steal_fn(rhs.storage, temp, lhs.destroy_fn);
        } else if(lhs.steal_fn) {
            lhs.instance = lhs.steal_fn(rhs.storage, lhs.instance, lhs.destroy_fn);
            std::swap(rhs.instance, lhs.instance);
        } else if(rhs.steal_fn) {
            rhs.instance = rhs.steal_fn(lhs.storage, rhs.instance, rhs.destroy_fn);
            std::swap(rhs.instance, lhs.instance);
        } else {
            std::swap(lhs.instance, rhs.instance);
        }

        std::swap(lhs.node, rhs.node);
        std::swap(lhs.destroy_fn, rhs.destroy_fn);
        std::swap(lhs.compare_fn, rhs.compare_fn);
        std::swap(lhs.copy_fn, rhs.copy_fn);
        std::swap(lhs.steal_fn, rhs.steal_fn);
    }

private:
    storage_type storage;
    void *instance;
    internal::meta_type_node *node;
    destroy_fn_type *destroy_fn;
    compare_fn_type *compare_fn;
    copy_fn_type *copy_fn;
    steal_fn_type *steal_fn;
};


/**
 * @brief Meta handle object.
 *
 * A meta handle is an opaque pointer to an instance of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance. Users are
 * responsible for ensuring that the target object remains alive for the entire
 * interval of use of the handle.
 */
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle() ENTT_NOEXCEPT
        : node{nullptr},
          instance{nullptr}
    {}

    /**
     * @brief Constructs a meta handle from a meta any object.
     * @param any A reference to an object to use to initialize the handle.
     */
    meta_handle(meta_any &any) ENTT_NOEXCEPT
        : node{any.node},
          instance{any.instance}
    {}

    /**
     * @brief Constructs a meta handle from a given instance.
     * @tparam Type Type of object to use to initialize the handle.
     * @param obj A reference to an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_handle>>>
    meta_handle(Type &obj) ENTT_NOEXCEPT
        : node{internal::meta_info<Type>::resolve()},
          instance{&obj}
    {}

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
     * @return A (possibly null) pointer to the underlying object.
     */
    template<typename Type>
    const Type * data() const ENTT_NOEXCEPT {
        return internal::try_cast<Type>(node, instance);
    }

    /*! @copydoc data */
    template<typename Type>
    Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(std::as_const(*this).data<Type>());
    }

    /**
     * @brief Returns false if a handle is empty, true otherwise.
     * @return False if the handle is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return instance;
    }

private:
    const internal::meta_type_node *node;
    void *instance;
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


/**
 * @brief Meta property object.
 *
 * A meta property is an opaque container for a key/value pair.<br/>
 * Properties are associated with any other meta object to enrich it.
 */
class meta_prop {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_prop(const internal::meta_prop_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_prop() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    meta_any key() const ENTT_NOEXCEPT {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    meta_any value() const ENTT_NOEXCEPT {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_prop &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_prop_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_prop &lhs, const meta_prop &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta base object.
 *
 * A meta base is an opaque container for a base class to be used to walk
 * through hierarchies.
 */
class meta_base {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_base(const internal::meta_base_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_base() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta base belongs.
     * @return The meta type to which the meta base belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of a given meta base.
     * @return The meta type of the meta base.
     */
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
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_base &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_base_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_base &lhs, const meta_base &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta conversion function object.
 *
 * A meta conversion function is an opaque container for a conversion function
 * to be used to convert a given instance to another type.
 */
class meta_conv {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_conv(const internal::meta_conv_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_conv() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta conversion function belongs.
     * @return The meta type to which the meta conversion function belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of a given meta conversion function.
     * @return The meta type of the meta conversion function.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Converts an instance to a given type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    meta_any convert(const void *instance) const ENTT_NOEXCEPT {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_conv &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_conv_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_conv &lhs, const meta_conv &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta constructor object.
 *
 * A meta constructor is an opaque container for a function to be used to
 * construct instances of a given type.
 */
class meta_ctor {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_ctor(const internal::meta_ctor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /*! @brief Default constructor. */
    meta_ctor() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta constructor belongs.
     * @return The meta type to which the meta constructor belongs.
     */
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
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        if(sizeof...(Args) == size()) {
            any = node->invoke(arguments.data());
        }

        return any;
    }

    /**
     * @brief Iterates all the properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_ctor &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_ctor_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_ctor &lhs, const meta_ctor &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta destructor object.
 *
 * A meta destructor is an opaque container for a function to be used to
 * destroy instances of a given type.
 */
class meta_dtor {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_dtor(const internal::meta_dtor_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_dtor() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the meta type to which a meta destructor belongs.
     * @return The meta type to which the meta destructor belongs.
     */
    inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * destructor. Otherwise, invoking the meta destructor results in an
     * undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return True in case of success, false otherwise.
     */
    bool invoke(meta_handle handle) const {
        return node->invoke(handle);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_dtor &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_dtor_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_dtor &lhs, const meta_dtor &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta data object.
 *
 * A meta data is an opaque container for a data member associated with a given
 * type.
 */
class meta_data {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_data(const internal::meta_data_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Default constructor. */
    meta_data() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta data.
     * @return The identifier assigned to the meta data.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
    }

    /**
     * @brief Returns the meta type to which a meta data belongs.
     * @return The meta type to which the meta data belongs.
     */
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
     *
     * A static meta data is such that it can be accessed using a null pointer
     * as an instance.
     *
     * @return True if the meta data is static, false otherwise.
     */
    bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type of a given meta data.
     * @return The meta type of the meta data.
     */
    inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must coincide exactly with that of the variable
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle handle, Type &&value) const {
        return node->set(handle, meta_any{}, std::forward<Type>(value));
    }

    /**
     * @brief Sets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must coincide exactly with that of the array type
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param index Position of the underlying element to set.
     * @param value Parameter to use to set the underlying element.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle handle, std::size_t index, Type &&value) const {
        ENTT_ASSERT(index < node->type()->extent);
        return node->set(handle, index, std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    meta_any get(meta_handle handle) const ENTT_NOEXCEPT {
        return node->get(handle, meta_any{});
    }

    /**
     * @brief Gets the i-th element of an array enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param index Position of the underlying element to get.
     * @return A meta any containing the value of the underlying element.
     */
    meta_any get(meta_handle handle, std::size_t index) const ENTT_NOEXCEPT {
        ENTT_ASSERT(index < node->type()->extent);
        return node->get(handle, index);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_data &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_data_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_data &lhs, const meta_data &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta function object.
 *
 * A meta function is an opaque container for a member function associated with
 * a given type.
 */
class meta_func {
    /*! @brief A meta factory is allowed to create meta objects. */
    template<typename> friend class meta_factory;

    meta_func(const internal::meta_func_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @brief Default constructor. */
    meta_func() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta function.
     * @return The identifier assigned to the meta function.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
    }

    /**
     * @brief Returns the meta type to which a meta function belongs.
     * @return The meta type to which the meta function belongs.
     */
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
     *
     * A static meta function is such that it can be invoked using a null
     * pointer as an instance.
     *
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
     * To invoke a meta function, the types of the parameters must coincide
     * exactly with those required by the underlying function. Otherwise, an
     * empty and then invalid container is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle handle, Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        if(sizeof...(Args) == size()) {
            any = node->invoke(handle, arguments.data());
        }

        return any;
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node->prop);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_func &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_func_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_func &lhs, const meta_func &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta type object.
 *
 * A meta type is the starting point for accessing a reflected type, thus being
 * able to work through it on real objects.
 */
class meta_type {
    /*! @brief A meta node is allowed to create meta objects. */
    template<typename...> friend struct internal::meta_node;

    meta_type(const internal::meta_type_node *curr) ENTT_NOEXCEPT
        : node{curr}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @brief Default constructor. */
    meta_type() ENTT_NOEXCEPT
        : node{nullptr}
    {}

    /**
     * @brief Returns the identifier assigned to a given meta type.
     * @return The identifier assigned to the meta type.
     */
    ENTT_ID_TYPE identifier() const ENTT_NOEXCEPT {
        return node->identifier;
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
     * @brief Indicates whether a given meta type refers to a function type or
     * not.
     * @return True if the underlying type is a function, false otherwise.
     */
    bool is_function() const ENTT_NOEXCEPT {
        return node->is_function;
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
     * @brief Iterates all the meta base of a meta type.
     *
     * Iteratively returns **all** the base classes of the given type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_base>, void>
    base(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::base>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta base associated with a given identifier.
     *
     * Searches recursively among **all** the base classes of the given type.
     *
     * @param identifier Unique identifier.
     * @return The meta base associated with the given identifier, if any.
     */
    meta_base base(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::base>([identifier](auto *candidate) {
            return candidate->type()->identifier == identifier;
        }, node);

        return curr ? curr->meta() : meta_base{};
    }

    /**
     * @brief Iterates all the meta conversion functions of a meta type.
     *
     * Iteratively returns **all** the meta conversion functions of the given
     * type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void conv(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::conv>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta conversion function associated with a given type.
     *
     * Searches recursively among **all** the conversion functions of the given
     * type.
     *
     * @tparam Type The type to use to search for a meta conversion function.
     * @return The meta conversion function associated with the given type, if
     * any.
     */
    template<typename Type>
    meta_conv conv() const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::conv>([type = internal::meta_info<Type>::resolve()](auto *candidate) {
            return candidate->type() == type;
        }, node);

        return curr ? curr->meta() : meta_conv{};
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    void ctor(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node->ctor);
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    meta_ctor ctor() const ENTT_NOEXCEPT {
        const auto *curr = internal::ctor<Args...>(std::make_index_sequence<sizeof...(Args)>{}, node);
        return curr ? curr->meta() : meta_ctor{};
    }

    /**
     * @brief Returns the meta destructor associated with a given type.
     * @return The meta destructor associated with the given type, if any.
     */
    meta_dtor dtor() const ENTT_NOEXCEPT {
        return node->dtor ? node->dtor->meta() : meta_dtor{};
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     *
     * Iteratively returns **all** the meta data of the given type. This means
     * that the meta data of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_data>, void>
    data(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::data>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta data associated with a given identifier.
     *
     * Searches recursively among **all** the meta data of the given type. This
     * means that the meta data of the base classes will also be inspected, if
     * any.
     *
     * @param identifier Unique identifier.
     * @return The meta data associated with the given identifier, if any.
     */
    meta_data data(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::data>([identifier](auto *candidate) {
            return candidate->identifier == identifier;
        }, node);

        return curr ? curr->meta() : meta_data{};
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     *
     * Iteratively returns **all** the meta functions of the given type. This
     * means that the meta functions of the base classes will also be returned,
     * if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_func>, void>
    func(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::func>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the meta function associated with a given identifier.
     *
     * Searches recursively among **all** the meta functions of the given type.
     * This means that the meta functions of the base classes will also be
     * inspected, if any.
     *
     * @param identifier Unique identifier.
     * @return The meta function associated with the given identifier, if any.
     */
    meta_func func(const ENTT_ID_TYPE identifier) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::func>([identifier](auto *candidate) {
            return candidate->identifier == identifier;
        }, node);

        return curr ? curr->meta() : meta_func{};
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any construct(Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{{std::forward<Args>(args)...}};
        meta_any any{};

        internal::find_if<&internal::meta_type_node::ctor>([data = arguments.data(), &any](auto *curr) -> bool {
            if(curr->size == sizeof...(args)) {
                any = curr->invoke(data);
            }

            return static_cast<bool>(any);
        }, node);

        return any;
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the underlying type.
     * Otherwise, invoking the meta destructor results in an undefined
     * behavior.<br/>
     * If no destructor has been set, this function returns true without doing
     * anything.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return True in case of success, false otherwise.
     */
    bool destroy(meta_handle handle) const {
        return (handle.type() == node->meta()) && (!node->dtor || node->dtor->invoke(handle));
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     *
     * Iteratively returns **all** the properties of the given type. This means
     * that the properties of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::prop>([op = std::move(op)](auto *curr) {
            op(curr->meta());
        }, node);
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * Searches recursively among **all** the properties of the given type. This
     * means that the properties of the base classes will also be inspected, if
     * any.
     *
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    std::enable_if_t<!std::is_invocable_v<Key, meta_prop>, meta_prop>
    prop(Key &&key) const ENTT_NOEXCEPT {
        const auto *curr = internal::find_if<&internal::meta_type_node::prop>([key = meta_any{std::forward<Key>(key)}](auto *candidate) {
            return candidate->key() == key;
        }, node);

        return curr ? curr->meta() : meta_prop{};
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        return node;
    }

    /**
     * @brief Checks if two meta objects refer to the same node.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same node, false
     * otherwise.
     */
    bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return node == other.node;
    }

private:
    const internal::meta_type_node *node;
};


/**
 * @brief Checks if two meta objects refer to the same node.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return True if the two meta objects refer to the same node, false otherwise.
 */
inline bool operator!=(const meta_type &lhs, const meta_type &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


inline meta_type meta_any::type() const ENTT_NOEXCEPT {
    return node ? node->meta() : meta_type{};
}


inline meta_type meta_handle::type() const ENTT_NOEXCEPT {
    return node ? node->meta() : meta_type{};
}


inline meta_type meta_base::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_base::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_conv::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_conv::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index)->meta() : meta_type{};
}


inline meta_type meta_dtor::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_data::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_data::type() const ENTT_NOEXCEPT {
    return node->type()->meta();
}


inline meta_type meta_func::parent() const ENTT_NOEXCEPT {
    return node->parent->meta();
}


inline meta_type meta_func::ret() const ENTT_NOEXCEPT {
    return node->ret()->meta();
}


inline meta_type meta_func::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index)->meta() : meta_type{};
}


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Type>
inline meta_type_node * meta_node<Type>::resolve() ENTT_NOEXCEPT {
    if(!type) {
        static meta_type_node node{
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_array_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_function_v<Type>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            std::extent_v<Type>,
            []() ENTT_NOEXCEPT -> meta_type {
                return internal::meta_info<std::remove_pointer_t<Type>>::resolve();
            },
            []() ENTT_NOEXCEPT -> meta_type {
                return &node;
            }
        };

        type = &node;
    }

    return type;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


}


#endif // ENTT_META_META_HPP
