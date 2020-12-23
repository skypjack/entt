#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <functional>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/utility.hpp"
#include "../core/type_info.hpp"
#include "ctx.hpp"
#include "internal.hpp"
#include "range.hpp"
#include "type_traits.hpp"


namespace entt {


class meta_type;
class meta_any;


/*! @brief Proxy object for sequence containers. */
class meta_sequence_container {
    template<typename>
    struct meta_sequence_container_proxy;

    class meta_iterator;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_sequence_container() ENTT_NOEXCEPT = default;

    /**
     * @brief Construct a proxy object for sequence containers.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    meta_sequence_container(std::in_place_type_t<Type>, any instance) ENTT_NOEXCEPT
        : value_type_fn{&meta_sequence_container_proxy<Type>::value_type},
          size_fn{&meta_sequence_container_proxy<Type>::size},
          resize_fn{&meta_sequence_container_proxy<Type>::resize},
          clear_fn{&meta_sequence_container_proxy<Type>::clear},
          begin_fn{&meta_sequence_container_proxy<Type>::begin},
          end_fn{&meta_sequence_container_proxy<Type>::end},
          insert_fn{&meta_sequence_container_proxy<Type>::insert},
          erase_fn{&meta_sequence_container_proxy<Type>::erase},
          get_fn{&meta_sequence_container_proxy<Type>::get},
          storage{std::move(instance)}
    {}

    [[nodiscard]] inline meta_type value_type() const ENTT_NOEXCEPT;
    [[nodiscard]] inline size_type size() const ENTT_NOEXCEPT;
    inline bool resize(size_type);
    inline bool clear();
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline std::pair<iterator, bool> insert(iterator, meta_any);
    inline std::pair<iterator, bool> erase(iterator);
    [[nodiscard]] inline meta_any operator[](size_type);
    [[nodiscard]] inline explicit operator bool() const ENTT_NOEXCEPT;

private:
    meta_type(* value_type_fn)() ENTT_NOEXCEPT = nullptr;
    size_type(* size_fn)(const any &) ENTT_NOEXCEPT = nullptr;
    bool(* resize_fn)(any &, size_type) = nullptr;
    bool(* clear_fn)(any &) = nullptr;
    iterator(* begin_fn)(any &) = nullptr;
    iterator(* end_fn)(any &) = nullptr;
    std::pair<iterator, bool>(* insert_fn)(any &, iterator, meta_any &) = nullptr;
    std::pair<iterator, bool>(* erase_fn)(any &, iterator) = nullptr;
    meta_any(* get_fn)(any &, size_type) = nullptr;
    any storage{};
};


/*! @brief Proxy object for associative containers. */
class meta_associative_container {
    template<typename>
    struct meta_associative_container_proxy;

    class meta_iterator;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_associative_container() ENTT_NOEXCEPT = default;

    /**
     * @brief Construct a proxy object for associative containers.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    meta_associative_container(std::in_place_type_t<Type>, any instance) ENTT_NOEXCEPT
        : key_only_container{is_key_only_meta_associative_container_v<Type>},
          key_type_fn{&meta_associative_container_proxy<Type>::key_type},
          mapped_type_fn{&meta_associative_container_proxy<Type>::mapped_type},
          value_type_fn{&meta_associative_container_proxy<Type>::value_type},
          size_fn{&meta_associative_container_proxy<Type>::size},
          clear_fn{&meta_associative_container_proxy<Type>::clear},
          begin_fn{&meta_associative_container_proxy<Type>::begin},
          end_fn{&meta_associative_container_proxy<Type>::end},
          insert_fn{&meta_associative_container_proxy<Type>::insert},
          erase_fn{&meta_associative_container_proxy<Type>::erase},
          find_fn{&meta_associative_container_proxy<Type>::find},
          storage{std::move(instance)}
    {}

    [[nodiscard]] inline bool key_only() const ENTT_NOEXCEPT;
    [[nodiscard]] inline meta_type key_type() const ENTT_NOEXCEPT;
    [[nodiscard]] inline meta_type mapped_type() const ENTT_NOEXCEPT;
    [[nodiscard]] inline meta_type value_type() const ENTT_NOEXCEPT;
    [[nodiscard]] inline size_type size() const ENTT_NOEXCEPT;
    inline bool clear();
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline bool insert(meta_any, meta_any);
    inline bool erase(meta_any);
    [[nodiscard]] inline iterator find(meta_any);
    [[nodiscard]] inline explicit operator bool() const ENTT_NOEXCEPT;

private:
    bool key_only_container{};
    meta_type(* key_type_fn)() ENTT_NOEXCEPT = nullptr;
    meta_type(* mapped_type_fn)() ENTT_NOEXCEPT = nullptr;
    meta_type(* value_type_fn)() ENTT_NOEXCEPT = nullptr;
    size_type(* size_fn)(const any &) ENTT_NOEXCEPT = nullptr;
    bool(* clear_fn)(any &) = nullptr;
    iterator(* begin_fn)(any &) = nullptr;
    iterator(* end_fn)(any &) = nullptr;
    bool(* insert_fn)(any &, meta_any &, meta_any &) = nullptr;
    bool(* erase_fn)(any &, meta_any &) = nullptr;
    iterator(* find_fn)(any &, meta_any &) = nullptr;
    any storage{};
};


/*! @brief Opaque wrapper for values of any type. */
class meta_any {
    enum class operation { DEREF, CDEREF, SEQ, CSEQ, ASSOC, CASSOC };

    using vtable_type = void(const operation, const any &, void *);

