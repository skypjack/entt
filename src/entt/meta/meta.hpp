#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "adl_pointer.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "node.hpp"
#include "range.hpp"
#include "type_traits.hpp"

namespace entt {

class meta_any;
class meta_type;

/*! @brief Proxy object for sequence containers. */
class meta_sequence_container {
    class meta_iterator;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_sequence_container() noexcept = default;

    /**
     * @brief Construct a proxy object for sequence containers.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    meta_sequence_container(std::in_place_type_t<Type>, any instance) noexcept
        : value_type_node{internal::meta_node<std::remove_cv_t<std::remove_reference_t<typename Type::value_type>>>::resolve()},
          size_fn{&meta_sequence_container_traits<Type>::size},
          resize_fn{&meta_sequence_container_traits<Type>::resize},
          iter_fn{&meta_sequence_container_traits<Type>::iter},
          insert_or_erase_fn{&meta_sequence_container_traits<Type>::insert_or_erase},
          storage{std::move(instance)} {}

    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool resize(const size_type);
    inline bool clear();
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline iterator insert(iterator, meta_any);
    inline iterator erase(iterator);
    [[nodiscard]] inline meta_any operator[](const size_type);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    internal::meta_type_node *value_type_node = nullptr;
    size_type (*size_fn)(const any &) noexcept = nullptr;
    bool (*resize_fn)(any &, size_type) = nullptr;
    iterator (*iter_fn)(any &, const bool) = nullptr;
    iterator (*insert_or_erase_fn)(any &, const any &, meta_any &) = nullptr;
    any storage{};
};

/*! @brief Proxy object for associative containers. */
class meta_associative_container {
    class meta_iterator;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_associative_container() noexcept = default;

    /**
     * @brief Construct a proxy object for associative containers.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    meta_associative_container(std::in_place_type_t<Type>, any instance) noexcept
        : key_only_container{meta_associative_container_traits<Type>::key_only},
          key_type_node{internal::meta_node<std::remove_cv_t<std::remove_reference_t<typename Type::key_type>>>::resolve()},
          mapped_type_node{nullptr},
          value_type_node{internal::meta_node<std::remove_cv_t<std::remove_reference_t<typename Type::value_type>>>::resolve()},
          size_fn{&meta_associative_container_traits<Type>::size},
          clear_fn{&meta_associative_container_traits<Type>::clear},
          iter_fn{&meta_associative_container_traits<Type>::iter},
          insert_or_erase_fn{&meta_associative_container_traits<Type>::insert_or_erase},
          find_fn{&meta_associative_container_traits<Type>::find},
          storage{std::move(instance)} {
        if constexpr(!meta_associative_container_traits<Type>::key_only) {
            mapped_type_node = internal::meta_node<std::remove_cv_t<std::remove_reference_t<typename Type::mapped_type>>>::resolve();
        }
    }

    [[nodiscard]] inline bool key_only() const noexcept;
    [[nodiscard]] inline meta_type key_type() const noexcept;
    [[nodiscard]] inline meta_type mapped_type() const noexcept;
    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool clear();
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline bool insert(meta_any, meta_any);
    inline size_type erase(meta_any);
    [[nodiscard]] inline iterator find(meta_any);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    bool key_only_container{};
    internal::meta_type_node *key_type_node = nullptr;
    internal::meta_type_node *mapped_type_node = nullptr;
    internal::meta_type_node *value_type_node = nullptr;
    size_type (*size_fn)(const any &) noexcept = nullptr;
    bool (*clear_fn)(any &) = nullptr;
    iterator (*iter_fn)(any &, const bool) = nullptr;
    size_type (*insert_or_erase_fn)(any &, meta_any &, meta_any &) = nullptr;
    iterator (*find_fn)(any &, meta_any &) = nullptr;
    any storage{};
};

/*! @brief Opaque wrapper for values of any type. */
class meta_any {
    enum class operation : std::uint8_t {
        deref,
        seq,
        assoc
    };

    using vtable_type = void(const operation, const any &, void *);

    template<typename Type>
    static void basic_vtable([[maybe_unused]] const operation op, [[maybe_unused]] const any &value, [[maybe_unused]] void *other) {
        static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, Type>, "Invalid type");

        if constexpr(!std::is_void_v<Type>) {
            switch(op) {
            case operation::deref:
                if constexpr(is_meta_pointer_like_v<Type>) {
                    if constexpr(std::is_function_v<typename std::pointer_traits<Type>::element_type>) {
                        *static_cast<meta_any *>(other) = any_cast<Type>(value);
                    } else if constexpr(!std::is_same_v<std::remove_const_t<typename std::pointer_traits<Type>::element_type>, void>) {
                        using in_place_type = decltype(adl_meta_pointer_like<Type>::dereference(any_cast<const Type &>(value)));

                        if constexpr(std::is_constructible_v<bool, Type>) {
                            if(const auto &pointer_like = any_cast<const Type &>(value); pointer_like) {
                                static_cast<meta_any *>(other)->emplace<in_place_type>(adl_meta_pointer_like<Type>::dereference(pointer_like));
                            }
                        } else {
                            static_cast<meta_any *>(other)->emplace<in_place_type>(adl_meta_pointer_like<Type>::dereference(any_cast<const Type &>(value)));
                        }
                    }
                }
                break;
            case operation::seq:
                if constexpr(is_complete_v<meta_sequence_container_traits<Type>>) {
                    *static_cast<meta_sequence_container *>(other) = {std::in_place_type<Type>, std::move(const_cast<any &>(value))};
                }
                break;
            case operation::assoc:
                if constexpr(is_complete_v<meta_associative_container_traits<Type>>) {
                    *static_cast<meta_associative_container *>(other) = {std::in_place_type<Type>, std::move(const_cast<any &>(value))};
                }
                break;
            }
        }
    }

