#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/utility.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "adl_pointer.hpp"
#include "ctx.hpp"
#include "node.hpp"
#include "range.hpp"
#include "type_traits.hpp"


namespace entt {


class meta_any;
class meta_type;


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
    enum class operation { DTOR, REF, CREF, DEREF, CDEREF, SEQ, CSEQ, ASSOC, CASSOC };

    using vtable_type = void(const operation, const any &, void *);

    template<typename Type>
    static void basic_vtable(const operation op, [[maybe_unused]] const any &from, [[maybe_unused]] void *to) {
        if constexpr(!std::is_void_v<Type>) {
            switch(op) {
            case operation::DTOR:
                if constexpr(!std::is_lvalue_reference_v<Type>) {
                    if(auto *curr = static_cast<internal::meta_type_node *>(to); curr->dtor) {
                        curr->dtor(const_cast<any &>(from).data());
                    }
                }
            break;
            case operation::REF:
            case operation::CREF:
                *static_cast<meta_any *>(to) = (op == operation::REF ? meta_any{std::ref(any_cast<Type &>(const_cast<any &>(from)))} : meta_any{std::cref(any_cast<const std::decay_t<Type> &>(from))});
                break;
            case operation::DEREF:
            case operation::CDEREF:
                if constexpr(is_meta_pointer_like_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
                    using element_type = std::remove_const_t<typename std::pointer_traits<std::decay_t<Type>>::element_type>;

                    if constexpr(std::is_function_v<element_type>) {
                        *static_cast<meta_any *>(to) = any_cast<std::decay_t<Type>>(from);
                    } else if constexpr(!std::is_same_v<element_type, void>) {
                        using adl_meta_pointer_like_type = adl_meta_pointer_like<std::decay_t<Type>>;

                        if constexpr(std::is_lvalue_reference_v<decltype(adl_meta_pointer_like_type::dereference(std::declval<const std::decay_t<Type> &>()))>) {
                            auto &&obj = adl_meta_pointer_like_type::dereference(any_cast<const std::decay_t<Type> &>(from));
                            *static_cast<meta_any *>(to) = (op == operation::DEREF ? meta_any{std::ref(obj)} : meta_any{std::cref(obj)});
                        } else {
                            *static_cast<meta_any *>(to) = adl_meta_pointer_like_type::dereference(any_cast<const std::decay_t<Type> &>(from));
                        }
                    }
                }
                break;
            case operation::SEQ:
            case operation::CSEQ:
                if constexpr(is_complete_v<meta_sequence_container_traits<std::decay_t<Type>>>) {
                    *static_cast<meta_sequence_container *>(to) = { std::in_place_type<std::decay_t<Type>>, (op == operation::SEQ ? const_cast<any &>(from).as_ref() : from.as_ref()) };
                }
                break;
            case operation::ASSOC:
            case operation::CASSOC:
                if constexpr(is_complete_v<meta_associative_container_traits<std::decay_t<Type>>>) {
                    *static_cast<meta_associative_container *>(to) = { std::in_place_type<std::decay_t<Type>>, (op == operation::ASSOC ? const_cast<any &>(from).as_ref() : from.as_ref()) };
                }
                break;
            }
        }
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          node{},
          vtable{&basic_vtable<void>}
    {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, Args &&... args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          node{internal::meta_info<Type>::resolve()},
          vtable{&basic_vtable<Type>}
    {}

    /**
     * @brief Constructs a wrapper that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    meta_any(std::reference_wrapper<Type> value)
        : meta_any{std::in_place_type<Type &>, value.get()}
    {}

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)}
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
        : storage{std::move(other.storage)},
          node{std::exchange(other.node, nullptr)},
          vtable{std::exchange(other.vtable, &basic_vtable<void>)}
    {}

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        vtable(operation::DTOR, storage, node);
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This meta any object.
     */
    meta_any & operator=(const meta_any &other) {
        std::exchange(vtable, other.vtable)(operation::DTOR, storage, node);
        storage = other.storage;
        node = other.node;
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This meta any object.
     */
    meta_any & operator=(meta_any &&other) {
        std::exchange(vtable, std::exchange(other.vtable, &basic_vtable<void>))(operation::DTOR, storage, node);
        storage = std::move(other.storage);
        node = std::exchange(other.node, nullptr);
        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This meta any object.
     */
    template<typename Type>
    meta_any & operator=(std::reference_wrapper<Type> value) {
        emplace<Type &>(value.get());
        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This meta any object.
     */
    template<typename Type>
    std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>, meta_any &>
    operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    /**
     * @brief Returns the type of the underlying object.
     * @return The type of the underlying object, if any.
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
     * @sa meta_func::invoke
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param id Unique identifier.
     * @param args Parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
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
     * @return A wrapper containing the value of the underlying variable.
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
            if(const auto info = type_id<Type>(); node->info == info) {
                return any_cast<Type>(&storage);
            } else if(const auto *base = internal::meta_visit<&internal::meta_type_node::base>([info](const auto *curr) { return curr->type()->info == info; }, node); base) {
                return static_cast<const Type *>(base->cast(storage.data()));
            }
        }

        return nullptr;
    }

    /*! @copydoc try_cast */
    template<typename Type>
    [[nodiscard]] Type * try_cast() {
        if(node) {
            if(const auto info = type_id<Type>(); node->info == info) {
                return any_cast<Type>(&storage);
            } else if(const auto *base = internal::meta_visit<&internal::meta_type_node::base>([info](const auto *curr) { return curr->type()->info == info; }, node); base) {
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
     * Attempting to perform an invalid cast results in undefined behavior.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    [[nodiscard]] Type cast() const {
        auto * const instance = try_cast<std::remove_reference_t<Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    /*! @copydoc cast */
    template<typename Type>
    [[nodiscard]] Type cast() {
        // forces const on non-reference types to make them work also with wrappers for const references
        auto * const instance = try_cast<std::remove_reference_t<const Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @tparam Type Type to which the cast is requested.
     * @return A valid meta any object if there exists a viable conversion, an
     * invalid one otherwise.
     */
    template<typename Type>
    [[nodiscard]] meta_any allow_cast() const {
        if(try_cast<std::remove_reference_t<Type>>() != nullptr) {
            return as_ref();
        } else if(node) {
            if(const auto * const conv = internal::meta_visit<&internal::meta_type_node::conv>([info = type_id<Type>()](const auto *curr) { return curr->type()->info == info; }, node); conv) {
                return conv->conv(storage.data());
            }
        }

        return {};
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @tparam Type Type to which the cast is requested.
     * @return True if there exists a viable conversion, false otherwise.
     */
    template<typename Type>
    bool allow_cast() {
        // forces const on non-reference types to make them work also with wrappers for const references
        if(try_cast<std::remove_reference_t<const Type>>() != nullptr) {
            return true;
        } else if(node) {
            if(const auto * const conv = internal::meta_visit<&internal::meta_type_node::conv>([info = type_id<Type>()](const auto *curr) { return curr->type()->info == info; }, node); conv) {
                *this = conv->conv(std::as_const(storage).data());
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
        std::exchange(vtable, &basic_vtable<Type>)(operation::DTOR, storage, node);
        storage.emplace<Type>(std::forward<Args>(args)...);
        node = internal::meta_info<Type>::resolve();
    }

    /*! @brief Destroys contained object */
    void reset() {
        std::exchange(vtable, &basic_vtable<void>)(operation::DTOR, storage, node);
        storage.reset();
        node = nullptr;
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
     * @return A wrapper that shares a reference to an unmanaged object if the
     * wrapped element is dereferenceable, an invalid meta any otherwise.
     */
    [[nodiscard]] meta_any operator*() ENTT_NOEXCEPT {
        meta_any ret{};
        vtable(operation::DEREF, storage, &ret);
        return ret;
    }

    /*! @copydoc operator* */
    [[nodiscard]] meta_any operator*() const ENTT_NOEXCEPT {
        meta_any ret{};
        vtable(operation::CDEREF, storage, &ret);
        return ret;
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
        return (!node && !other.node) || (node && other.node && node->info == other.node->info && storage == other.storage);
    }

    /**
     * @brief Aliasing constructor.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any as_ref() ENTT_NOEXCEPT {
        meta_any ref{};
        vtable(operation::REF, storage, &ref);
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] meta_any as_ref() const ENTT_NOEXCEPT {
        meta_any ref{};
        vtable(operation::CREF, storage, &ref);
        return ref;
    }

private:
    any storage;
    internal::meta_type_node *node;
    vtable_type *vtable;
};


/**
 * @brief Checks if two wrappers differ in their content.
 * @param lhs A wrapper, either empty or not.
 * @param rhs A wrapper, either empty or not.
 * @return True if the two wrappers differ in their content, false otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_any &lhs, const meta_any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Constructs a wrapper from a given type, passing it all arguments.
 * @tparam Type Type of object to use to initialize the wrapper.
 * @tparam Args Types of arguments to use to construct the new instance.
 * @param args Parameters to use to construct the instance.
 * @return A properly initialized wrapper for an object of the given type.
 */
template<typename Type, typename... Args>
meta_any make_meta_any(Args &&... args) {
    return meta_any{std::in_place_type<Type>, std::forward<Args>(args)...};
}


/**
 * @brief Opaque pointers to instances of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance.<br/>
 * Handles are used to generate references to actual objects when needed.
 */
struct meta_handle {
    /*! @brief Default constructor. */
    meta_handle() = default;


    /*! @brief Default copy constructor, deleted on purpose. */
    meta_handle(const meta_handle &) = delete;

    /*! @brief Default move constructor. */
    meta_handle(meta_handle &&) = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This meta handle.
     */
    meta_handle & operator=(const meta_handle &) = delete;

    /**
     * @brief Default move assignment operator.
     * @return This meta handle.
     */
    meta_handle & operator=(meta_handle &&) = default;

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @tparam Type Type of object to use to initialize the handle.
     * @param value An instance of an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(Type &value) ENTT_NOEXCEPT
        : meta_handle{}
    {
        if constexpr(std::is_same_v<std::decay_t<Type>, meta_any>) {
            any = value.as_ref();
        } else {
            any.emplace<Type &>(value);
        }
    }

    /**
     * @brief Returns false if a handle is invalid, true otherwise.
     * @return False if the handle is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(any);
    }

    /**
     * @brief Access operator for accessing the contained opaque object.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any * operator->() {
        return &any;
    }

    /*! @copydoc operator-> */
    [[nodiscard]] const meta_any * operator->() const {
        return &any;
    }

private:
    meta_any any;
};


/*! @brief Opaque wrapper for properties of any type. */
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
     * @brief Returns the stored key as a const reference.
     * @return A wrapper containing the key stored with the property.
     */
    [[nodiscard]] meta_any key() const {
        return node->id.as_ref();
    }

    /**
     * @brief Returns the stored value by copy.
     * @return A wrapper containing the value stored with the property.
     */
    [[nodiscard]] meta_any value() const {
        return node->value;
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for constructors. */
struct meta_ctor {
    /*! @brief Node type. */
    using node_type = internal::meta_ctor_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_ctor(const node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Returns the type to which an object belongs.
     * @return The type to which the object belongs.
     */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a constructor.
     * @return The number of arguments accepted by the constructor.
     */
    [[nodiscard]] size_type arity() const ENTT_NOEXCEPT {
        return node->arity;
    }

    /**
     * @brief Returns the type of the i-th argument of a constructor.
     * @param index Index of the argument of which to return the type.
     * @return The type of the i-th argument of a constructor.
     */
    [[nodiscard]] meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * Parameters must be such that a cast or conversion to the required types
     * is possible. Otherwise, an empty and thus invalid wrapper is returned.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    [[nodiscard]] meta_any invoke(meta_any * const args, const size_type sz) const {
        return sz == arity() ? node->invoke(args) : meta_any{};
    }

    /**
     * @copybrief invoke
     *
     * @sa invoke
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any invoke([[maybe_unused]] Args &&... args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return invoke(arguments, sizeof...(Args));
    }

    /**
     * @brief Returns a range to use to visit all properties.
     * @return An iterable range to use to visit all properties.
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
        return internal::meta_visit<&node_type::prop>([&key](const auto *curr) { return curr->id == key; }, node);
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for data members. */
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

    /*! @copydoc meta_ctor::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Indicates whether a data member is constant or not.
     * @return True if the data member is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a data member is static or not.
     * @return True if the data member is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /*! @copydoc meta_any::type */
    [[nodiscard]] inline meta_type type() const ENTT_NOEXCEPT;

    /**
     * @brief Sets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the data
     * member. Otherwise, invoking the setter results in an undefined
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
     * It must be possible to cast the instance to the parent type of the data
     * member. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
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
        return internal::meta_visit<&node_type::prop>([&key](const auto *curr) { return curr->id == key; }, node);
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for member functions. */
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

    /*! @copydoc meta_ctor::parent */
    [[nodiscard]] inline meta_type parent() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the number of arguments accepted by a member function.
     * @return The number of arguments accepted by the member function.
     */
    [[nodiscard]] size_type arity() const ENTT_NOEXCEPT {
        return node->arity;
    }

    /**
     * @brief Indicates whether a member function is constant or not.
     * @return True if the member function is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a member function is static or not.
     * @return True if the member function is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the return type of a member function.
     * @return The return type of the member function.
     */
    [[nodiscard]] inline meta_type ret() const ENTT_NOEXCEPT;

    /**
     * @brief Returns the type of the i-th argument of a member function.
     * @param index Index of the argument of which to return the type.
     * @return The type of the i-th argument of a member function.
     */
    [[nodiscard]] inline meta_type arg(size_type index) const ENTT_NOEXCEPT;

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a member function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.<br/>
     * It must be possible to cast the instance to the parent type of the member
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(meta_handle instance, meta_any * const args, const size_type sz) const {
        return sz == arity() ? node->invoke(std::move(instance), args) : meta_any{};
    }

    /**
     * @copybrief invoke
     *
     * @sa invoke
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A wrapper containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&... args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return invoke(std::move(instance), arguments, sizeof...(Args));
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
        return internal::meta_visit<&node_type::prop>([&key](const auto *curr) { return curr->id == key; }, node);
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};


/*! @brief Opaque wrapper for types. */
class meta_type {
    static bool can_cast_or_convert(const internal::meta_type_node *type, const type_info info) ENTT_NOEXCEPT {
        if(type->info == info) {
            return true;
        }

        for(const auto *curr = type->conv; curr; curr = curr->next) {
            if(curr->type()->info == info) {
                return true;
            }
        }

        for(const auto *curr = type->base; curr; curr = curr->next) {
            if(auto *target = curr->type(); can_cast_or_convert(target, info)) {
                return true;
            }
        }

        return false;
    }

    template<typename... Args, auto... Index>
    [[nodiscard]] static const internal::meta_ctor_node * ctor(const internal::meta_ctor_node *curr, std::index_sequence<Index...>) {
        for(; curr; curr = curr->next) {
            if(curr->arity == sizeof...(Args) && (can_cast_or_convert(internal::meta_info<Args>::resolve(), curr->arg(Index).info()) && ...)) {
                return curr;
            }
        }

        return nullptr;
    }

public:
    /*! @brief Node type. */
    using node_type = internal::meta_type_node;
    /*! @brief Node type. */
    using base_node_type = internal::meta_base_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_type(node_type *curr = nullptr) ENTT_NOEXCEPT
        : node{curr}
    {}

    /**
     * @brief Constructs an instance from a given base node.
     * @param curr The base node with which to construct the instance.
     */
    meta_type(base_node_type *curr) ENTT_NOEXCEPT
        : node{curr ? curr->type() : nullptr}
    {}

    /**
     * @brief Returns the type info object of the underlying type.
     * @return The type info object of the underlying type.
     */
    [[nodiscard]] type_info info() const ENTT_NOEXCEPT {
        return node->info;
    }

    /**
     * @brief Returns the identifier assigned to a type.
     * @return The identifier assigned to the type.
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
     * @return True if the type is a sequence container, false otherwise.
     */
    [[nodiscard]] bool is_sequence_container() const ENTT_NOEXCEPT {
        return node->is_sequence_container;
    }

    /**
     * @brief Checks whether a type refers to an associative container or not.
     * @return True if the type is an associative container, false otherwise.
     */
    [[nodiscard]] bool is_associative_container() const ENTT_NOEXCEPT {
        return node->is_associative_container;
    }

    /**
     * @brief Checks whether a type refers to a recognized class template
     * specialization or not.
     * @return True if the type is a recognized class template specialization,
     * false otherwise.
     */
    [[nodiscard]] bool is_template_specialization() const ENTT_NOEXCEPT {
        return node->template_info.is_template_specialization;
    }

    /**
     * @brief Returns the number of template arguments, if any.
     * @return The number of template arguments, if any.
     */
    [[nodiscard]] size_type template_arity() const ENTT_NOEXCEPT {
        return node->template_info.arity;
    }

    /**
     * @brief Returns a tag for the class template of the underlying type.
     *
     * @sa meta_class_template_tag
     *
     * @return The tag for the class template of the underlying type.
     */
    [[nodiscard]] inline meta_type template_type() const ENTT_NOEXCEPT {
        return is_template_specialization() ?  node->template_info.type() : meta_type{};
    }

    /**
     * @brief Returns the type of the i-th template argument of a type.
     * @param index Index of the template argument of which to return the type.
     * @return The type of the i-th template argument of a type.
     */
    [[nodiscard]] inline meta_type template_arg(size_type index) const ENTT_NOEXCEPT {
        return index < template_arity() ? node->template_info.arg(index) : meta_type{};
    }

    /**
     * @brief Provides the number of dimensions of an array type.
     * @return The number of dimensions in case of array types, 0 otherwise.
     */
    [[nodiscard]] size_type rank() const ENTT_NOEXCEPT {
        return node->rank;
    }

    /**
     * @brief The number of elements along the given dimension of an array type.
     * @param dim The dimension of which to return the number of elements.
     * @return The number of elements along the given dimension in case of array
     * types, 0 otherwise.
     */
    [[nodiscard]] size_type extent(size_type dim = {}) const ENTT_NOEXCEPT {
        return node->extent(dim);
    }

    /**
     * @brief Provides the type for which the pointer is defined.
     * @return The type for which the pointer is defined or this type if it
     * doesn't refer to a pointer type.
     */
    [[nodiscard]] meta_type remove_pointer() const ENTT_NOEXCEPT {
        return node->remove_pointer();
    }

    /**
     * @brief Provides the type for which the array is defined.
     * @return The type for which the array is defined or this type if it
     * doesn't refer to an array type.
     */
    [[nodiscard]] meta_type remove_extent() const ENTT_NOEXCEPT {
        return node->remove_extent();
    }

    /**
     * @brief Returns a range to use to visit top-level base meta types.
     * @return An iterable range to use to visit top-level base meta types.
     */
    [[nodiscard]] meta_range<meta_type, internal::meta_base_node> base() const ENTT_NOEXCEPT {
        return node->base;
    }

    /**
     * @brief Returns the base meta type associated with a given identifier.
     * @param id Unique identifier.
     * @return The base meta type associated with the given identifier, if any.
     */
    [[nodiscard]] meta_type base(const id_type id) const {
        return internal::meta_visit<&node_type::base>([id](const auto *curr) { return curr->type()->id == id; }, node);
    }

    /**
     * @brief Returns a range to use to visit top-level constructors.
     * @return An iterable range to use to visit top-level constructors.
     */
    [[nodiscard]] meta_range<meta_ctor> ctor() const ENTT_NOEXCEPT {
        return node->ctor;
    }

    /**
     * @brief Returns a constructor for a given list of types of arguments.
     * @tparam Args Constructor arguments.
     * @return The requested constructor, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_ctor ctor() const {
        return ctor<Args...>(node->ctor, std::make_index_sequence<sizeof...(Args)>{});
    }

    /**
     * @brief Returns a range to use to visit top-level data.
     * @return An iterable range to use to visit top-level data.
     */
    [[nodiscard]] meta_range<meta_data> data() const ENTT_NOEXCEPT {
        return node->data;
    }

    /**
     * @brief Returns the data associated with a given identifier.
     *
     * The data of the base classes will also be visited, if any.
     *
     * @param id Unique identifier.
     * @return The data associated with the given identifier, if any.
     */
    [[nodiscard]] meta_data data(const id_type id) const {
        return internal::meta_visit<&node_type::data>([id](const auto *curr) { return curr->id == id; }, node);
    }

    /**
     * @brief Returns a range to use to visit top-level functions.
     * @return An iterable range to use to visit top-level functions.
     */
    [[nodiscard]] meta_range<meta_func> func() const ENTT_NOEXCEPT {
        return node->func;
    }

    /**
     * @brief Returns the function associated with a given identifier.
     *
     * The functions of the base classes will also be visited, if any.<br/>
     * In the case of overloaded functions, the first one with the required
     * identifier will be returned.
     *
     * @param id Unique identifier.
     * @return The function associated with the given identifier, if any.
     */
    [[nodiscard]] meta_func func(const id_type id) const {
        return internal::meta_visit<&node_type::func>([id](const auto *curr) { return curr->id == id; }, node);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * Parameters must be such that a cast or conversion to the required types
     * is possible. Otherwise, an empty and thus invalid wrapper is returned.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    [[nodiscard]] meta_any construct(meta_any * const args, const size_type sz) const {
        meta_any ret{};
        internal::meta_visit<&node_type::ctor>([args, sz, &ret](const auto *curr) { return (curr->arity == sz) && (ret = curr->invoke(args)); }, node);
        return ret;
    }

    /**
     * @copybrief construct
     *
     * @sa construct
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any construct(Args &&... args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return construct(arguments, sizeof...(Args));
    }

    /**
     * @brief Invokes a function given an identifier, if possible.
     *
     * It must be possible to cast the instance to the parent type of the member
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @sa meta_func::invoke
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(const id_type id, meta_handle instance, meta_any * const args, const size_type sz) const {
        const internal::meta_func_node* candidate{};
        size_type extent{sz + 1u};
        bool ambiguous{};

        for(auto *it = internal::meta_visit<&node_type::func>([id, sz](const auto *curr) { return curr->id == id && curr->arity == sz; }, node); it && it->id == id && it->arity == sz; it = it->next) {
            size_type direct{};
            size_type ext{};

            for(size_type next{}; next < sz && next == (direct + ext); ++next) {
                const auto type = args[next].type();
                const auto req = it->arg(next).info();
                type.info() == req ? ++direct : (ext += can_cast_or_convert(type.node, req));
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

        return (candidate && !ambiguous) ? candidate->invoke(std::move(instance), args) : meta_any{};
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
     * @return A wrapper containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(const id_type id, meta_handle instance, Args &&... args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return invoke(id, std::move(instance), arguments, sizeof...(Args));
    }

    /**
     * @brief Sets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the data
     * member. Otherwise, invoking the setter results in an undefined
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
     * It must be possible to cast the instance to the parent type of the data
     * member. Otherwise, invoking the getter results in an undefined behavior.
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(const id_type id, meta_handle instance) const {
        auto const candidate = data(id);
        return candidate ? candidate.get(std::move(instance)) : meta_any{};
    }

    /**
     * @brief Returns a range to use to visit top-level properties.
     * @return An iterable range to use to visit top-level properties.
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
        return internal::meta_visit<&node_type::prop>([&key](const auto *curr) { return curr->id == key; }, node);
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two objects refer to the same type.
     * @param other The object with which to compare.
     * @return True if the objects refer to the same type, false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_type &other) const ENTT_NOEXCEPT {
        return (!node && !other.node) || (node && other.node && node->info == other.node->info);
    }

    /**
     * @brief Resets a type and all its parts.
     *
     * This function resets a type and all its data members, member functions
     * and properties, as well as its constructors, destructors and conversion
     * functions if any.<br/>
     * Base classes aren't reset but the link between the two types is removed.
     *
     * The type is also removed from the list of searchable types.
     */
    void reset() ENTT_NOEXCEPT {
        auto** it = internal::meta_context::global();

        while(*it && *it != node) {
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
        node->ctor = node->def_ctor;
        node->dtor = nullptr;
    }

private:
    node_type *node;
};


/**
 * @brief Checks if two objects refer to the same type.
 * @param lhs An object, either valid or not.
 * @param rhs An object, either valid or not.
 * @return False if the objects refer to the same node, true otherwise.
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


[[nodiscard]] inline meta_type meta_ctor::parent() const ENTT_NOEXCEPT {
    return node->parent;
}


[[nodiscard]] inline meta_type meta_ctor::arg(size_type index) const ENTT_NOEXCEPT {
    return index < arity() ? node->arg(index) : meta_type{};
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
    return index < arity() ? node->arg(index) : meta_type{};
}


/*! @brief Opaque iterator for sequence containers. */
class meta_sequence_container::meta_iterator {
    /*! @brief A sequence container can access the underlying iterator. */
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
            static_cast<meta_any *>(to)->emplace<typename std::iterator_traits<It>::reference>(*any_cast<const It &>(from));
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
     * @brief Checks if two iterators refer to the same element.
     * @param other The iterator with which to compare.
     * @return True if the iterators refer to the same element, false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return handle == other.handle;
    }

    /**
     * @brief Checks if two iterators refer to the same element.
     * @param other The iterator with which to compare.
     * @return False if the iterators refer to the same element, true otherwise.
     */
    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    /**
     * @brief Indirection operator.
     * @return The element to which the iterator points.
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
    vtable_type *vtable{};
    any handle{};
};


template<typename Type>
struct meta_sequence_container::meta_sequence_container_proxy {
    using traits_type = meta_sequence_container_traits<Type>;

    [[nodiscard]] static meta_type value_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename Type::value_type>::resolve();
    }

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
        return traits_type::size(any_cast<const Type &>(container));
    }

    [[nodiscard]] static bool resize(any &container, size_type sz) {
        auto * const cont = any_cast<Type>(&container);
        return cont ? traits_type::resize(*cont, sz) : false;
    }

    [[nodiscard]] static bool clear(any &container) {
        auto * const cont = any_cast<Type>(&container);
        return cont ? traits_type::clear(*cont) : false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            return iterator{traits_type::begin(*cont)};
        }

        return iterator{traits_type::cbegin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            return iterator{traits_type::end(*cont)};
        }

        return iterator{traits_type::cend(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static std::pair<iterator, bool> insert(any &container, iterator it, meta_any &value) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
            if(value.allow_cast<typename Type::const_reference>() || value.allow_cast<typename Type::value_type>()) {
                const auto *element = value.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                auto ret = traits_type::insert(*cont, any_cast<const typename Type::iterator &>(it.handle), element ? *element : value.cast<typename Type::value_type>());
                return { iterator{std::move(ret.first)}, ret.second };
            }
        }

        return {};
    }

    [[nodiscard]] static std::pair<iterator, bool> erase(any &container, iterator it) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            auto ret = traits_type::erase(*cont, any_cast<const typename Type::iterator &>(it.handle));
            return { iterator{std::move(ret.first)}, ret.second };
        }

        return {};
    }

    [[nodiscard]] static meta_any get(any &container, size_type pos) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            return meta_any{std::in_place_type<typename Type::reference>, traits_type::get(*cont, pos)};
        }

        return meta_any{std::in_place_type<typename Type::const_reference>, traits_type::cget(any_cast<const Type &>(container), pos)};
    }
};


/**
 * @brief Returns the meta value type of a container.
 * @return The meta value type of the container.
 */
[[nodiscard]] inline meta_type meta_sequence_container::value_type() const ENTT_NOEXCEPT {
    return value_type_fn();
}


/**
 * @brief Returns the size of a container.
 * @return The size of the container.
 */
[[nodiscard]] inline meta_sequence_container::size_type meta_sequence_container::size() const ENTT_NOEXCEPT {
    return size_fn(storage);
}


/**
 * @brief Resizes a container to contain a given number of elements.
 * @param sz The new size of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::resize(size_type sz) {
    return resize_fn(storage, sz);
}


/**
 * @brief Clears the content of a container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::clear() {
    return clear_fn(storage);
}


/**
 * @brief Returns an iterator to the first element of a container.
 * @return An iterator to the first element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    return begin_fn(storage);
}


/**
 * @brief Returns an iterator that is past the last element of a container.
 * @return An iterator that is past the last element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    return end_fn(storage);
}


/**
 * @brief Inserts an element at a specified location of a container.
 * @param it Iterator before which the element will be inserted.
 * @param value Element value to insert.
 * @return A pair consisting of an iterator to the inserted element (in case of
 * success) and a bool denoting whether the insertion took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::insert(iterator it, meta_any value) {
    return insert_fn(storage, it, value);
}


/**
 * @brief Removes a given element from a container.
 * @param it Iterator to the element to remove.
 * @return A pair consisting of an iterator following the last removed element
 * (in case of success) and a bool denoting whether the insertion took place.
 */
inline std::pair<meta_sequence_container::iterator, bool> meta_sequence_container::erase(iterator it) {
    return erase_fn(storage, it);
}


/**
 * @brief Returns a reference to the element at a given location of a container
 * (no bounds checking is performed).
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


/*! @brief Opaque iterator for associative containers. */
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
                static_cast<std::pair<meta_any, meta_any> *>(to)->first = std::cref(*any_cast<const It &>(from));
            } else {
                *static_cast<std::pair<meta_any, meta_any> *>(to) = std::make_pair<meta_any, meta_any>(std::cref(any_cast<const It &>(from)->first), std::ref(any_cast<const It &>(from)->second));
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
     * @brief Constructs an meta iterator from a given iterator.
     * @tparam KeyOnly True if the container is also key-only, false otherwise.
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
     * @brief Checks if two iterators refer to the same element.
     * @param other The iterator with which to compare.
     * @return True if the iterators refer to the same element, false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return handle == other.handle;
    }

    /**
     * @brief Checks if two iterators refer to the same element.
     * @param other The iterator with which to compare.
     * @return False if the iterators refer to the same element, true otherwise.
     */
    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    /**
     * @brief Indirection operator.
     * @return The element to which the iterator points.
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
    vtable_type *vtable{};
    any handle{};
};


template<typename Type>
struct meta_associative_container::meta_associative_container_proxy {
    using traits_type = meta_associative_container_traits<Type>;

    [[nodiscard]] static meta_type key_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename Type::key_type>::resolve();
    }

    [[nodiscard]] static meta_type mapped_type() ENTT_NOEXCEPT {
        if constexpr(is_key_only_meta_associative_container_v<Type>) {
            return meta_type{};
        } else {
            return internal::meta_info<typename Type::mapped_type>::resolve();
        }
    }

    [[nodiscard]] static meta_type value_type() ENTT_NOEXCEPT {
        return internal::meta_info<typename Type::value_type>::resolve();
    }

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
        return traits_type::size(any_cast<const Type &>(container));
    }

    [[nodiscard]] static bool clear(any &container) {
        auto * const cont = any_cast<Type>(&container);
        return cont ? traits_type::clear(*cont) : false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::begin(*cont)};
        }

        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cbegin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto * const cont = any_cast<Type>(&container); cont) {
            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::end(*cont)};
        }

        return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cend(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static bool insert(any &container, meta_any &key, meta_any &value) {
        if(auto * const cont = any_cast<Type>(&container); cont && key.allow_cast<const typename Type::key_type &>()) {
            if constexpr(is_key_only_meta_associative_container_v<Type>) {
                return traits_type::insert(*cont, key.cast<const typename Type::key_type &>());
            } else {
                if(value.allow_cast<const typename Type::mapped_type &>()) {
                    return traits_type::insert(*cont, key.cast<const typename Type::key_type &>(), value.cast<const typename Type::mapped_type &>());
                }
            }
        }

        return false;
    }

    [[nodiscard]] static bool erase(any &container, meta_any &key) {
        if(auto * const cont = any_cast<Type>(&container); cont && key.allow_cast<const typename Type::key_type &>()) {
            return traits_type::erase(*cont, key.cast<const typename Type::key_type &>());
        }

        return false;
    }

    [[nodiscard]] static iterator find(any &container, meta_any &key) {
        if(key.allow_cast<const typename Type::key_type &>()) {
            if(auto * const cont = any_cast<Type>(&container); cont) {
                return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::find(*cont, key.cast<const typename Type::key_type &>())};
            }

            return iterator{is_key_only_meta_associative_container<Type>{}, traits_type::cfind(any_cast<const Type &>(container), key.cast<const typename Type::key_type &>())};
        }

        return {};
    }
};


/**
 * @brief Returns true if a container is also key-only, false otherwise.
 * @return True if the associative container is also key-only, false otherwise.
 */
[[nodiscard]] inline bool meta_associative_container::key_only() const ENTT_NOEXCEPT {
    return key_only_container;
}


/**
 * @brief Returns the meta key type of a container.
 * @return The meta key type of the a container.
 */
[[nodiscard]] inline meta_type meta_associative_container::key_type() const ENTT_NOEXCEPT {
    return key_type_fn();
}


/**
 * @brief Returns the meta mapped type of a container.
 * @return The meta mapped type of the a container.
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
 * @brief Inserts an element (a key/value pair) into a container.
 * @param key The key of the element to insert.
 * @param value The value of the element to insert.
 * @return A bool denoting whether the insertion took place.
 */
inline bool meta_associative_container::insert(meta_any key, meta_any value = {}) {
    return insert_fn(storage, key, value);
}


/**
 * @brief Removes the specified element from a container.
 * @param key The key of the element to remove.
 * @return A bool denoting whether the removal took place.
 */
inline bool meta_associative_container::erase(meta_any key) {
    return erase_fn(storage, key);
}


/**
 * @brief Returns an iterator to the element with a given key, if any.
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