    template<typename Type>
    static void basic_vtable(const operation op, [[maybe_unused]] const any &from, [[maybe_unused]] void *to) {
        switch(op) {
        case operation::DEREF:
            if constexpr(is_meta_pointer_like_v<Type>) {
                *static_cast<meta_any *>(to) = std::reference_wrapper{*any_cast<const Type>(from)};
            }
            break;
        case operation::CDEREF:
            if constexpr(is_meta_pointer_like_v<Type>) {
                *static_cast<meta_any *>(to) = std::cref(*any_cast<const Type>(from));
            }
            break;
        case operation::SEQ:
            if constexpr(has_meta_sequence_container_traits_v<Type>) {
                *static_cast<meta_sequence_container *>(to) = { std::in_place_type<Type>, as_ref(const_cast<any &>(from)) };
            }
            break;
        case operation::CSEQ:
            if constexpr(has_meta_sequence_container_traits_v<Type>) {
                *static_cast<meta_sequence_container *>(to) = { std::in_place_type<Type>, as_ref(from) };
            }
            break;
        case operation::ASSOC:
            if constexpr(has_meta_associative_container_traits_v<Type>) {
                *static_cast<meta_associative_container *>(to) = { std::in_place_type<Type>, as_ref(const_cast<any &>(from)) };
            }
            break;
        case operation::CASSOC:
            if constexpr(has_meta_associative_container_traits_v<Type>) {
                *static_cast<meta_associative_container *>(to) = { std::in_place_type<Type>, as_ref(from) };
            }
            break;
        }
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          vtable{},
          node{}
    {}

    /**
     * @brief Constructs a meta any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, Args &&... args)
        : storage(std::in_place_type<Type>, std::forward<Args>(args)...),
          vtable{&basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>},
          node{internal::meta_info<std::remove_const_t<std::remove_reference_t<Type>>>::resolve()}
    {}

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    meta_any(std::reference_wrapper<Type> value)
        : meta_any{std::in_place_type<Type &>, &value.get()}
    {}

    /**
     * @brief Constructs a meta any from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other) = default;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other) ENTT_NOEXCEPT
        : meta_any{}
    {
        swap(*this, other);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(node && node->dtor) {
            node->dtor(storage.data());
        }
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
    [[nodiscard]] inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void * data() const ENTT_NOEXCEPT {
        return storage.data();
    }

    /*! @copydoc data */
    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return storage.data();
    }

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * @sa invoke
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param id Unique identifier.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(const id_type id, Args &&... args) const;

    /*! @copydoc invoke */
    template<typename... Args>
    meta_any invoke(const id_type id, Args &&... args);

    /**
     * @brief Sets the value of a given variable.
     *
     * The type of the value must be such that a cast or conversion to the type
     * of the variable is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param id Unique identifier.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(const id_type id, Type &&value);

    /**
     * @brief Gets the value of a given variable.
     * @param id Unique identifier.
     * @return A meta any containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(const id_type id) const;

    /*! @copydoc get */
    [[nodiscard]] meta_any get(const id_type id);

    /**
     * @brief Tries to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return A (possibly null) pointer to the contained instance.
     */
    template<typename Type>
    [[nodiscard]] const Type * try_cast() const {
        if(node) {
            if(const auto info = internal::meta_info<Type>::resolve()->info; node->info == info) {
                return any_cast<Type>(&storage);
            } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([info](const auto *curr) { return curr->type()->info == info; }, node); base) {
                return static_cast<const Type *>(base->cast(storage.data()));
            }
        }

        return nullptr;
    }

    /*! @copydoc try_cast */
    template<typename Type>
    [[nodiscard]] Type * try_cast() {
        if(node) {
            if(const auto info = internal::meta_info<Type>::resolve()->info; node->info == info) {
                return any_cast<Type>(&storage);
            } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([info](const auto *curr) { return curr->type()->info == info; }, node); base) {
                return static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(base->cast(static_cast<constness_as_t<any, Type> &>(storage).data())));
            }
        }
        
        return nullptr;
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the cast is possible.
     *
     * @warning
     * Attempting to perform a cast that isn't viable results in undefined
     * behavior.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    [[nodiscard]] Type cast() const {
        auto * const actual = try_cast<std::remove_reference_t<Type>>();
        ENTT_ASSERT(actual);
        return static_cast<Type>(*actual);
    }

    /*! @copydoc cast */
    template<typename Type>
    [[nodiscard]] Type cast() {
        // forces const on non-reference types to make them work also with wrappers for const references
        auto * const actual = try_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>();
        ENTT_ASSERT(actual);
        return static_cast<Type>(*actual);
    }

    /**
     * @brief Tries to make an instance castable to a certain type.
     * @tparam Type Type to which the cast is requested.
     * @return A valid meta any object if there exists a a viable conversion
     * that makes the cast possible, an invalid object otherwise.
     */
    template<typename Type>
    [[nodiscard]] meta_any allow_cast() const {
        if(try_cast<std::remove_reference_t<Type>>() != nullptr) {
            return as_ref(*this);
        } else if(node) {
            if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([info = internal::meta_info<Type>::resolve()->info](const auto *curr) {
                return curr->type()->info == info;
            }, node); conv) {
                return conv->conv(storage.data());
            }
        }

        return {};
    }

