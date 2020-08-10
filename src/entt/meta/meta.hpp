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
#include "../core/fwd.hpp"
#include "../core/utility.hpp"
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
    meta_sequence_container() ENTT_NOEXCEPT
        : instance{nullptr}
    {}

    /**
     * @brief Construct a proxy object for sequence containers.
     * @tparam Type Type of container to wrap.
     * @param container The container to wrap.
     */
    template<typename Type>
    meta_sequence_container(Type *container) ENTT_NOEXCEPT
        : value_type_fn{&meta_sequence_container_proxy<Type>::value_type},
          size_fn{&meta_sequence_container_proxy<Type>::size},
          resize_fn{&meta_sequence_container_proxy<Type>::resize},
          clear_fn{&meta_sequence_container_proxy<Type>::clear},
          begin_fn{&meta_sequence_container_proxy<Type>::begin},
          end_fn{&meta_sequence_container_proxy<Type>::end},
          insert_fn{&meta_sequence_container_proxy<Type>::insert},
          erase_fn{&meta_sequence_container_proxy<Type>::erase},
          get_fn{&meta_sequence_container_proxy<Type>::get},
          instance{container}
    {}

    [[nodiscard]] inline meta_type value_type() const ENTT_NOEXCEPT;
    [[nodiscard]] inline size_type size() const ENTT_NOEXCEPT;
    inline bool resize(size_type) const;
    inline bool clear();
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline std::pair<iterator, bool> insert(iterator, meta_any);
    inline std::pair<iterator, bool> erase(iterator);
    [[nodiscard]] inline meta_any operator[](size_type);
    [[nodiscard]] inline explicit operator bool() const ENTT_NOEXCEPT;

private:
    meta_type(* value_type_fn)() ENTT_NOEXCEPT;
    size_type(* size_fn)(const void *) ENTT_NOEXCEPT;
    bool(* resize_fn)(void *, size_type);
    bool(* clear_fn)(void *);
    iterator(* begin_fn)(void *);
    iterator(* end_fn)(void *);
    std::pair<iterator, bool>(* insert_fn)(void *, iterator, meta_any);
    std::pair<iterator, bool>(* erase_fn)(void *, iterator);
    meta_any(* get_fn)(void *, size_type);
    void *instance;
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
    meta_associative_container() ENTT_NOEXCEPT
        : instance{nullptr}
    {}

    /**
     * @brief Construct a proxy object for associative containers.
     * @tparam Type Type of container to wrap.
     * @param container The container to wrap.
     */
    template<typename Type>
    meta_associative_container(Type *container) ENTT_NOEXCEPT
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
          instance{container}
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
    bool key_only_container;
    meta_type(* key_type_fn)() ENTT_NOEXCEPT;
    meta_type(* mapped_type_fn)() ENTT_NOEXCEPT;
    meta_type(* value_type_fn)() ENTT_NOEXCEPT;
    size_type(* size_fn)(const void *) ENTT_NOEXCEPT;
    bool(* clear_fn)(void *);
    iterator(* begin_fn)(void *);
    iterator(* end_fn)(void *);
    bool(* insert_fn)(void *, meta_any, meta_any);
    bool(* erase_fn)(void *, meta_any);
    iterator(* find_fn)(void *, meta_any);
    void *instance;
};


/**
 * @brief Opaque wrapper for values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to get rid
 * of memory allocations if possible. This should improve overall performance.
 */
class meta_any {
    using dereference_operator_type = meta_any(meta_any &);

    template<typename Type>
    [[nodiscard]] static meta_any dereference_operator(meta_any &any) {
        if constexpr(is_meta_pointer_like_v<Type>) {
            if constexpr(std::is_const_v<std::remove_reference_t<decltype(*std::declval<Type>())>>) {
                return *any.cast<Type>();
            } else {
                return std::ref(*any.cast<Type>());
            }
        } else {
            return {};
        }
    }

    template<typename Type>
    [[nodiscard]] static meta_sequence_container meta_sequence_container_factory([[maybe_unused]] void *container) ENTT_NOEXCEPT {
        if constexpr(has_meta_sequence_container_traits_v<Type>) {
            return static_cast<Type *>(container);
        } else {
            return {};
        }
    }