    void release() {
        if(node && node->dtor && owner()) {
            node->dtor(storage.data());
        }
    }

    meta_any(const meta_any &other, any ref) noexcept
        : storage{std::move(ref)},
          node{storage ? other.node : nullptr},
          vtable{storage ? other.vtable : &basic_vtable<void>} {}

public:
    /*! @brief Default constructor. */
    meta_any() noexcept
        : storage{},
          node{},
          vtable{&basic_vtable<void>} {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, Args &&...args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          node{internal::meta_node<std::remove_cv_t<std::remove_reference_t<Type>>>::resolve()},
          vtable{&basic_vtable<std::remove_cv_t<std::remove_reference_t<Type>>>} {}

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)} {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    meta_any(const meta_any &other) = default;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other) noexcept
        : storage{std::move(other.storage)},
          node{std::exchange(other.node, nullptr)},
          vtable{std::exchange(other.vtable, &basic_vtable<void>)} {}

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        release();
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This meta any object.
     */
    meta_any &operator=(const meta_any &other) {
        release();
        vtable = other.vtable;
        storage = other.storage;
        node = other.node;
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This meta any object.
     */
    meta_any &operator=(meta_any &&other) noexcept {
        release();
        vtable = std::exchange(other.vtable, &basic_vtable<void>);
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
    std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>, meta_any &>
    operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    /*! @copydoc any::type */
    [[nodiscard]] inline meta_type type() const noexcept;

    /*! @copydoc any::data */
    [[nodiscard]] const void *data() const noexcept {
        return storage.data();
    }

    /*! @copydoc any::data */
    [[nodiscard]] void *data() noexcept {
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
    meta_any invoke(const id_type id, Args &&...args) const;

    /*! @copydoc invoke */
    template<typename... Args>
    meta_any invoke(const id_type id, Args &&...args);

    /**
     * @brief Sets the value of a given variable.
     *
     * The type of the value is such that a cast or conversion to the type of
     * the variable is possible. Otherwise, invoking the setter does nothing.
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
    [[nodiscard]] const Type *try_cast() const {
        auto *self = any_cast<Type>(&storage);

        for(auto *it = node ? node->base : nullptr; it && !self; it = it->next) {
            const auto &as_const = it->cast(as_ref());
            self = as_const.template try_cast<Type>();
        }

        return self;
    }

    /*! @copydoc try_cast */
    template<typename Type>
    [[nodiscard]] Type *try_cast() {
        auto *self = any_cast<Type>(&storage);

        for(auto *it = node ? node->base : nullptr; it && !self; it = it->next) {
            self = it->cast(as_ref()).template try_cast<Type>();
        }

        return self;
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the cast is possible.
     *
     * @warning
     * Attempting to perform an invalid cast results is undefined behavior.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    [[nodiscard]] Type cast() const {
        auto *const instance = try_cast<std::remove_reference_t<Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    /*! @copydoc cast */
    template<typename Type>
    [[nodiscard]] Type cast() {
        // forces const on non-reference types to make them work also with wrappers for const references
        auto *const instance = try_cast<std::remove_reference_t<const Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @param type Meta type to which the cast is requested.
     * @return A valid meta any object if there exists a viable conversion, an
     * invalid one otherwise.
     */
    [[nodiscard]] meta_any allow_cast(const meta_type &type) const;

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @param type Meta type to which the cast is requested.
     * @return True if there exists a viable conversion, false otherwise.
     */
    [[nodiscard]] bool allow_cast(const meta_type &type) {
        if(auto other = std::as_const(*this).allow_cast(type); other) {
            if(other.owner()) {
                std::swap(*this, other);
            }

            return true;
        }

        return false;
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @tparam Type Type to which the cast is requested.
     * @return A valid meta any object if there exists a viable conversion, an
     * invalid one otherwise.
     */
    template<typename Type>
    [[nodiscard]] meta_any allow_cast() const {
        const auto other = allow_cast(internal::meta_node<std::remove_cv_t<std::remove_reference_t<Type>>>::resolve());
        return (!std::is_reference_v<Type> || std::is_const_v<std::remove_reference_t<Type>> || other.owner()) ? other : meta_any{};
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @tparam Type Type to which the cast is requested.
     * @return True if there exists a viable conversion, false otherwise.
     */
    template<typename Type>
    bool allow_cast() {
        if(auto other = std::as_const(*this).allow_cast(internal::meta_node<std::remove_cv_t<std::remove_reference_t<Type>>>::resolve()); other) {
            if(other.owner()) {
                std::swap(*this, other);
                return true;
            }

            return (static_cast<constness_as_t<any, std::remove_reference_t<const Type>> &>(storage).data() != nullptr);
        }

        return false;
    }

    /*! @copydoc any::emplace */
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        release();
        vtable = &basic_vtable<std::remove_cv_t<std::remove_reference_t<Type>>>;
        storage.emplace<Type>(std::forward<Args>(args)...);
        node = internal::meta_node<std::remove_cv_t<std::remove_reference_t<Type>>>::resolve();
    }

    /*! @copydoc any::assign */
    bool assign(const meta_any &other);

    /*! @copydoc any::assign */
    bool assign(meta_any &&other);

    /*! @copydoc any::reset */
    void reset() {
        release();
        vtable = &basic_vtable<void>;
        storage.reset();
        node = nullptr;
    }

    /**
     * @brief Returns a sequence container proxy.
     * @return A sequence container proxy for the underlying object.
     */
    [[nodiscard]] meta_sequence_container as_sequence_container() noexcept {
        any detached = storage.as_ref();
        meta_sequence_container proxy;
        vtable(operation::seq, detached, &proxy);
        return proxy;
    }

    /*! @copydoc as_sequence_container */
    [[nodiscard]] meta_sequence_container as_sequence_container() const noexcept {
        any detached = storage.as_ref();
        meta_sequence_container proxy;
        vtable(operation::seq, detached, &proxy);
        return proxy;
    }

    /**
     * @brief Returns an associative container proxy.
     * @return An associative container proxy for the underlying object.
     */
    [[nodiscard]] meta_associative_container as_associative_container() noexcept {
        any detached = storage.as_ref();
        meta_associative_container proxy;
        vtable(operation::assoc, detached, &proxy);
        return proxy;
    }

    /*! @copydoc as_associative_container */
    [[nodiscard]] meta_associative_container as_associative_container() const noexcept {
        any detached = storage.as_ref();
        meta_associative_container proxy;
        vtable(operation::assoc, detached, &proxy);
        return proxy;
    }

    /**
     * @brief Indirection operator for dereferencing opaque objects.
     * @return A wrapper that shares a reference to an unmanaged object if the
     * wrapped element is dereferenceable, an invalid meta any otherwise.
     */
    [[nodiscard]] meta_any operator*() const noexcept {
        meta_any ret{};
        vtable(operation::deref, storage, &ret);
        return ret;
    }

    /**
     * @brief Returns false if a wrapper is invalid, true otherwise.
     * @return False if the wrapper is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(node == nullptr);
    }

    /*! @copydoc any::operator== */
    [[nodiscard]] bool operator==(const meta_any &other) const {
        return (!node && !other.node) || (node && other.node && *node->info == *other.node->info && storage == other.storage);
    }

    /*! @copydoc any::as_ref */
    [[nodiscard]] meta_any as_ref() noexcept {
        return meta_any{*this, storage.as_ref()};
    }

    /*! @copydoc any::as_ref */
    [[nodiscard]] meta_any as_ref() const noexcept {
        return meta_any{*this, storage.as_ref()};
    }

    /*! @copydoc any::owner */
    [[nodiscard]] bool owner() const noexcept {
        return storage.owner();
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
[[nodiscard]] inline bool operator!=(const meta_any &lhs, const meta_any &rhs) noexcept {
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
meta_any make_meta(Args &&...args) {
    return meta_any{std::in_place_type<Type>, std::forward<Args>(args)...};
}

/**
 * @brief Forwards its argument and avoids copies for lvalue references.
 * @tparam Type Type of argument to use to construct the new instance.
 * @param value Parameter to use to construct the instance.
 * @return A properly initialized and not necessarily owning wrapper.
 */
template<typename Type>
meta_any forward_as_meta(Type &&value) {
    return meta_any{std::in_place_type<std::conditional_t<std::is_rvalue_reference_v<Type>, std::decay_t<Type>, Type>>, std::forward<Type>(value)};
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
    meta_handle()
        : any{} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    meta_handle(const meta_handle &) = delete;

    /*! @brief Default move constructor. */
    meta_handle(meta_handle &&) = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This meta handle.
     */
    meta_handle &operator=(const meta_handle &) = delete;

    /**
     * @brief Default move assignment operator.
     * @return This meta handle.
     */
    meta_handle &operator=(meta_handle &&) = default;

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @tparam Type Type of object to use to initialize the handle.
     * @param value An instance of an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(Type &value) noexcept
        : meta_handle{} {
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
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(any);
    }

    /**
     * @brief Access operator for accessing the contained opaque object.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] meta_any *operator->() {
        return &any;
    }

    /*! @copydoc operator-> */
    [[nodiscard]] const meta_any *operator->() const {
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
    meta_prop(const node_type *curr = nullptr) noexcept
        : node{curr} {}

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
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};

/*! @brief Opaque wrapper for data members. */
struct meta_data {
    /*! @brief Node type. */
    using node_type = internal::meta_data_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_data(const node_type *curr = nullptr) noexcept
        : node{curr} {}

    /*! @copydoc meta_type::id */
    [[nodiscard]] id_type id() const noexcept {
        return node->id;
    }

    /**
     * @brief Returns the number of setters available.
     * @return The number of setters available.
     */
    [[nodiscard]] size_type arity() const noexcept {
        return node->arity;
    }

    /**
     * @brief Indicates whether a data member is constant or not.
     * @return True if the data member is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_const);
    }

    /**
     * @brief Indicates whether a data member is static or not.
     * @return True if the data member is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_static);
    }

    /*! @copydoc meta_any::type */
    [[nodiscard]] inline meta_type type() const noexcept;

    /**
     * @brief Sets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the data
     * member.<br/>
     * The type of the value is such that a cast or conversion to the type of
     * the variable is possible. Otherwise, invoking the setter does nothing.
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
     * member.
     *
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(meta_handle instance) const {
        return node->get(std::move(instance));
    }

    /**
     * @brief Returns the type accepted by the i-th setter.
     * @param index Index of the setter of which to return the accepted type.
     * @return The type accepted by the i-th setter.
     */
    [[nodiscard]] inline meta_type arg(const size_type index) const noexcept;

    /**
     * @brief Returns a range to visit registered meta properties.
     * @return An iterable range to visit registered meta properties.
     */
    [[nodiscard]] meta_range<meta_prop> prop() const noexcept {
        return {node->prop, nullptr};
    }

    /**
     * @brief Lookup function for registered meta properties.
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        for(auto curr: prop()) {
            if(curr.key() == key) {
                return curr;
            }
        }

        return nullptr;
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
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
    meta_func(const node_type *curr = nullptr) noexcept
        : node{curr} {}

    /*! @copydoc meta_type::id */
    [[nodiscard]] id_type id() const noexcept {
        return node->id;
    }

    /**
     * @brief Returns the number of arguments accepted by a member function.
     * @return The number of arguments accepted by the member function.
     */
    [[nodiscard]] size_type arity() const noexcept {
        return node->arity;
    }

    /**
     * @brief Indicates whether a member function is constant or not.
     * @return True if the member function is constant, false otherwise.
     */
    [[nodiscard]] bool is_const() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_const);
    }

    /**
     * @brief Indicates whether a member function is static or not.
     * @return True if the member function is static, false otherwise.
     */
    [[nodiscard]] bool is_static() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_static);
    }

    /**
     * @brief Returns the return type of a member function.
     * @return The return type of the member function.
     */
    [[nodiscard]] inline meta_type ret() const noexcept;

    /**
     * @brief Returns the type of the i-th argument of a member function.
     * @param index Index of the argument of which to return the type.
     * @return The type of the i-th argument of a member function.
     */
    [[nodiscard]] inline meta_type arg(const size_type index) const noexcept;

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a member function, the parameters must be such that a cast or
     * conversion to the required types is possible. Otherwise, an empty and
     * thus invalid wrapper is returned.<br/>
     * It must be possible to cast the instance to the parent type of the member
     * function.
     *
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(meta_handle instance, meta_any *const args, const size_type sz) const {
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
    meta_any invoke(meta_handle instance, Args &&...args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return invoke(std::move(instance), arguments, sizeof...(Args));
    }

    /*! @copydoc meta_data::prop */
    [[nodiscard]] meta_range<meta_prop> prop() const noexcept {
        return {node->prop, nullptr};
    }

    /**
     * @brief Lookup function for registered meta properties.
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        for(auto curr: prop()) {
            if(curr.key() == key) {
                return curr;
            }
        }

        return nullptr;
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(node == nullptr);
    }

private:
    const node_type *node;
};

/*! @brief Opaque wrapper for types. */
class meta_type {
    template<auto Member, typename... Check>
    [[nodiscard]] std::decay_t<decltype(std::declval<internal::meta_type_node>().*Member)> lookup(meta_any *const args, const typename internal::meta_type_node::size_type sz, Check... check) const {
        std::decay_t<decltype(node->*Member)> candidate{};
        size_type extent{sz + 1u};
        bool ambiguous{};

        for(auto *curr = (node->*Member); curr; curr = curr->next) {
            if(((curr->id == check) && ... && (curr->arity == sz))) {
                size_type direct{};
                size_type ext{};

                for(size_type next{}; next < sz && next == (direct + ext) && args[next]; ++next) {
                    const auto type = args[next].type();
                    const auto other = curr->arg(next);

                    if(const auto &info = other.info(); info == type.info()) {
                        ++direct;
                    } else {
                        ext += internal::find_by<&node_type::base>(info, type.node)
                               || internal::find_by<&node_type::conv>(info, type.node)
                               || (type.node->conversion_helper && other.node->conversion_helper);
                    }
                }

                if((direct + ext) == sz) {
                    if(ext < extent) {
                        candidate = curr;
                        extent = ext;
                        ambiguous = false;
                    } else if(ext == extent) {
                        ambiguous = true;
                    }
                }
            }
        }

        return (candidate && !ambiguous) ? candidate : decltype(candidate){};
    }

public:
    /*! @brief Node type. */
    using node_type = internal::meta_type_node;
    /*! @brief Node type. */
    using base_node_type = internal::meta_base_node;
    /*! @brief Unsigned integer type. */
    using size_type = typename node_type::size_type;

    /*! @copydoc meta_prop::meta_prop */
    meta_type(const node_type *curr = nullptr) noexcept
        : node{curr} {}

    /**
     * @brief Constructs an instance from a given base node.
     * @param curr The base node with which to construct the instance.
     */
    meta_type(const base_node_type *curr) noexcept
        : node{curr ? curr->type : nullptr} {}

    /**
     * @brief Returns the type info object of the underlying type.
     * @return The type info object of the underlying type.
     */
    [[nodiscard]] const type_info &info() const noexcept {
        return *node->info;
    }

    /**
     * @brief Returns the identifier assigned to a type.
     * @return The identifier assigned to the type.
     */
    [[nodiscard]] id_type id() const noexcept {
        return node->id;
    }

    /**
     * @brief Returns the size of the underlying type if known.
     * @return The size of the underlying type if known, 0 otherwise.
     */
    [[nodiscard]] size_type size_of() const noexcept {
        return node->size_of;
    }

    /**
     * @brief Checks whether a type refers to an arithmetic type or not.
     * @return True if the underlying type is an arithmetic type, false
     * otherwise.
     */
    [[nodiscard]] bool is_arithmetic() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_arithmetic);
    }

    /**
     * @brief Checks whether a type refers to an integral type or not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    [[nodiscard]] bool is_integral() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_integral);
    }

    /**
     * @brief Checks whether a type refers to a signed type or not.
     * @return True if the underlying type is a signed type, false otherwise.
     */
    [[nodiscard]] bool is_signed() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_signed);
    }

    /**
     * @brief Checks whether a type refers to an array type or not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    [[nodiscard]] bool is_array() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_array);
    }

    /**
     * @brief Checks whether a type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    [[nodiscard]] bool is_enum() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_enum);
    }

    /**
     * @brief Checks whether a type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    [[nodiscard]] bool is_class() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_class);
    }

    /**
     * @brief Checks whether a type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    [[nodiscard]] bool is_pointer() const noexcept {
        return node != node->remove_pointer();
    }

    /**
     * @brief Provides the type for which the pointer is defined.
     * @return The type for which the pointer is defined or this type if it
     * doesn't refer to a pointer type.
     */
    [[nodiscard]] meta_type remove_pointer() const noexcept {
        return node->remove_pointer();
    }

    /**
     * @brief Checks whether a type is a pointer-like type or not.
     * @return True if the underlying type is a pointer-like one, false
     * otherwise.
     */
    [[nodiscard]] bool is_pointer_like() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_meta_pointer_like);
    }

    /**
     * @brief Checks whether a type refers to a sequence container or not.
     * @return True if the type is a sequence container, false otherwise.
     */
    [[nodiscard]] bool is_sequence_container() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_meta_sequence_container);
    }

    /**
     * @brief Checks whether a type refers to an associative container or not.
     * @return True if the type is an associative container, false otherwise.
     */
    [[nodiscard]] bool is_associative_container() const noexcept {
        return static_cast<bool>(node->traits & internal::meta_traits::is_meta_associative_container);
    }

    /**
     * @brief Checks whether a type refers to a recognized class template
     * specialization or not.
     * @return True if the type is a recognized class template specialization,
     * false otherwise.
     */
    [[nodiscard]] bool is_template_specialization() const noexcept {
        return (node->templ != nullptr);
    }

    /**
     * @brief Returns the number of template arguments.
     * @return The number of template arguments.
     */
    [[nodiscard]] size_type template_arity() const noexcept {
        return node->templ ? node->templ->arity : size_type{};
    }

    /**
     * @brief Returns a tag for the class template of the underlying type.
     *
     * @sa meta_class_template_tag
     *
     * @return The tag for the class template of the underlying type.
     */
    [[nodiscard]] inline meta_type template_type() const noexcept {
        return node->templ ? node->templ->type : meta_type{};
    }

    /**
     * @brief Returns the type of the i-th template argument of a type.
     * @param index Index of the template argument of which to return the type.
     * @return The type of the i-th template argument of a type.
     */
    [[nodiscard]] inline meta_type template_arg(const size_type index) const noexcept {
        return index < template_arity() ? node->templ->arg(index) : meta_type{};
    }

    /**
     * @brief Returns a range to visit registered top-level base meta types.
     * @return An iterable range to visit registered top-level base meta types.
     */
    [[nodiscard]] meta_range<meta_type, internal::meta_base_node> base() const noexcept {
        return {node->base, nullptr};
    }

    /**
     * @brief Lookup function for registered base meta types.
     * @param id Unique identifier.
     * @return The registered base meta type for the given identifier, if any.
     */
    [[nodiscard]] meta_type base(const id_type id) const {
        return internal::find_by<&node_type::base>(id, node);
    }

    /**
     * @brief Returns a range to visit registered top-level meta data.
     * @return An iterable range to visit registered top-level meta data.
     */
    [[nodiscard]] meta_range<meta_data> data() const noexcept {
        return {node->data, nullptr};
    }

    /**
     * @brief Lookup function for registered meta data.
     *
     * Registered meta data of base classes will also be visited.
     *
     * @param id Unique identifier.
     * @return The registered meta data for the given identifier, if any.
     */
    [[nodiscard]] meta_data data(const id_type id) const {
        return internal::find_by<&node_type::data>(id, node);
    }

    /**
     * @brief Returns a range to visit registered top-level functions.
     * @return An iterable range to visit registered top-level functions.
     */
    [[nodiscard]] meta_range<meta_func> func() const noexcept {
        return {node->func, nullptr};
    }

    /**
     * @brief Lookup function for registered meta functions.
     *
     * Registered meta functions of base classes will also be visited.<br/>
     * In case of overloaded functions, the first one with the required
     * identifier will be returned.
     *
     * @param id Unique identifier.
     * @return The registered meta function for the given identifier, if any.
     */
    [[nodiscard]] meta_func func(const id_type id) const {
        return internal::find_by<&node_type::func>(id, node);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * Parameters are such that a cast or conversion to the required types is
     * possible. Otherwise, an empty and thus invalid wrapper is returned.<br/>
     * If suitable, the implicitly generated default constructor is used.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    [[nodiscard]] meta_any construct(meta_any *const args, const size_type sz) const {
        const auto *candidate = lookup<&node_type::ctor>(args, sz);
        return candidate ? candidate->invoke(args) : ((!sz && node->default_constructor) ? node->default_constructor() : meta_any{});
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
    [[nodiscard]] meta_any construct(Args &&...args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return construct(arguments, sizeof...(Args));
    }

    /**
     * @brief Wraps an opaque element of the underlying type.
     * @param element A valid pointer to an element of the underlying type.
     * @return A wrapper that references the given instance.
     */
    meta_any from_void(void *element) const {
        return (element && node->from_void) ? node->from_void(element, nullptr) : meta_any{};
    }

    /*! @copydoc from_void */
    meta_any from_void(const void *element) const {
        return (element && node->from_void) ? node->from_void(nullptr, element) : meta_any{};
    }

    /**
     * @brief Invokes a function given an identifier, if possible.
     *
     * It must be possible to cast the instance to the parent type of the member
     * function.
     *
     * @sa meta_func::invoke
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(const id_type id, meta_handle instance, meta_any *const args, const size_type sz) const {
        const auto *candidate = lookup<&node_type::func>(args, sz, id);

        for(auto it = base().begin(), last = base().end(); it != last && !candidate; ++it) {
            candidate = it->lookup<&node_type::func>(args, sz, id);
        }

        return candidate ? candidate->invoke(std::move(instance), args) : meta_any{};
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
    meta_any invoke(const id_type id, meta_handle instance, Args &&...args) const {
        meta_any arguments[sizeof...(Args) + 1u]{std::forward<Args>(args)...};
        return invoke(id, std::move(instance), arguments, sizeof...(Args));
    }

    /**
     * @brief Sets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the data
     * member.<br/>
     * The type of the value is such that a cast or conversion to the type of
     * the variable is possible. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(const id_type id, meta_handle instance, Type &&value) const {
        const auto candidate = data(id);
        return candidate && candidate.set(std::move(instance), std::forward<Type>(value));
    }

    /**
     * @brief Gets the value of a given variable.
     *
     * It must be possible to cast the instance to the parent type of the data
     * member.
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(const id_type id, meta_handle instance) const {
        const auto candidate = data(id);
        return candidate ? candidate.get(std::move(instance)) : meta_any{};
    }

    /**
     * @brief Returns a range to visit registered top-level meta properties.
     * @return An iterable range to visit registered top-level meta properties.
     */
    [[nodiscard]] meta_range<meta_prop> prop() const noexcept {
        return {node->prop, nullptr};
    }

    /**
     * @brief Lookup function for meta properties.
     *
     * Properties of base classes are also visited.
     *
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(meta_any key) const {
        return internal::find_by<&internal::meta_type_node::prop>(key, node);
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(node == nullptr);
    }

    /**
     * @brief Checks if two objects refer to the same type.
     * @param other The object with which to compare.
     * @return True if the objects refer to the same type, false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_type &other) const noexcept {
        return (!node && !other.node) || (node && other.node && *node->info == *other.node->info);
    }

private:
    const node_type *node;
};

/**
 * @brief Checks if two objects refer to the same type.
 * @param lhs An object, either valid or not.
 * @param rhs An object, either valid or not.
 * @return False if the objects refer to the same node, true otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_type &lhs, const meta_type &rhs) noexcept {
    return !(lhs == rhs);
}

[[nodiscard]] inline meta_type meta_any::type() const noexcept {
    return node;
}

template<typename... Args>
meta_any meta_any::invoke(const id_type id, Args &&...args) const {
    return type().invoke(id, *this, std::forward<Args>(args)...);
}

template<typename... Args>
meta_any meta_any::invoke(const id_type id, Args &&...args) {
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

[[nodiscard]] inline meta_any meta_any::allow_cast(const meta_type &type) const {
    if(const auto &info = type.info(); node && *node->info == info) {
        return as_ref();
    } else if(node) {
        for(auto *it = node->conv; it; it = it->next) {
            if(*it->type->info == info) {
                return it->conv(*this);
            }
        }

        if(node->conversion_helper && (type.is_arithmetic() || type.is_enum())) {
            // exploits the fact that arithmetic types and enums are also default constructible
            auto other = type.construct();
            ENTT_ASSERT(other.node->conversion_helper, "Conversion helper not found");
            const auto value = node->conversion_helper(nullptr, storage.data());
            other.node->conversion_helper(other.storage.data(), &value);
            return other;
        }

        for(auto *it = node->base; it; it = it->next) {
            const auto &as_const = it->cast(as_ref());

            if(auto other = as_const.allow_cast(type); other) {
                return other;
            }
        }
    }

    return {};
}

inline bool meta_any::assign(const meta_any &other) {
    auto value = other.allow_cast(node);
    return value && storage.assign(std::move(value.storage));
}

inline bool meta_any::assign(meta_any &&other) {
    if(*node->info == *other.node->info) {
        return storage.assign(std::move(other.storage));
    }

    return assign(std::as_const(other));
}

[[nodiscard]] inline meta_type meta_data::type() const noexcept {
    return node->type;
}

[[nodiscard]] inline meta_type meta_func::ret() const noexcept {
    return node->ret;
}

[[nodiscard]] inline meta_type meta_data::arg(const size_type index) const noexcept {
    return index < arity() ? node->arg(index) : meta_type{};
}

[[nodiscard]] inline meta_type meta_func::arg(const size_type index) const noexcept {
    return index < arity() ? node->arg(index) : meta_type{};
}

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

class meta_sequence_container::meta_iterator final {
    friend class meta_sequence_container;

    enum class operation : std::uint8_t {
        incr,
        deref
    };

    using vtable_type = void(const operation, const any &, const std::ptrdiff_t, meta_any *);

    template<typename It>
    static void basic_vtable(const operation op, const any &value, const std::ptrdiff_t offset, meta_any *other) {
        switch(op) {
        case operation::incr: {
            auto &it = any_cast<It &>(const_cast<any &>(value));
            it = std::next(it, offset);
        } break;
        case operation::deref: {
            const auto &it = any_cast<const It &>(value);
            other->emplace<decltype(*it)>(*it);
        } break;
        }
    }

public:
    using difference_type = std::ptrdiff_t;
    using value_type = meta_any;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    constexpr meta_iterator() noexcept
        : vtable{},
          handle{} {}

    template<typename It>
    explicit meta_iterator(It iter) noexcept
        : vtable{&basic_vtable<It>},
          handle{std::move(iter)} {}

    meta_iterator &operator++() noexcept {
        vtable(operation::incr, handle, 1, nullptr);
        return *this;
    }

    meta_iterator operator++(int value) noexcept {
        meta_iterator orig = *this;
        vtable(operation::incr, handle, ++value, nullptr);
        return orig;
    }

    meta_iterator &operator--() noexcept {
        vtable(operation::incr, handle, -1, nullptr);
        return *this;
    }

    meta_iterator operator--(int value) noexcept {
        meta_iterator orig = *this;
        vtable(operation::incr, handle, --value, nullptr);
        return orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other;
        vtable(operation::deref, handle, 0, &other);
        return other;
    }

    [[nodiscard]] pointer operator->() const {
        return operator*();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle);
    }

    [[nodiscard]] bool operator==(const meta_iterator &other) const noexcept {
        return handle == other.handle;
    }

    [[nodiscard]] bool operator!=(const meta_iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    vtable_type *vtable;
    any handle;
};

class meta_associative_container::meta_iterator final {
    enum class operation : std::uint8_t {
        incr,
        deref
    };

    using vtable_type = void(const operation, const any &, std::pair<meta_any, meta_any> *);

    template<bool KeyOnly, typename It>
    static void basic_vtable(const operation op, const any &value, std::pair<meta_any, meta_any> *other) {
        switch(op) {
        case operation::incr:
            ++any_cast<It &>(const_cast<any &>(value));
            break;
        case operation::deref:
            const auto &it = any_cast<const It &>(value);
            if constexpr(KeyOnly) {
                other->first.emplace<decltype(*it)>(*it);
            } else {
                other->first.emplace<decltype((it->first))>(it->first);
                other->second.emplace<decltype((it->second))>(it->second);
            }
            break;
        }
    }

public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<meta_any, meta_any>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    constexpr meta_iterator() noexcept
        : vtable{},
          handle{} {}

    template<bool KeyOnly, typename It>
    meta_iterator(std::integral_constant<bool, KeyOnly>, It iter) noexcept
        : vtable{&basic_vtable<KeyOnly, It>},
          handle{std::move(iter)} {}

    meta_iterator &operator++() noexcept {
        vtable(operation::incr, handle, nullptr);
        return *this;
    }

    meta_iterator operator++(int) noexcept {
        meta_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other;
        vtable(operation::deref, handle, &other);
        return other;
    }

    [[nodiscard]] pointer operator->() const {
        return operator*();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle);
    }

    [[nodiscard]] bool operator==(const meta_iterator &other) const noexcept {
        return handle == other.handle;
    }

    [[nodiscard]] bool operator!=(const meta_iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    vtable_type *vtable;
    any handle;
};

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Returns the meta value type of a container.
 * @return The meta value type of the container.
 */
[[nodiscard]] inline meta_type meta_sequence_container::value_type() const noexcept {
    return value_type_node;
}

/**
 * @brief Returns the size of a container.
 * @return The size of the container.
 */
[[nodiscard]] inline meta_sequence_container::size_type meta_sequence_container::size() const noexcept {
    return size_fn(storage);
}

/**
 * @brief Resizes a container to contain a given number of elements.
 * @param sz The new size of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::resize(const size_type sz) {
    return resize_fn(storage, sz);
}

/**
 * @brief Clears the content of a container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::clear() {
    return resize_fn(storage, 0u);
}

/**
 * @brief Returns an iterator to the first element of a container.
 * @return An iterator to the first element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    return iter_fn(storage, false);
}

/**
 * @brief Returns an iterator that is past the last element of a container.
 * @return An iterator that is past the last element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    return iter_fn(storage, true);
}

/**
 * @brief Inserts an element at a specified location of a container.
 * @param it Iterator before which the element will be inserted.
 * @param value Element value to insert.
 * @return A possibly invalid iterator to the inserted element.
 */
inline meta_sequence_container::iterator meta_sequence_container::insert(iterator it, meta_any value) {
    return insert_or_erase_fn(storage, it.handle, value);
}

/**
 * @brief Removes a given element from a container.
 * @param it Iterator to the element to remove.
 * @return A possibly invalid iterator following the last removed element.
 */
inline meta_sequence_container::iterator meta_sequence_container::erase(iterator it) {
    return insert(std::move(it), {});
}

/**
 * @brief Returns a reference to the element at a given location of a container
 * (no bounds checking is performed).
 * @param pos The position of the element to return.
 * @return A reference to the requested element properly wrapped.
 */
[[nodiscard]] inline meta_any meta_sequence_container::operator[](const size_type pos) {
    auto it = begin();
    it.operator++(static_cast<int>(pos) - 1);
    return *it;
}

/**
 * @brief Returns false if a proxy is invalid, true otherwise.
 * @return False if the proxy is invalid, true otherwise.
 */
[[nodiscard]] inline meta_sequence_container::operator bool() const noexcept {
    return static_cast<bool>(storage);
}

/**
 * @brief Returns true if a container is also key-only, false otherwise.
 * @return True if the associative container is also key-only, false otherwise.
 */
[[nodiscard]] inline bool meta_associative_container::key_only() const noexcept {
    return key_only_container;
}

/**
 * @brief Returns the meta key type of a container.
 * @return The meta key type of the a container.
 */
[[nodiscard]] inline meta_type meta_associative_container::key_type() const noexcept {
    return key_type_node;
}

/**
 * @brief Returns the meta mapped type of a container.
 * @return The meta mapped type of the a container.
 */
[[nodiscard]] inline meta_type meta_associative_container::mapped_type() const noexcept {
    return mapped_type_node;
}

/*! @copydoc meta_sequence_container::value_type */
[[nodiscard]] inline meta_type meta_associative_container::value_type() const noexcept {
    return value_type_node;
}

/*! @copydoc meta_sequence_container::size */
[[nodiscard]] inline meta_associative_container::size_type meta_associative_container::size() const noexcept {
    return size_fn(storage);
}

/*! @copydoc meta_sequence_container::clear */
inline bool meta_associative_container::clear() {
    return clear_fn(storage);
}

/*! @copydoc meta_sequence_container::begin */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::begin() {
    return iter_fn(storage, false);
}

/*! @copydoc meta_sequence_container::end */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::end() {
    return iter_fn(storage, true);
}

/**
 * @brief Inserts an element (a key/value pair) into a container.
 * @param key The key of the element to insert.
 * @param value The value of the element to insert.
 * @return A bool denoting whether the insertion took place.
 */
inline bool meta_associative_container::insert(meta_any key, meta_any value = std::in_place_type<void>) {
    return (insert_or_erase_fn(storage, key, value) != 0u);
}

/**
 * @brief Removes the specified element from a container.
 * @param key The key of the element to remove.
 * @return A bool denoting whether the removal took place.
 */
inline meta_associative_container::size_type meta_associative_container::erase(meta_any key) {
    return insert(std::move(key), {});
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
[[nodiscard]] inline meta_associative_container::operator bool() const noexcept {
    return static_cast<bool>(storage);
}

} // namespace entt

#endif