    /**
     * @brief Tries to make an instance castable to a certain type.
     * @tparam Type Type to which the cast is requested.
     * @return True if there exists a a viable conversion that makes the cast
     * possible, false otherwise.
     */
    template<typename Type>
    bool allow_cast() {
        if(try_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>() != nullptr) {
            return true;
        } else if(node) {
            if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([info = internal::meta_info<Type>::resolve()->info](const auto *curr) {
                return curr->type()->info == info;
            }, node); conv) {
                auto other = conv->conv(std::as_const(storage).data());
                swap(other, *this);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = meta_any{std::in_place_type<Type>, std::forward<Args>(args)...};
    }

    /**
     * @brief Returns a sequence container proxy.
     * @return A sequence container proxy for the underlying object.
     */
    [[nodiscard]] meta_sequence_container as_sequence_container() ENTT_NOEXCEPT {
        meta_sequence_container proxy;
        vtable(operation::SEQ, storage, &proxy);
        return proxy;
    }

    /*! @copydoc as_sequence_container */
    [[nodiscard]] meta_sequence_container as_sequence_container() const ENTT_NOEXCEPT {
        meta_sequence_container proxy;
        vtable(operation::CSEQ, storage, &proxy);
        return proxy;
    }

    /**
     * @brief Returns an associative container proxy.
     * @return An associative container proxy for the underlying object.
     */
    [[nodiscard]] meta_associative_container as_associative_container() ENTT_NOEXCEPT {
        meta_associative_container proxy;
        vtable(operation::ASSOC, storage, &proxy);
        return proxy;
    }

    /*! @copydoc as_associative_container */
    [[nodiscard]] meta_associative_container as_associative_container() const ENTT_NOEXCEPT {
        meta_associative_container proxy;
        vtable(operation::CASSOC, storage, &proxy);
        return proxy;
    }

    /**
     * @brief Indirection operator for dereferencing opaque objects.
     * @return A meta any that shares a reference to an unmanaged object if the
     * wrapped element is dereferenceable, an invalid meta any otherwise.
     */
    [[nodiscard]] meta_any operator*() ENTT_NOEXCEPT {
        meta_any any{};
        vtable(operation::DEREF, storage, &any);
        return any;
    }

    /*! @copydoc operator* */
    [[nodiscard]] meta_any operator*() const ENTT_NOEXCEPT {
        meta_any any{};
        vtable(operation::CDEREF, storage, &any);
        return any;
    }

    /**
     * @brief Returns false if a wrapper is invalid, true otherwise.
     * @return False if the wrapper is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    [[nodiscard]] bool operator==(const meta_any &other) const {
        return (node == other.node) && (storage == other.storage);
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) {
        using std::swap;
        swap(lhs.storage, rhs.storage);
        swap(lhs.vtable, rhs.vtable);
        swap(lhs.node, rhs.node);
    }

    /**
     * @brief Aliasing constructor.
     * @param other A reference to an object that isn't necessarily initialized.
     * @return A meta any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] friend meta_any as_ref(meta_any &other) ENTT_NOEXCEPT {
        meta_any ref = as_ref(std::as_const(other));
        ref.storage = as_ref(other.storage);
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] friend meta_any as_ref(const meta_any &other) ENTT_NOEXCEPT {
        meta_any ref{};
        ref.node = other.node;
        ref.storage = as_ref(other.storage);
        ref.vtable = other.vtable;
        return ref;
    }

private:
    any storage;
    vtable_type *vtable;
    internal::meta_type_node *node;
};


/**
 * @brief Checks if two wrappers differ in their content.
 * @param lhs A meta any object, either empty or not.
 * @param rhs A meta any object, either empty or not.
 * @return True if the two wrappers differ in their content, false otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_any &lhs, const meta_any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Opaque pointers to instances of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance.<br/>
 * Handles are used to generate meta references to actual objects when needed.
 */
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle() = default;

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @tparam Type Type of object to use to initialize the handle.
     * @param value An instance of an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_handle>>>
    meta_handle(Type &value) ENTT_NOEXCEPT
        : meta_handle{}
    {
        if constexpr(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>) {
            any = as_ref(value);
        } else {
            any = std::reference_wrapper{value};
        }
    }

    /**
     * @brief Dereference operator for accessing the contained opaque object.
     * @return A meta any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any operator*() const {
        return any;
    }

    /**
     * @brief Access operator for accessing the contained opaque object.
     * @return A meta any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any * operator->() {
        return &any;
    }

    /**
     * @brief Access operator for accessing the contained opaque object.
     * @return A meta any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] const meta_any * operator->() const {
        return &any;
    }

private:
    meta_any any;
};


/*! @brief Opaque wrapper for meta properties of any type. */
struct meta_prop {
    /*! @brief Node type. */
    using node_type = internal::meta_prop_node;

    /**
     * @brief Constructs an instance from a given node.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_prop(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the property.
     */
    [[nodiscard]] meta_any key() const {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the property.
     */
    [[nodiscard]] meta_any value() const {
        return node->value();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta base classes. */
struct meta_base {
    /*! @brief Node type. */
    using node_type = internal::meta_base_node;

    /*! @copydoc meta_prop::meta_prop */
    meta_base(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the meta type to which a meta object belongs.
     * @return The meta type to which the meta object belongs.
     */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    [[nodiscard]] inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Casts an instance from a parent type to a base type.
     * @param instance The instance to cast.
     * @return An opaque pointer to the base type.
     */
    [[nodiscard]] const void * cast(const void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta conversion functions. */
struct meta_conv {
    /*! @brief Node type. */
    using node_type = internal::meta_conv_node;

    /*! @copydoc meta_prop::meta_prop */
    meta_conv(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /*! @copydoc meta_any::type */
    [[nodiscard]] inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Converts an instance to the underlying type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    [[nodiscard]] meta_any convert(const void *instance) const {
        return node->conv(instance);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta constructors. */
struct meta_ctor {
    /*! @brief Node type. */
    using node_type = internal::meta_ctor_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_ctor(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_base::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta constructor.
     * @return The number of arguments accepted by the meta constructor.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta constructor.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta constructor, if any.
     */
    [[nodiscard]] meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    [[nodiscard]] meta_any invoke(meta_any * const args, const size_type sz) const {
        return sz == size() ? node->invoke(args) : meta_any{};
    }

    /**
     * @copybrief invoke
     *
     * @sa invoke
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any invoke([[maybe_unused]] Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return invoke(arguments.data(), sizeof...(Args));
    }

    /**
     * @brief Returns a range to use to visit all meta properties.
     * @return An iterable range to use to visit all meta properties.
     */
    [[nodiscard]] meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        internal::meta_range range{node->prop};
        return std::find_if(range.begin(), range.end(), [&key](const auto &curr) { return curr.key() == key; }).operator->();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta data. */
struct meta_data {
    /*! @brief Node type. */
    using node_type = internal::meta_data_node;

    /*! @copydoc meta_prop::meta_prop */
    meta_data(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::id */
    [[nodiscard]] id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /*! @copydoc meta_base::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Indicates whether a meta data is constant or not.
     * @return True if the meta data is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const ENTT_NOEXCEPT {
        return (node->set == nullptr);
    }

    /**
     * @brief Indicates whether a meta data is static or not.
     * @return True if the meta data is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /*! @copydoc meta_any::type */
    [[nodiscard]] inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of a given variable.
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
        return node->set && node->set(std::move(instance), std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(meta_handle instance) const {
        return node->get(std::move(instance));
    }

    /*! @copydoc meta_ctor::prop */
    [[nodiscard]] meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        internal::meta_range range{node->prop};
        return std::find_if(range.begin(), range.end(), [&key](const auto &curr) { return curr.key() == key; }).operator->();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta functions. */
struct meta_func {
    /*! @brief Node type. */
    using node_type = internal::meta_func_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_func(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /*! @copydoc meta_type::id */
    [[nodiscard]] id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /*! @copydoc meta_base::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a meta function.
     * @return The number of arguments accepted by the meta function.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Indicates whether a meta function is constant or not.
     * @return True if the meta function is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a meta function is static or not.
     * @return True if the meta function is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type of the return type of a meta function.
     * @return The meta type of the return type of the meta function.
     */
    [[nodiscard]] inline meta_type ret() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the meta type of the i-th argument of a meta function.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta function, if any.
     */
    [[nodiscard]] inline meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a meta function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    meta_any invoke(meta_handle instance, meta_any * const args, const size_type sz) const {
        return sz == size() ? node->invoke(std::move(instance), args) : meta_any{};
    }

    /**
     * @copybrief invoke
     *
     * @sa invoke
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return invoke(std::move(instance), arguments.data(), sizeof...(Args));
    }

    /*! @copydoc meta_ctor::prop */
    [[nodiscard]] meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Returns the property associated with a given key.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        internal::meta_range range{node->prop};
        return std::find_if(range.begin(), range.end(), [&key](const auto &curr) { return curr.key() == key; }).operator->();
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for meta types. */
class meta_type {
    bool can_cast_or_convert(const meta_type type, const type_info info) const ENTT_NOEXCEPT {
        for(auto curr: type.conv()) {
            if(curr.type().info() == info) {
                return true;
            }
        }

        for(auto curr: type.base()) {
            if(curr.type().info() == info || can_cast_or_convert(curr.type(), info)) {
                return true;
            }
        }

        return false;
    }

public:
    /*! @brief Node type. */
    using node_type = internal::meta_type_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_type(node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the type info object of the underlying type.
     * @return The type info object of the underlying type.
     */
    [[nodiscard]] type_info info() const ENTT_NOEXCEPT {
        return node->info;
    }

    /**
     * @brief Returns the identifier assigned to a meta object.
     * @return The identifier assigned to the meta object.
     */
    [[nodiscard]] id_type id() const ENTT_NOEXCEPT {
        return node->id;
    }

    /**
     * @brief Returns the size of the underlying type if known.
     * @return The size of the underlying type if known, 0 otherwise.
     */
    [[nodiscard]] size_type size_of() const ENTT_NOEXCEPT {
        return node->size_of;
    }

    /**
     * @brief Checks whether a type refers to void or not.
     * @return True if the underlying type is void, false otherwise.
     */
    [[nodiscard]] bool is_void() const ENTT_NOEXCEPT {
        return node->is_void;
    }

    /**
     * @brief Checks whether a type refers to an integral type or not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    [[nodiscard]] bool is_integral() const ENTT_NOEXCEPT {
        return node->is_integral;
    }

    /**
     * @brief Checks whether a type refers to a floating-point type or not.
     * @return True if the underlying type is a floating-point type, false
     * otherwise.
     */
    [[nodiscard]] bool is_floating_point() const ENTT_NOEXCEPT {
        return node->is_floating_point;
    }

    /**
     * @brief Checks whether a type refers to an array type or not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    [[nodiscard]] bool is_array() const ENTT_NOEXCEPT {
        return node->is_array;
    }

    /**
     * @brief Checks whether a type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    [[nodiscard]] bool is_enum() const ENTT_NOEXCEPT {
        return node->is_enum;
    }

    /**
     * @brief Checks whether a type refers to an union or not.
     * @return True if the underlying type is an union, false otherwise.
     */
    [[nodiscard]] bool is_union() const ENTT_NOEXCEPT {
        return node->is_union;
    }

    /**
     * @brief Checks whether a type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    [[nodiscard]] bool is_class() const ENTT_NOEXCEPT {
        return node->is_class;
    }

    /**
     * @brief Checks whether a type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    [[nodiscard]] bool is_pointer() const ENTT_NOEXCEPT {
        return node->is_pointer;
    }

    /**
     * @brief Checks whether a type refers to a function pointer or not.
     * @return True if the underlying type is a function pointer, false
     * otherwise.
     */
    [[nodiscard]] bool is_function_pointer() const ENTT_NOEXCEPT {
        return node->is_function_pointer;
    }

    /**
     * @brief Checks whether a type refers to a pointer to data member or not.
     * @return True if the underlying type is a pointer to data member, false
     * otherwise.
     */
    [[nodiscard]] bool is_member_object_pointer() const ENTT_NOEXCEPT {
        return node->is_member_object_pointer;
    }

    /**
     * @brief Checks whether a type refers to a pointer to member function or
     * not.
     * @return True if the underlying type is a pointer to member function,
     * false otherwise.
     */
    [[nodiscard]] bool is_member_function_pointer() const ENTT_NOEXCEPT {
        return node->is_member_function_pointer;
    }

    /**
     * @brief Checks whether a type is a pointer-like type or not.
     * @return True if the underlying type is a pointer-like one, false
     * otherwise.
     */
    [[nodiscard]] bool is_pointer_like() const ENTT_NOEXCEPT {
        return node->is_pointer_like;
    }

    /**
     * @brief Checks whether a type refers to a sequence container or not.
     * @return True if the underlying type is a sequence container, false
     * otherwise.
     */
    [[nodiscard]] bool is_sequence_container() const ENTT_NOEXCEPT {
        return node->is_sequence_container;
    }

    /**
     * @brief Checks whether a type refers to an associative container or not.
     * @return True if the underlying type is an associative container, false
     * otherwise.
     */
    [[nodiscard]] bool is_associative_container() const ENTT_NOEXCEPT {
        return node->is_associative_container;
    }

    /**
     * @brief If a type refers to an array type, provides the number of
     * dimensions of the array.
     * @return The number of dimensions of the array if the underlying type is
     * an array type, 0 otherwise.
     */
    [[nodiscard]] size_type rank() const ENTT_NOEXCEPT {
        return node->rank;
    }

    /**
     * @brief If a type refers to an array type, provides the number of elements
     * along the given dimension of the array.
     * @param dim The dimension of which to return the number of elements.
     * @return The number of elements along the given dimension of the array if
     * the underlying type is an array type, 0 otherwise.
     */
    [[nodiscard]] size_type extent(size_type dim = {}) const ENTT_NOEXCEPT {
        return node->extent(dim);
    }

    /**
     * @brief Provides the meta type for which the pointer is defined.
     * @return The meta type for which the pointer is defined or this meta type
     * if it doesn't refer to a pointer type.
     */
    [[nodiscard]] meta_type remove_pointer() const ENTT_NOEXCEPT {
        return node->remove_pointer();
    }

    /**
     * @brief Provides the meta type for which the array is defined.
     * @return The meta type for which the array is defined or this meta type
     * if it doesn't refer to an array type.
     */
    [[nodiscard]] meta_type remove_extent() const ENTT_NOEXCEPT {
        return node->remove_extent();
    }

    /**
     * @brief Returns a range to use to visit top-level meta bases.
     * @return An iterable range to use to visit top-level meta bases.
     */
    [[nodiscard]] meta_range<meta_base> base() const ENTT_NOEXCEPT {
        return node->base;
    }

    /**
     * @brief Returns the meta base associated with a given identifier.
     * @param id Unique identifier.
     * @return The meta base associated with the given identifier, if any.
     */
    [[nodiscard]] meta_base base(const id_type id) const {
        return internal::find_if<&node_type::base>([id](const auto *curr) {
            return curr->type()->id == id;
        }, node);
    }

    /**
     * @brief Returns a range to use to visit top-level meta conversion
     * functions.
     * @return An iterable range to use to visit top-level meta conversion
     * functions.
     */
    [[nodiscard]] meta_range<meta_conv> conv() const ENTT_NOEXCEPT {
        return node->conv;
    }

    /**
     * @brief Returns the meta conversion function associated with a given type.
     * @tparam Type The type to use to search for a meta conversion function.
     * @return The meta conversion function associated with the given type, if
     * any.
     */
    template<typename Type>
    [[nodiscard]] meta_conv conv() const {
        return internal::find_if<&node_type::conv>([info = internal::meta_info<Type>::resolve()->info](const auto *curr) {
            return curr->type()->info == info;
        }, node);
    }

    /**
     * @brief Returns a range to use to visit top-level meta constructors.
     * @return An iterable range to use to visit top-level meta constructors.
     */
    [[nodiscard]] meta_range<meta_ctor> ctor() const ENTT_NOEXCEPT {
        return node->ctor;
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_ctor ctor() const {
        for(const auto &candidate: internal::meta_range{node->ctor}) {
            if(size_type index{}; candidate.size == sizeof...(Args) && ([this](auto *from, auto *to) { return from->info == to->info || can_cast_or_convert(from, to->info); }(internal::meta_info<Args>::resolve(), candidate.arg(index++)) && ...)) {
                return &candidate;
            }
        }

        return nullptr;
    }

    /**
     * @brief Returns a range to use to visit top-level meta data.
     * @return An iterable range to use to visit top-level meta data.
     */
    [[nodiscard]] meta_range<meta_data> data() const ENTT_NOEXCEPT {
        return node->data;
    }

    /**
     * @brief Returns the meta data associated with a given identifier.
     *
     * The meta data of the base classes will also be visited, if any.
     *
     * @param id Unique identifier.
     * @return The meta data associated with the given identifier, if any.
     */
    [[nodiscard]] meta_data data(const id_type id) const {
        return internal::find_if<&node_type::data>([id](const auto *curr) {
            return curr->id == id;
        }, node);
    }

    /**
     * @brief Returns a range to use to visit top-level meta functions.
     * @return An iterable range to use to visit top-level meta functions.
     */
    [[nodiscard]] meta_range<meta_func> func() const ENTT_NOEXCEPT {
        return node->func;
    }

    /**
     * @brief Returns the meta function associated with a given identifier.
     *
     * The meta functions of the base classes will also be visited, if any.<br/>
     * In the case of overloaded meta functions, the first one with the required
     * id will be returned.
     *
     * @param id Unique identifier.
     * @return The meta function associated with the given identifier, if any.
     */
    [[nodiscard]] meta_func func(const id_type id) const {
        return internal::find_if<&node_type::func>([id](const auto *curr) {
            return curr->id == id;
        }, node);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    [[nodiscard]] meta_any construct(meta_any * const args, const size_type sz) const {
        meta_any any{};

        internal::find_if<&node_type::ctor>([args, sz, &any](const auto *curr) {
            return (curr->size == sz) && (any = curr->invoke(args));
        }, node);

        return any;
    }

    /**
     * @copybrief construct
     *
     * @sa construct
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any construct(Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return construct(arguments.data(), sizeof...(Args));
    }

    /**
     * @brief Invokes the function with the given identifier, if possible.
     *
     * To invoke a meta function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.<br/>
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    meta_any invoke(const id_type id, meta_handle instance, meta_any * const args, const size_type sz) const {
        const internal::meta_func_node* candidate{};
        size_type extent{sz + 1u};
        bool ambiguous{};

        for(auto *it = internal::find_if<&node_type::func>([id, sz](const auto *curr) { return curr->id == id && curr->size == sz; }, node); it && it->id == id && it->size == sz; it = it->next) {
            size_type direct{};
            size_type ext{};

            for(size_type next{}; next < sz && next == (direct + ext); ++next) {
                const auto type = args[next].type();
                const auto req = it->arg(next)->info;
                type.info() == req ? ++direct : (ext += can_cast_or_convert(type, req));
            }

            if((direct + ext) == sz) {
                if(ext < extent) {
                    candidate = it;
                    extent = ext;
                    ambiguous = false;
                } else if(ext == extent) {
                    ambiguous = true;
                }
            }
        }

        return (candidate && !ambiguous) ? candidate->invoke(instance, args) : meta_any{};
    }

    /**
     * @copybrief invoke
     *
     * @sa invoke
     *
     * @param id Unique identifier.
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(const id_type id, meta_handle instance, Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return invoke(id, std::move(instance), arguments.data(), sizeof...(Args));
    }

    /**
     * @brief Sets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must be such that a cast or conversion to the type
     * of the variable is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(const id_type id, meta_handle instance, Type &&value) const {
        auto const candidate = data(id);
        return candidate ? candidate.set(std::move(instance), std::forward<Type>(value)) : false;
    }

    /**
     * @brief Gets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(const id_type id, meta_handle instance) const {
        auto const candidate = data(id);
        return candidate ? candidate.get(std::move(instance)) : meta_any{};
    }

    /**
     * @brief Returns a range to use to visit top-level meta properties.
     * @return An iterable range to use to visit top-level meta properties.
     */
    [[nodiscard]] meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * Properties of the base classes will also be visited, if any.
     *
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        return internal::find_if<&node_type::prop>([key = std::move(key)](const auto *curr) {
            return curr->key() == key;
        }, node);
    }

    /**
     * @brief Returns true if a meta object is valid, false otherwise.
     * @return True if the meta object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two meta objects refer to the same type.
     * @param other The meta object with which to compare.
     * @return True if the two meta objects refer to the same type, false
     * otherwise.
     */
    [[nodiscard]] bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return (!node && !other.node) || (node && other.node && node->info == other.node->info);
    }

    /**
     * @brief Resets a meta type and all its parts.
     *
     * This function resets a meta type and all its data members, member
     * functions and properties, as well as its constructors, destructors and
     * conversion functions if any.<br/>
     * Base classes aren't reset but the link between the two types is removed.
     *
     * The meta type is also removed from the list of searchable types.
     */
    void reset() ENTT_NOEXCEPT {
        auto** it = internal::meta_context::global();

        while (*it && *it != node) {
            it = &(*it)->next;
        }

        if(*it) {
            *it = (*it)->next;
        }

        const auto unregister_all = y_combinator{
            [](auto &&self, auto **curr, auto... member) {
                while(*curr) {
                    auto *prev = *curr;
                    (self(&(prev->*member)), ...);
                    *curr = prev->next;
                    prev->next = nullptr;
                }
            }
        };

        unregister_all(&node->prop);
        unregister_all(&node->base);
        unregister_all(&node->conv);
        unregister_all(&node->ctor, &internal::meta_ctor_node::prop);
        unregister_all(&node->data, &internal::meta_data_node::prop);
        unregister_all(&node->func, &internal::meta_func_node::prop);

        node->id = {};
        node->dtor = nullptr;
    }

private:
    node_type *node;
};


/**
 * @brief Checks if two meta objects refer to the same type.
 * @param lhs A meta object, either valid or not.
 * @param rhs A meta object, either valid or not.
 * @return False if the two meta objects refer to the same node, true otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_type &lhs, const meta_type &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


[[nodiscard]] inline meta_type meta_any::type() const ENTT_NOEXCEPT {
    return node;
}


template<typename... Args>
meta_any meta_any::invoke(const id_type id, Args &&... args) const {
    return type().invoke(id, *this, std::forward<Args>(args)...);
}


template<typename... Args>
meta_any meta_any::invoke(const id_type id, Args &&... args) {
    return type().invoke(id, *this, std::forward<Args>(args)...);
}


template<typename Type>
bool meta_any::set(const id_type id, Type &&value) {
    return type().set(id, *this, std::forward<Type>(value));
}


[[nodiscard]] inline meta_any meta_any::get(const id_type id) const {
    return type().get(id, *this);
}


[[nodiscard]] inline meta_any meta_any::get(const id_type id) {
    return type().get(id, *this);
}


[[nodiscard]] inline meta_type meta_base::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_base::type() const ENTT_NOEXCEPT {
    return node->type();
}


[[nodiscard]] inline meta_type meta_conv::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_conv::type() const ENTT_NOEXCEPT {
    return node->type();
}


[[nodiscard]] inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


[[nodiscard]] inline meta_type meta_data::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_data::type() const ENTT_NOEXCEPT {
    return node->type();
}


[[nodiscard]] inline meta_type meta_func::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_func::ret() const ENTT_NOEXCEPT {
    return node->ret();
}


[[nodiscard]] inline meta_type meta_func::arg(size_type index) const ENTT_NOEXCEPT {
    return index < size() ? node->arg(index) : nullptr;
}


/*! @brief Opaque iterator for meta sequence containers. */
class meta_sequence_container::meta_iterator {
    /*! @brief A meta sequence container can access the underlying iterator. */
    friend class meta_sequence_container;

    enum class operation { INCR, DEREF };

    using vtable_type = void(const operation, const any &, void *);

    template<typename It>
    static void basic_vtable(const operation op, const any &from, void *to) {
        switch(op) {
        case operation::INCR:
            ++any_cast<It &>(const_cast<any &>(from));
            break;
        case operation::DEREF:
            *static_cast<meta_any *>(to) = std::reference_wrapper{*any_cast<const It &>(from)};
            break;
        }
    }

public:
    /*! @brief Signed integer type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Type of elements returned by the iterator. */
    using value_type = meta_any;
    /*! @brief Pointer type, `void` on purpose. */
    using pointer = void;
    /*! @brief Reference type, it is **not** an actual reference. */
    using reference = value_type;
    /*! @brief Iterator category. */
    using iterator_category = std::input_iterator_tag;

    /*! @brief Default constructor. */
    meta_iterator() ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a meta iterator from a given iterator.
     * @tparam It Type of actual iterator with which to build the meta iterator.
     * @param iter The actual iterator with which to build the meta iterator.
     */
    template<typename It>
    meta_iterator(It iter)
        : vtable{&basic_vtable<It>},
          handle{std::move(iter)}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return vtable(operation::INCR, handle, nullptr), *this;
    }

    /*! @brief Post-increment operator. @return This iterator. */
    meta_iterator operator++(int) ENTT_NOEXCEPT {
        meta_iterator orig = *this;
        return ++(*this), orig;
    }

    /**
     * @brief Checks if two meta iterators refer to the same element.
     * @param other The meta iterator with which to compare.
     * @return True if the two meta iterators refer to the same element, false
     * otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return handle == other.handle;
    }

    /**
     * @brief Checks if two meta iterators refer to the same element.
     * @param other The meta iterator with which to compare.
     * @return False if the two meta iterators refer to the same element, true
     * otherwise.
     */
    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    /**
     * @brief Indirection operator.
     * @return The element to which the meta pointer points.
     */
    [[nodiscard]] reference operator*() const {
        meta_any other;
        vtable(operation::DEREF, handle, &other);
        return other;
    }

    /**
     * @brief Returns false if an iterator is invalid, true otherwise.
     * @return False if the iterator is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(handle);
    }

private:
    vtable_type *vtable;
    any handle;
};


template<typename Type>
struct meta_sequence_container::meta_sequence_container_proxy {
    using traits_type = meta_sequence_container_traits<Type>;

    [[nodiscard]] static meta_type value_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename traits_type::value_type>::resolve();
    }

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
        return traits_type::size(any_cast<const Type &>(container));
    }

    [[nodiscard]] static bool resize(any &container, size_type sz) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return traits_type::resize(*cont, sz);
        }

        return false;
    }

    [[nodiscard]] static bool clear(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return traits_type::clear(*cont);
        }

        return false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return iterator{traits_type::begin(*cont)};
        }

        return iterator{traits_type::cbegin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return iterator{traits_type::end(*cont)};
        }

        return iterator{traits_type::cend(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static std::pair<iterator, bool> insert(any &container, iterator it, meta_any &value) {
        if(auto *cont = any_cast<Type>(&container); cont && value.allow_cast<const typename traits_type::value_type &>()) {
            auto ret = traits_type::insert(*cont, any_cast<const typename traits_type::iterator &>(it.handle), value.cast<const typename traits_type::value_type &>());
            return { iterator{std::move(ret.first)}, ret.second };
        }

        return {};
    }

    [[nodiscard]] static std::pair<iterator, bool> erase(any &container, iterator it) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            auto ret = traits_type::erase(*cont, any_cast<const typename traits_type::iterator &>(it.handle));
            return { iterator{std::move(ret.first)}, ret.second };
        }

        return {};
    }

    [[nodiscard]] static meta_any get(any &container, size_type pos) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return std::reference_wrapper{traits_type::get(*cont, pos)};
        }

        return std::reference_wrapper{traits_type::cget(any_cast<const Type &>(container), pos)};
    }
};


/**
 * @brief Returns the value meta type of the wrapped container type.
 * @return The value meta type of the wrapped container type.
 */
[[nodiscard]] inline meta_type meta_sequence_container::value_type() const ENTT_NOEXCEPT {
    return value_type_fn();
}


/**
 * @brief Returns the size of the wrapped container.
 * @return The size of the wrapped container.
 */
[[nodiscard]] inline meta_sequence_container::size_type meta_sequence_container::size() const ENTT_NOEXCEPT {
    return size_fn(storage);
}


/**
 * @brief Resizes the wrapped container to contain a given number of elements.
 * @param sz The new size of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::resize(size_type sz) {
    return resize_fn(storage, sz);
}


/**
 * @brief Clears the content of the wrapped container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::clear() {
    return clear_fn(storage);
}


/**
 * @brief Returns a meta iterator to the first element of the wrapped container.
 * @return A meta iterator to the first element of the wrapped container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    return begin_fn(storage);
}


/**
 * @brief Returns a meta iterator that is past the last element of the wrapped
 * container.
 * @return A meta iterator that is past the last element of the wrapped
 * container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    return end_fn(storage);
}


/**
 * @brief Inserts an element at a specified location of the wrapped container.
 * @param it Meta iterator before which the element will be inserted.
 * @param value Element value to insert.
 * @return A pair consisting of a meta iterator to the inserted element (in
 * case of success) and a bool denoting whether the insertion took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::insert(iterator it, meta_any value) {
    return insert_fn(storage, it, value);
}


/**
 * @brief Removes the specified element from the wrapped container.
 * @param it Meta iterator to the element to remove.
 * @return A pair consisting of a meta iterator following the last removed
 * element (in case of success) and a bool denoting whether the insertion
 * took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::erase(iterator it) {
    return erase_fn(storage, it);
}


/**
 * @brief Returns a reference to the element at a specified location of the
 * wrapped container (no bounds checking is performed).
 * @param pos The position of the element to return.
 * @return A reference to the requested element properly wrapped.
 */
[[nodiscard]] inline meta_any meta_sequence_container::operator[](size_type pos) {
    return get_fn(storage, pos);
}


/**
 * @brief Returns false if a proxy is invalid, true otherwise.
 * @return False if the proxy is invalid, true otherwise.
 */
[[nodiscard]] inline meta_sequence_container::operator bool() const ENTT_NOEXCEPT {
    return static_cast<bool>(storage);
}


/*! @brief Opaque iterator for meta associative containers. */
class meta_associative_container::meta_iterator {
    enum operation { INCR, DEREF };

    using vtable_type = void(const operation, const any &, void *);

    template<bool KeyOnly, typename It>
    static void basic_vtable(const operation op, const any &from, void *to) {
        switch(op) {
        case operation::INCR:
            ++any_cast<It &>(const_cast<any &>(from));
            break;
        case operation::DEREF:
            if constexpr(KeyOnly) {
                static_cast<std::pair<meta_any, meta_any> *>(to)->first = *any_cast<const It &>(from);
            } else {
                *static_cast<std::pair<meta_any, meta_any> *>(to) = std::make_pair<meta_any, meta_any>(any_cast<const It &>(from)->first, std::reference_wrapper{any_cast<const It &>(from)->second});
            }
            break;
        }
    }

public:
    /*! @brief Signed integer type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Type of elements returned by the iterator. */
    using value_type = std::pair<meta_any, meta_any>;
    /*! @brief Pointer type, `void` on purpose. */
    using pointer = void;
    /*! @brief Reference type, it is **not** an actual reference. */
    using reference = value_type;
    /*! @brief Iterator category. */
    using iterator_category = std::input_iterator_tag;

    /*! @brief Default constructor. */
    meta_iterator() ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a meta iterator from a given iterator.
     * @tparam KeyOnly True if the associative container is also key-only, false
     * otherwise.
     * @tparam It Type of actual iterator with which to build the meta iterator.
     * @param iter The actual iterator with which to build the meta iterator.
     */
    template<bool KeyOnly, typename It>
    meta_iterator(std::integral_constant<bool, KeyOnly>, It iter)
        : vtable{&basic_vtable<KeyOnly, It>},
          handle{std::move(iter)}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return vtable(operation::INCR, handle, nullptr), *this;
    }

    /*! @brief Post-increment operator. @return This iterator. */
    meta_iterator operator++(int) ENTT_NOEXCEPT {
        meta_iterator orig = *this;
        return ++(*this), orig;
    }

    /**
     * @brief Checks if two meta iterators refer to the same element.
     * @param other The meta iterator with which to compare.
     * @return True if the two meta iterators refer to the same element, false
     * otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return handle == other.handle;
    }

    /**
     * @brief Checks if two meta iterators refer to the same element.
     * @param other The meta iterator with which to compare.
     * @return False if the two meta iterators refer to the same element, true
     * otherwise.
     */
    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    /**
     * @brief Indirection operator.
     * @return The element to which the meta pointer points.
     */
    [[nodiscard]] reference operator*() const {
        reference other;
        vtable(operation::DEREF, handle, &other);
        return other;
    }

    /**
     * @brief Returns false if an iterator is invalid, true otherwise.
     * @return False if the iterator is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(handle);
    }

private:
    vtable_type *vtable;
    any handle;
};


template<typename Type>
struct meta_associative_container::meta_associative_container_proxy {
    using traits_type = meta_associative_container_traits<Type>;

    [[nodiscard]] static meta_type key_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename traits_type::key_type>::resolve();
    }

    [[nodiscard]] static meta_type mapped_type() ENTT_NOEXCEPT {
        if constexpr(is_key_only_meta_associative_container_v<Type>) {
            return meta_type{};
        } else {
            return internal::meta_info<typename traits_type::mapped_type>::resolve();
        }
    }

    [[nodiscard]] static meta_type value_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename traits_type::value_type>::resolve();
    }

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
        return traits_type::size(any_cast<const Type &>(container));
    }

    [[nodiscard]] static bool clear(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return traits_type::clear(*cont);
        }

        return false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::begin(*cont)};
        }

        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cbegin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto *cont = any_cast<Type>(&container); cont) {
            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::end(*cont)};
        }

        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cend(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static bool insert(any &container, meta_any &key, meta_any &value) {
        if(auto *cont = any_cast<Type>(&container); cont && key.allow_cast<const typename traits_type::key_type &>()) {
            if constexpr(is_key_only_meta_associative_container_v<Type>) {
                return traits_type::insert(*cont, key.cast<const typename traits_type::key_type &>());
            } else {
                if(value.allow_cast<const typename traits_type::mapped_type &>()) {
                    return traits_type::insert(*cont, key.cast<const typename traits_type::key_type &>(), value.cast<const typename traits_type::mapped_type &>());
                }
            }
        }

        return false;
    }

    [[nodiscard]] static bool erase(any &container, meta_any &key) {
        if(auto *cont = any_cast<Type>(&container); cont && key.allow_cast<const typename traits_type::key_type &>()) {
            return traits_type::erase(*cont, key.cast<const typename traits_type::key_type &>());
        }

        return false;
    }

    [[nodiscard]] static iterator find(any &container, meta_any &key) {
        if(key.allow_cast<const typename traits_type::key_type &>()) {
            if(auto *cont = any_cast<Type>(&container); cont) {
                return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::find(*cont, key.cast<const typename traits_type::key_type &>())};
            }

            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cfind(any_cast<const Type &>(container), key.cast<const typename traits_type::key_type &>())};
        }

        return {};
    }
};


/**
 * @brief Returns true if the associative container is also key-only, false
 * otherwise.
 * @return True if the associative container is also key-only, false otherwise.
 */
[[nodiscard]] inline bool meta_associative_container::key_only() const ENTT_NOEXCEPT {
    return key_only_container;
}


/**
 * @brief Returns the key meta type of the wrapped container type.
 * @return The key meta type of the wrapped container type.
 */
[[nodiscard]] inline meta_type meta_associative_container::key_type() const ENTT_NOEXCEPT {
    return key_type_fn();
}


/**
 * @brief Returns the mapped meta type of the wrapped container type.
 * @return The mapped meta type of the wrapped container type.
 */
[[nodiscard]] inline meta_type meta_associative_container::mapped_type() const ENTT_NOEXCEPT {
    return mapped_type_fn();
}


/*! @copydoc meta_sequence_container::value_type */
[[nodiscard]] inline meta_type meta_associative_container::value_type() const ENTT_NOEXCEPT {
    return value_type_fn();
}


/*! @copydoc meta_sequence_container::size */
[[nodiscard]] inline meta_associative_container::size_type meta_associative_container::size() const ENTT_NOEXCEPT {
    return size_fn(storage);
}


/*! @copydoc meta_sequence_container::clear */
inline bool meta_associative_container::clear() {
    return clear_fn(storage);
}


/*! @copydoc meta_sequence_container::begin */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::begin() {
    return begin_fn(storage);
}


/*! @copydoc meta_sequence_container::end */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::end() {
    return end_fn(storage);
}


/**
 * @brief Inserts an element (a key/value pair) into the wrapped container.
 * @param key The key of the element to insert.
 * @param value The value of the element to insert.
 * @return A bool denoting whether the insertion took place.
 */
inline bool meta_associative_container::insert(meta_any key, meta_any value = {}) {
    return insert_fn(storage, key, value);
}


/**
 * @brief Removes the specified element from the wrapped container.
 * @param key The key of the element to remove.
 * @return A bool denoting whether the removal took place.
 */
inline bool meta_associative_container::erase(meta_any key) {
    return erase_fn(storage, key);
}


/**
 * @brief Returns an iterator to the element with key equivalent to a given
 * one, if any.
 * @param key The key of the element to search.
 * @return An iterator to the element with the given key, if any.
 */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::find(meta_any key) {
    return find_fn(storage, key);
}


/**
 * @brief Returns false if a proxy is invalid, true otherwise.
 * @return False if the proxy is invalid, true otherwise.
 */
[[nodiscard]] inline meta_associative_container::operator bool() const ENTT_NOEXCEPT {
    return static_cast<bool>(storage);
}


}


#endif