    template<typename Type>
    [[nodiscard]] static meta_associative_container meta_associative_container_factory([[maybe_unused]] void *container) ENTT_NOEXCEPT {
        if constexpr(has_meta_associative_container_traits_v<Type>) {
            return static_cast<Type *>(container);
        } else {
            return {};
        }
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          node{},
          deref{nullptr},
          seq_factory{nullptr},
          assoc_factory{nullptr}
    {}

    /**
     * @brief Constructs a meta any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : storage(std::in_place_type<Type>, std::forward<Args>(args)...),
          node{internal::meta_info<Type>::resolve()},
          deref{&dereference_operator<Type>},
          seq_factory{&meta_sequence_container_factory<Type>},
          assoc_factory{&meta_associative_container_factory<Type>}
    {}

    /**
     * @brief Constructs a meta any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    meta_any(std::reference_wrapper<Type> value)
        : storage{value},
          node{internal::meta_info<Type>::resolve()},
          deref{&dereference_operator<Type>},
          seq_factory{&meta_sequence_container_factory<Type>},
          assoc_factory{&meta_associative_container_factory<Type>}
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
    meta_any(meta_any &&other)
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
     * @brief Tries to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return A (possibly null) pointer to the contained instance.
     */
    template<typename Type>
    [[nodiscard]] const Type * try_cast() const {
        if(node) {
            if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node->type_id == type_id) {
                return static_cast<const Type *>(storage.data());
            } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); base) {
                return static_cast<const Type *>(base->cast(storage.data()));
            }
        }

        return nullptr;
    }

    /*! @copydoc try_cast */
    template<typename Type>
    [[nodiscard]] Type * try_cast() {
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
    [[nodiscard]] const Type & cast() const {
        auto * const actual = try_cast<Type>();
        ENTT_ASSERT(actual);
        return *actual;
    }

    /*! @copydoc cast */
    template<typename Type>
    [[nodiscard]] Type & cast() {
        return const_cast<Type &>(std::as_const(*this).cast<Type>());
    }

    /**
     * @brief Tries to convert an instance to a given type and returns it.
     * @tparam Type Type to which to convert the instance.
     * @return A valid meta any object if the conversion is possible, an invalid
     * one otherwise.
     */
    template<typename Type>
    [[nodiscard]] meta_any convert() const {
        if(node) {
            if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node->type_id == type_id) {
                return *this;
            } else if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); conv) {
                return conv->conv(storage.data());
            }
        }

        return {};
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
     * @brief Aliasing constructor.
     * @return A meta any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any ref() const ENTT_NOEXCEPT {
        meta_any other{};
        other.node = node;
        other.storage = storage.ref();
        other.deref = deref;
        other.seq_factory = seq_factory;
        other.assoc_factory = assoc_factory;
        return other;
    }

    /**
     * @brief Returns a sequence container proxy.
     * @return A sequence container proxy for the underlying object.
     */
    [[nodiscard]] meta_sequence_container as_sequence_container() ENTT_NOEXCEPT {
        return seq_factory(storage.data());
    }

    /**
     * @brief Returns an associative container proxy.
     * @return An associative container proxy for the underlying object.
     */
    [[nodiscard]] meta_associative_container as_associative_container() ENTT_NOEXCEPT {
        return assoc_factory(storage.data());
    }

    /**
     * @brief Indirection operator for dereferencing opaque objects.
     * @return A meta any that shares a reference to an unmanaged object if the
     * wrapped element is dereferenceable, an invalid meta any otherwise.
     */
    [[nodiscard]] meta_any operator*() ENTT_NOEXCEPT {
        return deref(*this);
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
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
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id && node->compare(storage.data(), other.storage.data()));
    }

    /**
     * @brief Swaps two meta any objects.
     * @param lhs A valid meta any object.
     * @param rhs A valid meta any object.
     */
    friend void swap(meta_any &lhs, meta_any &rhs) {
        using std::swap;
        swap(lhs.storage, rhs.storage);
        swap(lhs.node, rhs.node);
        swap(lhs.deref, rhs.deref);
        swap(lhs.seq_factory, rhs.seq_factory);
        swap(lhs.assoc_factory, rhs.assoc_factory);
    }

