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
#include "ctx.hpp"
#include "internal.hpp"
#include "range.hpp"


namespace entt {


class meta_type;
class meta_any;


/*! @brief Proxy object for containers of any type. */
class meta_container {
    /*! @brief A meta_any is allowed to create proxies. */
    friend class meta_any;

    struct meta_iterator;

    struct meta_view {
        [[nodiscard]] virtual std::size_t size(const void *) const ENTT_NOEXCEPT = 0;
        [[nodiscard]] virtual bool insert(void *, meta_any, meta_any) = 0;
        [[nodiscard]] virtual bool erase(void *, meta_any) = 0;
        [[nodiscard]] virtual meta_any begin(void *) const ENTT_NOEXCEPT = 0;
        [[nodiscard]] virtual meta_any end(void *) const ENTT_NOEXCEPT = 0;
        [[nodiscard]] virtual meta_any find(void *, meta_any) const ENTT_NOEXCEPT = 0;
        [[nodiscard]] virtual meta_any deref(meta_any) const ENTT_NOEXCEPT = 0;
        virtual void incr(meta_any) const ENTT_NOEXCEPT = 0;
    };

    meta_container(meta_view *impl, void *container)
        : view{impl},
          instance{container}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_container()
        : view{},
          instance{}
    {}

    [[nodiscard]] inline size_type size() const ENTT_NOEXCEPT;
    inline bool insert(meta_any, meta_any);
    inline bool erase(meta_any);
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    [[nodiscard]] inline iterator operator[](meta_any pos_or_key);
    [[nodiscard]] inline explicit operator bool() const ENTT_NOEXCEPT;

private:
    meta_view *view;
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

    template<typename, typename = void>
    struct container_view {
        [[nodiscard]] static meta_container::meta_view * instance() {
            return nullptr;
        }
    };

    template<typename Type>
    struct container_view<Type, std::enable_if_t<is_sequence_container_v<Type>>>: meta_container::meta_view {
        [[nodiscard]] static meta_container::meta_view * instance() {
            static container_view common{};
            return &common;
        }

        [[nodiscard]] std::size_t size(const void *container) const ENTT_NOEXCEPT override {
            return static_cast<const Type *>(container)->size();
        }

        [[nodiscard]] bool insert(void *container, meta_any it, meta_any value) override {
            bool ret = false;

            if constexpr(is_dynamic_sequence_container_v<Type>) {
                if(auto *iter = it.try_cast<typename Type::iterator>(); iter) {
                    if(const auto *curr = value.try_cast<typename Type::value_type>(); curr) {
                        *iter = static_cast<Type *>(container)->insert(*iter, *curr);
                        ret = true;
                    }
                }
            }

            return ret;
        }

        [[nodiscard]] bool erase(void *container, meta_any it) override {
            bool ret = false;

            if constexpr(is_dynamic_sequence_container_v<Type>) {
                if(auto *iter = it.try_cast<typename Type::iterator>(); iter) {
                    *iter = static_cast<Type *>(container)->erase(*iter);
                    ret = true;
                }
            }

            return ret;
        }

        [[nodiscard]] meta_any begin(void *container) const ENTT_NOEXCEPT override {
            return static_cast<Type *>(container)->begin();
        }

        [[nodiscard]] meta_any end(void *container) const ENTT_NOEXCEPT override {
            return static_cast<Type *>(container)->end();
        }

        [[nodiscard]] meta_any deref(meta_any it) const ENTT_NOEXCEPT override {
            return std::ref(*it.cast<typename Type::iterator>());
        }

        [[nodiscard]] meta_any find(void *container, meta_any idx) const ENTT_NOEXCEPT override {
            meta_any any{};

            if(const auto *curr = idx.try_cast<std::size_t>(); curr) {
                any = std::next(static_cast<Type *>(container)->begin(), *curr);
            }

            return any;
        }

        void incr(meta_any it) const ENTT_NOEXCEPT override {
            ++it.cast<typename Type::iterator>();
        }
    };

    template<typename Type>
    struct container_view<Type, std::enable_if_t<is_associative_container_v<Type>>>: meta_container::meta_view {
        [[nodiscard]] static meta_container::meta_view * instance() {
            static container_view common{};
            return &common;
        }