private:
    internal::meta_storage storage;
    internal::meta_type_node *node;
    dereference_operator_type *deref;
    meta_sequence_container(* seq_factory)(void *);
    meta_associative_container(* assoc_factory)(void *);
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
    meta_handle(Type &&value) ENTT_NOEXCEPT
        : meta_handle{}
    {
        if constexpr(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_any>) {
            any = value.ref();
        } else {
            static_assert(std::is_lvalue_reference_v<Type>, "Lvalue reference required");
            any = std::ref(value);
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
    [[nodiscard]] meta_any invoke(meta_any * const args, const std::size_t sz) const {
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
    [[nodiscard]] meta_any invoke(meta_handle instance, meta_any * const args, const std::size_t sz) const {
        return sz == size() ? node->invoke(instance, args) : meta_any{};
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
        return invoke(instance, arguments.data(), sizeof...(Args));
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
    template<typename... Args, std::size_t... Indexes>
    [[nodiscard]] auto ctor(std::index_sequence<Indexes...>) const {
        internal::meta_range range{node->ctor};

        return std::find_if(range.begin(), range.end(), [](const auto &candidate) {
            return candidate.size == sizeof...(Args) && ([](auto *from, auto *to) {
                return (from->type_id == to->type_id)
                        || internal::find_if<&node_type::base>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from)
                        || internal::find_if<&node_type::conv>([to](const auto *curr) { return curr->type()->type_id == to->type_id; }, from);
            }(internal::meta_info<Args>::resolve(), candidate.arg(Indexes)) && ...);
        }).operator->();
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
     * @brief Returns the type id of the underlying type.
     * @return The type id of the underlying type.
     */
    [[nodiscard]] id_type type_id() const ENTT_NOEXCEPT {
        return node->type_id;
    }

    /**
     * @brief Returns the identifier assigned to a meta object.
     * @return The identifier assigned to the meta object.
     */
    [[nodiscard]] id_type id() const ENTT_NOEXCEPT {
        return node->id;
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
        return internal::find_if<&node_type::conv>([type_id = internal::meta_info<Type>::resolve()->type_id](const auto *curr) {
            return curr->type()->type_id == type_id;
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
        return ctor<Args...>(std::index_sequence_for<Args...>{});
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
     * The meta functions of the base classes will also be visited, if any.
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
    [[nodiscard]] meta_any construct(meta_any * const args, const std::size_t sz) const {
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
        return (!node && !other.node) || (node && other.node && node->type_id == other.node->type_id);
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

    template<typename It>
    static void incr(meta_any any) {
        ++any.cast<It>();
    }

    template<typename It>
    [[nodiscard]] static meta_any deref(meta_any any) {
        if constexpr(std::is_const_v<std::remove_reference_t<decltype(*std::declval<It>())>>) {
            return *any.cast<It>();
        } else {
            return std::ref(*any.cast<It>());
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
        : next_fn{&incr<It>},
          get_fn{&deref<It>},
          handle{std::move(iter)}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return next_fn(handle.ref()), *this;
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
        return get_fn(handle.ref());
    }

    /**
     * @brief Returns false if an iterator is invalid, true otherwise.
     * @return False if the iterator is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(handle);
    }

private:
    void(* next_fn)(meta_any);
    meta_any(* get_fn)(meta_any);
    meta_any handle;
};


template<typename Type>
struct meta_sequence_container::meta_sequence_container_proxy {
    using traits_type = meta_sequence_container_traits<Type>;

    [[nodiscard]] static meta_type value_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename traits_type::value_type>::resolve();
    }

    [[nodiscard]] static size_type size(const void *container) ENTT_NOEXCEPT {
        return traits_type::size(*static_cast<const Type *>(container));
    }

    [[nodiscard]] static bool resize(void *container, size_type sz) {
        return traits_type::resize(*static_cast<Type *>(container), sz);
    }

    [[nodiscard]] static bool clear(void *container) {
        return traits_type::clear(*static_cast<Type *>(container));
    }

    [[nodiscard]] static iterator begin(void *container) {
        return iterator{traits_type::begin(*static_cast<Type *>(container))};
    }

    [[nodiscard]] static iterator end(void *container) {
        return iterator{traits_type::end(*static_cast<Type *>(container))};
    }

    [[nodiscard]] static std::pair<iterator, bool> insert(void *container, iterator it, meta_any value) {
        if(const auto *v_ptr = value.try_cast<typename traits_type::value_type>(); v_ptr || value.convert<typename traits_type::value_type>()) {
            auto ret = traits_type::insert(*static_cast<Type *>(container), it.handle.cast<typename traits_type::iterator>(), v_ptr ? *v_ptr : value.cast<typename traits_type::value_type>());
            return {iterator{std::move(ret.first)}, ret.second};
        }

        return {};
    }

    [[nodiscard]] static std::pair<iterator, bool> erase(void *container, iterator it) {
        auto ret = traits_type::erase(*static_cast<Type *>(container), it.handle.cast<typename traits_type::iterator>());
        return {iterator{std::move(ret.first)}, ret.second};
    }

    [[nodiscard]] static meta_any get(void *container, size_type pos) {
        return std::ref(traits_type::get(*static_cast<Type *>(container), pos));
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
    return size_fn(instance);
}


/**
 * @brief Resizes the wrapped container to contain a given number of elements.
 * @param sz The new size of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::resize(size_type sz) const {
    return resize_fn(instance, sz);
}


/**
 * @brief Clears the content of the wrapped container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::clear() {
    return clear_fn(instance);
}


/**
 * @brief Returns a meta iterator to the first element of the wrapped container.
 * @return A meta iterator to the first element of the wrapped container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    return begin_fn(instance);
}


/**
 * @brief Returns a meta iterator that is past the last element of the wrapped
 * container.
 * @return A meta iterator that is past the last element of the wrapped
 * container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    return end_fn(instance);
}


/**
 * @brief Inserts an element at a specified location of the wrapped container.
 * @param it Meta iterator before which the element will be inserted.
 * @param value Element value to insert.
 * @return A pair consisting of a meta iterator to the inserted element (in
 * case of success) and a bool denoting whether the insertion took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::insert(iterator it, meta_any value) {
    return insert_fn(instance, it, value.ref());
}


/**
 * @brief Removes the specified element from the wrapped container.
 * @param it Meta iterator to the element to remove.
 * @return A pair consisting of a meta iterator following the last removed
 * element (in case of success) and a bool denoting whether the insertion
 * took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::erase(iterator it) {
    return erase_fn(instance, it);
}


/**
 * @brief Returns a reference to the element at a specified location of the
 * wrapped container (no bounds checking is performed).
 * @param pos The position of the element to return.
 * @return A reference to the requested element properly wrapped.
 */
[[nodiscard]] inline meta_any meta_sequence_container::operator[](size_type pos) {
    return get_fn(instance, pos);
}


/**
 * @brief Returns false if a proxy is invalid, true otherwise.
 * @return False if the proxy is invalid, true otherwise.
 */
[[nodiscard]] inline meta_sequence_container::operator bool() const ENTT_NOEXCEPT {
    return (instance != nullptr);
}


/*! @brief Opaque iterator for meta associative containers. */
class meta_associative_container::meta_iterator {
    template<typename It>
    static void incr(meta_any any) {
        ++any.cast<It>();
    }

    template<bool KeyOnly, typename It>
    [[nodiscard]] static meta_any key(meta_any any) {
        if constexpr(KeyOnly) {
            return *any.cast<It>();
        } else {
            return any.cast<It>()->first;
        }
    }

    template<bool KeyOnly, typename It>
    [[nodiscard]] static meta_any value([[maybe_unused]] meta_any any) {
        if constexpr(KeyOnly) {
            return meta_any{};
        } else {
            return std::ref(any.cast<It>()->second);
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
        : next_fn{&incr<It>},
          key_fn{&key<KeyOnly, It>},
          value_fn{&value<KeyOnly, It>},
          handle{std::move(iter)}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return next_fn(handle.ref()), *this;
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
        return { key_fn(handle.ref()), value_fn(handle.ref()) };
    }

    /**
     * @brief Returns false if an iterator is invalid, true otherwise.
     * @return False if the iterator is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(handle);
    }

private:
    void(* next_fn)(meta_any);
    meta_any(* key_fn)(meta_any);
    meta_any(* value_fn)(meta_any);
    meta_any handle;
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

    [[nodiscard]] static size_type size(const void *container) ENTT_NOEXCEPT {
        return traits_type::size(*static_cast<const Type *>(container));
    }

    [[nodiscard]] static bool clear(void *container) {
        return traits_type::clear(*static_cast<Type *>(container));
    }

    [[nodiscard]] static iterator begin(void *container) {
        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::begin(*static_cast<Type *>(container))};
    }

    [[nodiscard]] static iterator end(void *container) {
        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::end(*static_cast<Type *>(container))};
    }

    [[nodiscard]] static bool insert(void *container, meta_any key, meta_any value) {
        if(const auto *k_ptr = key.try_cast<typename traits_type::key_type>(); k_ptr || key.convert<typename traits_type::key_type>()) {
            if constexpr(is_key_only_meta_associative_container_v<Type>) {
                return traits_type::insert(*static_cast<Type *>(container), k_ptr ? *k_ptr : key.cast<typename traits_type::key_type>());
            } else {
                if(auto *m_ptr = value.try_cast<typename traits_type::mapped_type>(); m_ptr || value.convert<typename traits_type::mapped_type>()) {
                    return traits_type::insert(*static_cast<Type *>(container), k_ptr ? *k_ptr : key.cast<typename traits_type::key_type>(), m_ptr ? *m_ptr : value.cast<typename traits_type::mapped_type>());
                }
            }
        }

        return false;
    }

    [[nodiscard]] static bool erase(void *container, meta_any key) {
        if(const auto *k_ptr = key.try_cast<typename traits_type::key_type>(); k_ptr || key.convert<typename traits_type::key_type>()) {
            return traits_type::erase(*static_cast<Type *>(container), k_ptr ? *k_ptr : key.cast<typename traits_type::key_type>());
        }

        return false;
    }

    [[nodiscard]] static iterator find(void *container, meta_any key) {
        if(const auto *k_ptr = key.try_cast<typename traits_type::key_type>(); k_ptr || key.convert<typename traits_type::key_type>()) {
            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::find(*static_cast<Type *>(container), k_ptr ? *k_ptr : key.cast<typename traits_type::key_type>())};
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
    return size_fn(instance);
}


/*! @copydoc meta_sequence_container::clear */
inline bool meta_associative_container::clear() {
    return clear_fn(instance);
}


/*! @copydoc meta_sequence_container::begin */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::begin() {
    return begin_fn(instance);
}


/*! @copydoc meta_sequence_container::end */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::end() {
    return end_fn(instance);
}


/**
 * @brief Inserts an element (a key/value pair) into the wrapped container.
 * @param key The key of the element to insert.
 * @param value The value of the element to insert.
 * @return A bool denoting whether the insertion took place.
 */
inline bool meta_associative_container::insert(meta_any key, meta_any value = {}) {
    return insert_fn(instance, key.ref(), value.ref());
}


/**
 * @brief Removes the specified element from the wrapped container.
 * @param key The key of the element to remove.
 * @return A bool denoting whether the removal took place.
 */
inline bool meta_associative_container::erase(meta_any key) {
    return erase_fn(instance, key.ref());
}


/**
 * @brief Returns an iterator to the element with key equivalent to a given
 * one, if any.
 * @param key The key of the element to search.
 * @return An iterator to the element with the given key, if any.
 */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::find(meta_any key) {
    return find_fn(instance, key.ref());
}


/**
 * @brief Returns false if a proxy is invalid, true otherwise.
 * @return False if the proxy is invalid, true otherwise.
 */
[[nodiscard]] inline meta_associative_container::operator bool() const ENTT_NOEXCEPT {
    return (instance != nullptr);
}


}


#endif