        [[nodiscard]] std::size_t size(const void *container) const ENTT_NOEXCEPT override {
            return static_cast<const Type *>(container)->size();
        }

        [[nodiscard]] bool insert(void *container, meta_any key, meta_any value) override {
            bool ret = false;

            if constexpr(is_key_only_associative_container_v<Type>) {
                if(const auto *curr = key.try_cast<typename Type::key_type>(); curr) {
                    static_cast<Type *>(container)->insert(*curr);
                    ret = true;
                }
            } else {
                if(const auto *k_curr = key.try_cast<typename Type::key_type>(); k_curr) {
                    if(const auto *v_curr = value.try_cast<typename Type::mapped_type>(); v_curr) {
                        static_cast<Type *>(container)->insert(std::make_pair(*k_curr, *v_curr));
                        ret = true;
                    }
                }
            }

            return ret;
        }

        [[nodiscard]] bool erase(void *container, meta_any key) override {
            bool ret = false;

            if(const auto *curr = key.try_cast<typename Type::key_type>(); curr) {
                static_cast<Type *>(container)->erase(*curr);
                ret = true;
            }

            return ret;
        }

        [[nodiscard]] meta_any begin(void *container) const ENTT_NOEXCEPT override {
            return static_cast<Type *>(container)->begin();
        }

        [[nodiscard]] meta_any end(void *container) const ENTT_NOEXCEPT override {
            return static_cast<Type *>(container)->end();
        }

        [[nodiscard]] meta_any deref(meta_any it) const ENTT_NOEXCEPT override {
            if constexpr(is_key_only_associative_container_v<Type>) {
                return *it.cast<typename Type::iterator>();
            } else {
                return std::ref(*it.cast<typename Type::iterator>());
            }
        }

        [[nodiscard]] meta_any find(void *container, meta_any key) const ENTT_NOEXCEPT override {
            meta_any any{};

            if constexpr(is_key_only_associative_container_v<Type>) {
                if(const auto *curr = key.try_cast<typename Type::key_type>(); curr) {
                    auto *cont = static_cast<Type *>(container);
                    any = std::find(cont->begin(), cont->end(), *curr);
                }
            } else {
                if(const auto *curr = key.try_cast<typename Type::key_type>(); curr) {
                    any = static_cast<Type *>(container)->find(*curr);
                }
            }

            return any;
        }

        void incr(meta_any it) const ENTT_NOEXCEPT override {
            it.cast<typename Type::iterator>().operator++();
        }
    };

    template<typename Type>
    static meta_any dereference_operator(meta_any &any) {
        meta_any other{};

        if constexpr(is_dereferenceable_v<Type>) {
            if constexpr(std::is_const_v<std::remove_reference_t<decltype(*std::declval<Type>())>>) {
                other = *any.cast<Type>();
            } else {
                other = std::ref(*any.cast<Type>());
            }
        }

        return other;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          node{},
          deref{},
          cview{}
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
          cview{container_view<Type>::instance()}
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
          cview{container_view<Type>::instance()}
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
        const void *ret = nullptr;

        if(node) {
            if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node->type_id == type_id) {
                ret = storage.data();
            } else if(const auto *base = internal::find_if<&internal::meta_type_node::base>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); base) {
                ret = base->cast(storage.data());
            }
        }

        return static_cast<const Type *>(ret);
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
        meta_any any{};

        if(node) {
            if(const auto type_id = internal::meta_info<Type>::resolve()->type_id; node->type_id == type_id) {
                any = *this;
            } else if(const auto * const conv = internal::find_if<&internal::meta_type_node::conv>([type_id](const auto *curr) { return curr->type()->type_id == type_id; }, node); conv) {
                any = conv->conv(storage.data());
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
        other.cview = cview;
        return other;
    }

    /**
     * @brief Returns a container view.
     * @return A container view for the underlying object.
     */
    [[nodiscard]] meta_container view() ENTT_NOEXCEPT {
        return { cview, storage.data() };
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
        std::swap(lhs.node, rhs.node);
        std::swap(lhs.storage, rhs.storage);
        std::swap(lhs.deref, rhs.deref);
        std::swap(lhs.cview, rhs.cview);
    }

private:
    internal::meta_storage storage;
    const internal::meta_type_node *node;
    dereference_operator_type *deref;
    meta_container::meta_view *cview;
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


/*! @brief Opaque iterator for meta containers. */
struct meta_container::meta_iterator {
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
     * @brief Constructs a meta iterator that wraps an actual iterator.
     * @param ref A proxy object that _knows_ how to use the wrapped iterator.
     * @param iter The actual iterator, properly wrapped.
     */
    meta_iterator(meta_container::meta_view *ref, meta_any iter)
        : view{ref},
          it{std::move(iter)}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return view->incr(handle()), *this;
    }

    /*! @brief Post-increment operator. @return This iterator. */
    meta_iterator operator++(int) ENTT_NOEXCEPT {
        iterator orig = *this;
        return view->incr(handle()), orig;
    }

    /**
     * @brief Checks if two meta iterators refer to the same element.
     * @param other The meta iterator with which to compare.
     * @return True if the two meta iterators refer to the same element, false
     * otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return it == other.it;
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
        return view->deref(handle());
    }

    /**
     * @brief Returns a handle to the underlying iterator.
     * @return The actual iterator, properly wrapped.
     */
    [[nodiscard]] meta_any handle() const ENTT_NOEXCEPT {
        return it.ref();
    }

    /**
     * @brief Returns false if an iterator is invalid, true otherwise.
     * @return False if the iterator is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(it);
    }

private:
    meta_container::meta_view *view;
    entt::meta_any it;
};


/**
 * @brief Returns the number of elements.
 * @return Number of elements.
 */
[[nodiscard]] meta_container::size_type meta_container::size() const ENTT_NOEXCEPT {
    return view->size(instance);
}


/**
 * @brief Inserts an element in the container.
 *
 * In case of sequence containers, the first parameter must be a valid iterator
 * handle. In case of associative containers, the type of the key must be such
 * that a cast or conversion to the key type of the container is possible.<br/>
 * The type of the value must be such that a cast or conversion to the value
 * type of the container is possible.
 *
 * @param it_or_key A valid iterator handle in case of sequence containers, a
 * key in case of associative containers.
 * @param value The element to insert in the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_container::insert(meta_any it_or_key, meta_any value = {}) {
    return view->insert(instance, std::move(it_or_key), std::move(value));
}


/**
 * @brief Erases an element from the container.
 *
 * In case of sequence containers, the parameter must be a valid iterator
 * handle. In case of associative containers, the type of the key must be such
 * that a cast or conversion to the key type of the container is possible.
 *
 * @param it_or_key A valid iterator handle in case of sequence containers, a
 * key in case of associative containers.
 * @return True in case of success, false otherwise.
 */
inline bool meta_container::erase(meta_any it_or_key) {
    return view->erase(instance, std::move(it_or_key));
}


/**
 * @brief Returns an iterator to the first element of the container.
 * @return An iterator to the first element of the container.
 */
[[nodiscard]] inline meta_container::iterator meta_container::begin() {
    return {view, view->begin(instance)};
}


/**
 * @brief Returns an iterator that is past the last element of the container.
 * @return An iterator that is past the last element of the container.
 */
[[nodiscard]] inline meta_container::iterator meta_container::end() {
    return {view, view->end(instance)};
}


/**
 * @brief Returns an iterator to the required element.
 *
 * In case of sequence containers, the parameter must be a valid position (no
 * bounds checking is performed) and such that a cast or conversion to size_type
 * is possible. In case of associative containers, the type of the key must be
 * such that a cast or conversion to the key type of the container is possible.
 *
 * @param pos_or_key A valid position in case of sequence containers, a key in
 * case of associative containers.
 * @return An iterator to the required element.
 */
[[nodiscard]] inline meta_container::iterator meta_container::operator[](meta_any pos_or_key) {
    return {view, view->find(instance, std::move(pos_or_key))};
}


/**
 * @brief Returns true if a meta container is valid, false otherwise.
 * @return True if the meta container is valid, false otherwise.
 */
meta_container::operator bool() const ENTT_NOEXCEPT {
    return (view != nullptr);
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
    meta_handle()
        : any{}
    {}

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
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any invoke([[maybe_unused]] Args &&... args) const {
        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return sizeof...(Args) == size() ? node->invoke(arguments.data()) : meta_any{};
    }

    /**
     * @brief Returns a range to use to visit all meta properties.
     * @return An iterable range to use to visit all meta properties.
     */
    meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Iterates all meta properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use prop() and entt::meta_range<meta_prop> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>>
    prop(Op op) const {
        for(auto curr: prop()) {
            op(curr);
        }
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
    meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Iterates all meta properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use prop() and entt::meta_range<meta_prop> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>>
    prop(Op op) const {
        for(auto curr: prop()) {
            op(curr);
        }
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
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&... args) const {
        meta_any any{};

        if(sizeof...(Args) == size()) {
            std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
            any = node->invoke(std::move(instance), arguments.data());
        }

        return any;
    }

    /*! @copydoc meta_ctor::prop */
    meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Iterates all meta properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use prop() and entt::meta_range<meta_prop> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>>
    prop(Op op) const {
        for(auto curr: prop()) {
            op(curr);
        }
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
    meta_type(const node_type *curr = nullptr) ENTT_NOEXCEPT
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
     * @brief Checks whether a type is dereferenceable or not.
     * @return True if the underlying type is dereferenceable, false otherwise.
     */
    [[nodiscard]] bool is_dereferenceable() const ENTT_NOEXCEPT {
        return node->is_dereferenceable;
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
    meta_range<meta_base> base() const ENTT_NOEXCEPT {
        return node->base;
    }

    /**
     * @brief Iterates all top-level meta bases of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use base() and entt::meta_range<meta_base> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_base>>
    base(Op op) const {
        for(auto curr: base()) {
            op(curr);
        }
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
    meta_range<meta_conv> conv() const ENTT_NOEXCEPT {
        return node->conv;
    }

    /**
     * @brief Iterates all top-level meta conversion functions of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use conv() and entt::meta_range<meta_conv> instead")]]
    void conv(Op op) const {
        for(auto curr: conv()) {
            op(curr);
        }
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
    meta_range<meta_ctor> ctor() const ENTT_NOEXCEPT {
        return node->ctor;
    }

    /**
     * @brief Iterates all top-level meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use ctor() and entt::meta_range<meta_ctor> instead")]]
    void ctor(Op op) const {
        for(auto curr: ctor()) {
            op(curr);
        }
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
    meta_range<meta_data> data() const ENTT_NOEXCEPT {
        return node->data;
    }

    /**
     * @brief Iterates all top-level meta data of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use ctor() and entt::meta_range<meta_ctor> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_data>>
    data(Op op) const {
        for(auto curr: data()) {
            op(curr);
        }
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
    meta_range<meta_func> func() const ENTT_NOEXCEPT {
        return node->func;
    }

    /**
     * @brief Iterates all top-level meta functions of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use ctor() and entt::meta_range<meta_ctor> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_func>>
    func(Op op) const {
        for(auto curr: func()) {
            op(curr);
        }
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
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any construct(Args &&... args) const {
        auto construct_if = [this](meta_any *params) {
            meta_any any{};

            internal::find_if<&node_type::ctor>([params, &any](const auto *curr) {
                return (curr->size == sizeof...(args)) && (any = curr->invoke(params));
            }, node);

            return any;
        };

        std::array<meta_any, sizeof...(Args)> arguments{std::forward<Args>(args)...};
        return construct_if(arguments.data());
    }

    /**
     * @brief Returns a range to use to visit top-level meta properties.
     * @return An iterable range to use to visit top-level meta properties.
     */
    meta_range<meta_prop> prop() const ENTT_NOEXCEPT {
        return node->prop;
    }

    /**
     * @brief Iterates all top-level meta properties assigned to a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    [[deprecated("use prop() and entt::meta_range<meta_prop> instead")]]
    std::enable_if_t<std::is_invocable_v<Op, meta_prop>>
    prop(Op op) const {
        for(auto curr: prop()) {
            op(curr);
        }
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

    /*! @brief Removes a meta object from the list of searchable types. */
    void detach() ENTT_NOEXCEPT {
        internal::meta_context::detach(node);
    }

private:
    const node_type *node;
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


}


#endif
