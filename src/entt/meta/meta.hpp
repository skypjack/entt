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
#include "../locator/locator.hpp"
#include "adl_pointer.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "node.hpp"
#include "range.hpp"
#include "type_traits.hpp"

namespace entt {

class meta_any;
class meta_type;

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

enum class meta_container_operation {
    size,
    clear,
    reserve,
    resize,
    begin,
    end,
    insert,
    erase,
    find
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/*! @brief Proxy object for sequence containers. */
class meta_sequence_container {
    class meta_iterator;
    using operation = internal::meta_container_operation;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_sequence_container() noexcept
        : meta_sequence_container{locator<meta_ctx>::value_or()} {}

    /**
     * @brief Context aware constructor.
     * @param area The context from which to search for meta types.
     */
    meta_sequence_container(const meta_ctx &area) noexcept
        : ctx{&area} {}

    /**
     * @brief Rebinds a proxy object to a sequence container type.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    void rebind(any instance) noexcept {
        ENTT_ASSERT(instance.type() == type_id<Type>(), "Unexpected type");
        value_type_node = &internal::resolve<typename Type::value_type>;
        vtable = &meta_sequence_container_traits<Type>::basic_vtable;
        storage = std::move(instance);
    }

    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool resize(const size_type);
    inline bool clear();
    inline bool reserve(const size_type);
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline iterator insert(iterator, meta_any);
    inline iterator erase(iterator);
    [[nodiscard]] inline meta_any operator[](const size_type);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    const meta_ctx *ctx{};
    internal::meta_type_node (*value_type_node)(const internal::meta_context &){};
    size_type (*vtable)(const operation, const void *, void *, iterator *){};
    any storage{};
};

/*! @brief Proxy object for associative containers. */
class meta_associative_container {
    class meta_iterator;
    using operation = internal::meta_container_operation;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Meta iterator type. */
    using iterator = meta_iterator;

    /*! @brief Default constructor. */
    meta_associative_container() noexcept
        : meta_associative_container{locator<meta_ctx>::value_or()} {}

    /**
     * @brief Context aware constructor.
     * @param area The context from which to search for meta types.
     */
    meta_associative_container(const meta_ctx &area) noexcept
        : ctx{&area} {}

    /**
     * @brief Rebinds a proxy object to an associative container type.
     * @tparam Type Type of container to wrap.
     * @param instance The container to wrap.
     */
    template<typename Type>
    void rebind(any instance) noexcept {
        ENTT_ASSERT(instance.type() == type_id<Type>(), "Unexpected type");
        key_type_node = &internal::resolve<typename Type::key_type>;
        value_type_node = &internal::resolve<typename Type::value_type>;

        if constexpr(!meta_associative_container_traits<Type>::key_only) {
            mapped_type_node = &internal::resolve<typename Type::mapped_type>;
        }

        vtable = meta_associative_container_traits<Type>::basic_vtable;
        storage = std::move(instance);
    }

    [[nodiscard]] inline meta_type key_type() const noexcept;
    [[nodiscard]] inline meta_type mapped_type() const noexcept;
    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool clear();
    inline bool reserve(const size_type);
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline bool insert(meta_any, meta_any);
    inline size_type erase(meta_any);
    [[nodiscard]] inline iterator find(meta_any);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    const meta_ctx *ctx{};
    internal::meta_type_node (*key_type_node)(const internal::meta_context &){};
    internal::meta_type_node (*mapped_type_node)(const internal::meta_context &){};
    internal::meta_type_node (*value_type_node)(const internal::meta_context &){};
    size_type (*vtable)(const operation, const void *, void *, meta_any *, iterator *){};
    any storage{};
};

/*! @brief Possible modes of a meta any object. */
using meta_any_policy = any_policy;

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
                        static_cast<meta_any *>(other)->emplace<Type>(any_cast<Type>(value));
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
                    static_cast<meta_sequence_container *>(other)->rebind<Type>(std::move(const_cast<any &>(value)));
                }
                break;
            case operation::assoc:
                if constexpr(is_complete_v<meta_associative_container_traits<Type>>) {
                    static_cast<meta_associative_container *>(other)->rebind<Type>(std::move(const_cast<any &>(value)));
                }
                break;
            }
        }
    }

    void release() {
        if(node.dtor.dtor && (storage.policy() == any_policy::owner)) {
            node.dtor.dtor(storage.data());
        }
    }

    meta_any(const meta_ctx &area, const meta_any &other, any ref) noexcept
        : storage{std::move(ref)},
          ctx{&area},
          node{storage ? other.node : internal::meta_type_node{}},
          vtable{storage ? other.vtable : &basic_vtable<void>} {}

public:
    /*! Default constructor. */
    meta_any() noexcept
        : meta_any{meta_ctx_arg, locator<meta_ctx>::value_or()} {}

    /**
     * @brief Context aware constructor.
     * @param area The context from which to search for meta types.
     */
    meta_any(meta_ctx_arg_t, const meta_ctx &area) noexcept
        : storage{},
          ctx{&area},
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
        : meta_any{locator<meta_ctx>::value_or(), std::in_place_type<Type>, std::forward<Args>(args)...} {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param area The context from which to search for meta types.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit meta_any(const meta_ctx &area, std::in_place_type_t<Type>, Args &&...args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          ctx{&area},
          node{internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(internal::meta_context::from(*ctx))},
          vtable{&basic_vtable<std::remove_cv_t<std::remove_reference_t<Type>>>} {}

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{locator<meta_ctx>::value_or(), std::forward<Type>(value)} {}

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param area The context from which to search for meta types.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(const meta_ctx &area, Type &&value)
        : meta_any{area, std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)} {}

    /**
     * @brief Context aware copy constructor.
     * @param area The context from which to search for meta types.
     * @param other The instance to copy from.
     */
    meta_any(const meta_ctx &area, const meta_any &other)
        : meta_any{other} {
        ctx = &area;
        node = node.resolve ? node.resolve(internal::meta_context::from(*ctx)) : node;
    }

    /**
     * @brief Context aware move constructor.
     * @param area The context from which to search for meta types.
     * @param other The instance to move from.
     */
    meta_any(const meta_ctx &area, meta_any &&other)
        : meta_any{std::move(other)} {
        ctx = &area;
        node = node.resolve ? node.resolve(internal::meta_context::from(*ctx)) : node;
    }

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
          ctx{other.ctx},
          node{std::exchange(other.node, internal::meta_type_node{})},
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
        storage = other.storage;
        ctx = other.ctx;
        node = other.node;
        vtable = other.vtable;
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This meta any object.
     */
    meta_any &operator=(meta_any &&other) noexcept {
        release();
        storage = std::move(other.storage);
        ctx = other.ctx;
        node = std::exchange(other.node, internal::meta_type_node{});
        vtable = std::exchange(other.vtable, &basic_vtable<void>);
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
        const auto other = internal::resolve<std::remove_cv_t<Type>>(internal::meta_context::from(*ctx));
        return static_cast<const Type *>(internal::try_cast(internal::meta_context::from(*ctx), node, other, data()));
    }

    /*! @copydoc try_cast */
    template<typename Type>
    [[nodiscard]] Type *try_cast() {
        if constexpr(std::is_const_v<Type>) {
            return std::as_const(*this).try_cast<std::remove_const_t<Type>>();
        } else {
            const auto other = internal::resolve<std::remove_cv_t<Type>>(internal::meta_context::from(*ctx));
            return static_cast<Type *>(const_cast<void *>(internal::try_cast(internal::meta_context::from(*ctx), node, other, data())));
        }
    }

    /**
     * @brief Tries to cast an instance to a given type.
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
            if((other.storage.policy() == any_policy::owner)) {
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
        if constexpr(std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>) {
            return meta_any{meta_ctx_arg, *ctx};
        } else {
            auto other = internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(internal::meta_context::from(*ctx));
            return allow_cast(meta_type{*ctx, other});
        }
    }

    /**
     * @brief Converts an object in such a way that a given cast becomes viable.
     * @tparam Type Type to which the cast is requested.
     * @return True if there exists a viable conversion, false otherwise.
     */
    template<typename Type>
    bool allow_cast() {
        auto other = internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(internal::meta_context::from(*ctx));
        return allow_cast(meta_type{*ctx, other}) && (!(std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>) || storage.data() != nullptr);
    }

    /*! @copydoc any::emplace */
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        release();
        storage.emplace<Type>(std::forward<Args>(args)...);
        node = internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(internal::meta_context::from(*ctx));
        vtable = &basic_vtable<std::remove_cv_t<std::remove_reference_t<Type>>>;
    }

    /*! @copydoc any::assign */
    bool assign(const meta_any &other);

    /*! @copydoc any::assign */
    bool assign(meta_any &&other);

    /*! @copydoc any::reset */
    void reset() {
        release();
        storage.reset();
        node = {};
        vtable = &basic_vtable<void>;
    }

    /**
     * @brief Returns a sequence container proxy.
     * @return A sequence container proxy for the underlying object.
     */
    [[nodiscard]] meta_sequence_container as_sequence_container() noexcept {
        any detached = storage.as_ref();
        meta_sequence_container proxy{*ctx};
        vtable(operation::seq, detached, &proxy);
        return proxy;
    }

    /*! @copydoc as_sequence_container */
    [[nodiscard]] meta_sequence_container as_sequence_container() const noexcept {
        any detached = storage.as_ref();
        meta_sequence_container proxy{*ctx};
        vtable(operation::seq, detached, &proxy);
        return proxy;
    }

    /**
     * @brief Returns an associative container proxy.
     * @return An associative container proxy for the underlying object.
     */
    [[nodiscard]] meta_associative_container as_associative_container() noexcept {
        any detached = storage.as_ref();
        meta_associative_container proxy{*ctx};
        vtable(operation::assoc, detached, &proxy);
        return proxy;
    }

    /*! @copydoc as_associative_container */
    [[nodiscard]] meta_associative_container as_associative_container() const noexcept {
        any detached = storage.as_ref();
        meta_associative_container proxy{*ctx};
        vtable(operation::assoc, detached, &proxy);
        return proxy;
    }

    /**
     * @brief Indirection operator for dereferencing opaque objects.
     * @return A wrapper that shares a reference to an unmanaged object if the
     * wrapped element is dereferenceable, an invalid meta any otherwise.
     */
    [[nodiscard]] meta_any operator*() const noexcept {
        meta_any ret{meta_ctx_arg, *ctx};
        vtable(operation::deref, storage, &ret);
        return ret;
    }

    /**
     * @brief Returns false if a wrapper is invalid, true otherwise.
     * @return False if the wrapper is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(node.info == nullptr);
    }

    /*! @copydoc any::operator== */
    [[nodiscard]] bool operator==(const meta_any &other) const noexcept {
        return (ctx == other.ctx) && ((!node.info && !other.node.info) || (node.info && other.node.info && *node.info == *other.node.info && storage == other.storage));
    }

    /*! @copydoc any::operator!= */
    [[nodiscard]] bool operator!=(const meta_any &other) const noexcept {
        return !(*this == other);
    }

    /*! @copydoc any::as_ref */
    [[nodiscard]] meta_any as_ref() noexcept {
        return meta_any{*ctx, *this, storage.as_ref()};
    }

    /*! @copydoc any::as_ref */
    [[nodiscard]] meta_any as_ref() const noexcept {
        return meta_any{*ctx, *this, storage.as_ref()};
    }

    /*! @copydoc any::owner */
    [[deprecated("use policy() and meta_any_policy instead")]] [[nodiscard]] bool owner() const noexcept {
        return (storage.policy() == any_policy::owner);
    }

    /**
     * @brief Returns the current mode of a meta any object.
     * @return The current mode of the meta any object.
     */
    [[nodiscard]] meta_any_policy policy() const noexcept {
        return storage.policy();
    }

private:
    any storage;
    const meta_ctx *ctx;
    internal::meta_type_node node;
    vtable_type *vtable;
};

/**
 * @brief Forwards its argument and avoids copies for lvalue references.
 * @tparam Type Type of argument to use to construct the new instance.
 * @param value Parameter to use to construct the instance.
 * @param ctx The context from which to search for meta types.
 * @return A properly initialized and not necessarily owning wrapper.
 */
template<typename Type>
[[nodiscard]] meta_any forward_as_meta(const meta_ctx &ctx, Type &&value) {
    return meta_any{ctx, std::in_place_type<Type &&>, std::forward<Type>(value)};
}

/**
 * @brief Forwards its argument and avoids copies for lvalue references.
 * @tparam Type Type of argument to use to construct the new instance.
 * @param value Parameter to use to construct the instance.
 * @return A properly initialized and not necessarily owning wrapper.
 */
template<typename Type>
[[nodiscard]] meta_any forward_as_meta(Type &&value) {
    return forward_as_meta(locator<meta_ctx>::value_or(), std::forward<Type>(value));
}

/**
 * @brief Opaque pointers to instances of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance.
 */
struct meta_handle {
    /*! Default constructor. */
    meta_handle() noexcept
        : meta_handle{meta_ctx_arg, locator<meta_ctx>::value_or()} {}

    /**
     * @brief Context aware constructor.
     * @param area The context from which to search for meta types.
     */
    meta_handle(meta_ctx_arg_t, const meta_ctx &area) noexcept
        : any{meta_ctx_arg, area} {}

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @param value An instance of an object to use to initialize the handle.
     */
    meta_handle(meta_any &value) noexcept
        : any{value.as_ref()} {}

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @param value An instance of an object to use to initialize the handle.
     */
    meta_handle(const meta_any &value) noexcept
        : any{value.as_ref()} {}

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @tparam Type Type of object to use to initialize the handle.
     * @param ctx The context from which to search for meta types.
     * @param value An instance of an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(const meta_ctx &ctx, Type &value) noexcept
        : any{ctx, std::in_place_type<Type &>, value} {}

    /**
     * @brief Creates a handle that points to an unmanaged object.
     * @tparam Type Type of object to use to initialize the handle.
     * @param value An instance of an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(Type &value) noexcept
        : meta_handle{locator<meta_ctx>::value_or(), value} {}

    /**
     * @brief Context aware copy constructor.
     * @param area The context from which to search for meta types.
     * @param other The instance to copy from.
     */
    meta_handle(const meta_ctx &area, const meta_handle &other)
        : any{area, other.any} {}

    /**
     * @brief Context aware move constructor.
     * @param area The context from which to search for meta types.
     * @param other The instance to move from.
     */
    meta_handle(const meta_ctx &area, meta_handle &&other)
        : any{area, std::move(other.any)} {}

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
     * @brief Returns false if a handle is invalid, true otherwise.
     * @return False if the handle is invalid, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(any);
    }

    /*! @copydoc meta_any::operator== */
    [[nodiscard]] bool operator==(const meta_handle &other) const noexcept {
        return (any == other.any);
    }

    /*! @copydoc meta_any::operator!= */
    [[nodiscard]] bool operator!=(const meta_handle &other) const noexcept {
        return !(*this == other);
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
    /*! @brief Default constructor. */
    meta_prop() noexcept
        : node{},
          ctx{} {}

    /**
     * @brief Context aware constructor for meta objects.
     * @param area The context from which to search for meta types.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_prop(const meta_ctx &area, const internal::meta_prop_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

    /**
     * @brief Returns the stored value by const reference.
     * @return A wrapper containing the value stored with the property.
     */
    [[nodiscard]] meta_any value() const {
        return node->value ? node->type(internal::meta_context::from(*ctx)).from_void(*ctx, nullptr, node->value.get()) : meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @brief Returns the stored value by reference.
     * @return A wrapper containing the value stored with the property.
     */
    [[nodiscard]] meta_any value() {
        return node->value ? node->type(internal::meta_context::from(*ctx)).from_void(*ctx, node->value.get(), nullptr) : meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    /**
     * @brief Checks if two objects refer to the same type.
     * @param other The object with which to compare.
     * @return True if the objects refer to the same type, false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_prop &other) const noexcept {
        return (ctx == other.ctx && node == other.node);
    }

private:
    const internal::meta_prop_node *node;
    const meta_ctx *ctx;
};

/**
 * @brief Checks if two objects refer to the same type.
 * @param lhs An object, either valid or not.
 * @param rhs An object, either valid or not.
 * @return False if the objects refer to the same node, true otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_prop &lhs, const meta_prop &rhs) noexcept {
    return !(lhs == rhs);
}

/*! @brief Opaque wrapper for data members. */
struct meta_data {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_data_node::size_type;

    /*! @brief Default constructor. */
    meta_data() noexcept
        : node{},
          ctx{} {}

    /**
     * @brief Context aware constructor for meta objects.
     * @param area The context from which to search for meta types.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_data(const meta_ctx &area, const internal::meta_data_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

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
     * @tparam Type Type of value to assign.
     * @param instance An opaque instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     * @return True in case of success, false otherwise.
     */
    template<typename Type>
    bool set(meta_handle instance, Type &&value) const {
        return node->set && node->set(meta_handle{*ctx, std::move(instance)}, meta_any{*ctx, std::forward<Type>(value)});
    }

    /**
     * @brief Gets the value of a given variable.
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(meta_handle instance) const {
        return node->get(*ctx, meta_handle{*ctx, std::move(instance)});
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
    [[nodiscard]] meta_range<meta_prop, typename decltype(internal::meta_data_node::prop)::const_iterator> prop() const noexcept {
        return {{*ctx, node->prop.cbegin()}, {*ctx, node->prop.cend()}};
    }

    /**
     * @brief Lookup utility for meta properties.
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(const id_type key) const {
        const auto it = node->prop.find(key);
        return it != node->prop.cend() ? meta_prop{*ctx, it->second} : meta_prop{};
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    /*! @copydoc meta_prop::operator== */
    [[nodiscard]] bool operator==(const meta_data &other) const noexcept {
        return (ctx == other.ctx && node == other.node);
    }

private:
    const internal::meta_data_node *node;
    const meta_ctx *ctx;
};

/**
 * @brief Checks if two objects refer to the same type.
 * @param lhs An object, either valid or not.
 * @param rhs An object, either valid or not.
 * @return False if the objects refer to the same node, true otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_data &lhs, const meta_data &rhs) noexcept {
    return !(lhs == rhs);
}

/*! @brief Opaque wrapper for member functions. */
struct meta_func {
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

    /*! @brief Default constructor. */
    meta_func() noexcept
        : node{},
          ctx{} {}

    /**
     * @brief Context aware constructor for meta objects.
     * @param area The context from which to search for meta types.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_func(const meta_ctx &area, const internal::meta_func_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

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
     * @warning
     * The context of the arguments is **never** changed.
     *
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(meta_handle instance, meta_any *const args, const size_type sz) const {
        return sz == arity() ? node->invoke(*ctx, meta_handle{*ctx, std::move(instance)}, args) : meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @copybrief invoke
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle instance, Args &&...args) const {
        meta_any arguments[sizeof...(Args) + !sizeof...(Args)]{{*ctx, std::forward<Args>(args)}...};
        return invoke(std::move(instance), arguments, sizeof...(Args));
    }

    /*! @copydoc meta_data::prop */
    [[nodiscard]] meta_range<meta_prop, typename decltype(internal::meta_func_node::prop)::const_iterator> prop() const noexcept {
        return {{*ctx, node->prop.cbegin()}, {*ctx, node->prop.cend()}};
    }

    /**
     * @brief Lookup utility for meta properties.
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(const id_type key) const {
        const auto it = node->prop.find(key);
        return it != node->prop.cend() ? meta_prop{*ctx, it->second} : meta_prop{};
    }

    /**
     * @brief Returns the next overload of a given function, if any.
     * @return The next overload of the given function, if any.
     */
    [[nodiscard]] meta_func next() const {
        return node->next ? meta_func{*ctx, *node->next} : meta_func{};
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    /*! @copydoc meta_prop::operator== */
    [[nodiscard]] bool operator==(const meta_func &other) const noexcept {
        return (ctx == other.ctx && node == other.node);
    }

private:
    const internal::meta_func_node *node;
    const meta_ctx *ctx;
};

/**
 * @brief Checks if two objects refer to the same type.
 * @param lhs An object, either valid or not.
 * @param rhs An object, either valid or not.
 * @return False if the objects refer to the same node, true otherwise.
 */
[[nodiscard]] inline bool operator!=(const meta_func &lhs, const meta_func &rhs) noexcept {
    return !(lhs == rhs);
}

/*! @brief Opaque wrapper for types. */
class meta_type {
    friend class meta_any;

    template<typename Func>
    [[nodiscard]] auto lookup(meta_any *const args, const typename internal::meta_type_node::size_type sz, [[maybe_unused]] bool constness, Func next) const {
        decltype(next()) candidate = nullptr;
        size_type same{};
        bool ambiguous{};

        for(auto curr = next(); curr; curr = next()) {
            if constexpr(std::is_same_v<std::decay_t<decltype(*curr)>, internal::meta_func_node>) {
                if(constness && !static_cast<bool>(curr->traits & internal::meta_traits::is_const)) {
                    continue;
                }
            }

            if(curr->arity == sz) {
                size_type match{};
                size_type pos{};

                for(; pos < sz && args[pos]; ++pos) {
                    const auto other = curr->arg(*ctx, pos);
                    const auto type = args[pos].type();

                    if(const auto &info = other.info(); info == type.info()) {
                        ++match;
                    } else if(!((type.node.details && (type.node.details->base.contains(info.hash()) || type.node.details->conv.contains(info.hash()))) || (type.node.conversion_helper && other.node.conversion_helper))) {
                        break;
                    }
                }

                if(pos == sz) {
                    if(!candidate || match > same) {
                        candidate = curr;
                        same = match;
                        ambiguous = false;
                    } else if(match == same) {
                        if constexpr(std::is_same_v<std::decay_t<decltype(*curr)>, internal::meta_func_node>) {
                            if(static_cast<bool>(curr->traits & internal::meta_traits::is_const) != static_cast<bool>(candidate->traits & internal::meta_traits::is_const)) {
                                candidate = static_cast<bool>(candidate->traits & internal::meta_traits::is_const) ? curr : candidate;
                                ambiguous = false;
                                continue;
                            }
                        }

                        ambiguous = true;
                    }
                }
            }
        }

        return ambiguous ? nullptr : candidate;
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_type_node::size_type;

    /*! @brief Default constructor. */
    meta_type() noexcept
        : node{},
          ctx{} {}

    /**
     * @brief Context aware constructor for meta objects.
     * @param area The context from which to search for meta types.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_type(const meta_ctx &area, const internal::meta_type_node &curr) noexcept
        : node{curr},
          ctx{&area} {}

    /**
     * @brief Context aware constructor for meta objects.
     * @param area The context from which to search for meta types.
     * @param curr The underlying node with which to construct the instance.
     */
    meta_type(const meta_ctx &area, const internal::meta_base_node &curr) noexcept
        : meta_type{area, curr.type(internal::meta_context::from(area))} {}

    /**
     * @brief Returns the type info object of the underlying type.
     * @return The type info object of the underlying type.
     */
    [[nodiscard]] const type_info &info() const noexcept {
        return *node.info;
    }

    /**
     * @brief Returns the identifier assigned to a type.
     * @return The identifier assigned to the type.
     */
    [[nodiscard]] id_type id() const noexcept {
        return node.id;
    }

    /**
     * @brief Returns the size of the underlying type if known.
     * @return The size of the underlying type if known, 0 otherwise.
     */
    [[nodiscard]] size_type size_of() const noexcept {
        return node.size_of;
    }

    /**
     * @brief Checks whether a type refers to an arithmetic type or not.
     * @return True if the underlying type is an arithmetic type, false
     * otherwise.
     */
    [[nodiscard]] bool is_arithmetic() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_arithmetic);
    }

    /**
     * @brief Checks whether a type refers to an integral type or not.
     * @return True if the underlying type is an integral type, false otherwise.
     */
    [[nodiscard]] bool is_integral() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_integral);
    }

    /**
     * @brief Checks whether a type refers to a signed type or not.
     * @return True if the underlying type is a signed type, false otherwise.
     */
    [[nodiscard]] bool is_signed() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_signed);
    }

    /**
     * @brief Checks whether a type refers to an array type or not.
     * @return True if the underlying type is an array type, false otherwise.
     */
    [[nodiscard]] bool is_array() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_array);
    }

    /**
     * @brief Checks whether a type refers to an enum or not.
     * @return True if the underlying type is an enum, false otherwise.
     */
    [[nodiscard]] bool is_enum() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_enum);
    }

    /**
     * @brief Checks whether a type refers to a class or not.
     * @return True if the underlying type is a class, false otherwise.
     */
    [[nodiscard]] bool is_class() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_class);
    }

    /**
     * @brief Checks whether a type refers to a pointer or not.
     * @return True if the underlying type is a pointer, false otherwise.
     */
    [[nodiscard]] bool is_pointer() const noexcept {
        return node.info && (node.info->hash() != remove_pointer().info().hash());
    }

    /**
     * @brief Provides the type for which the pointer is defined.
     * @return The type for which the pointer is defined or this type if it
     * doesn't refer to a pointer type.
     */
    [[nodiscard]] meta_type remove_pointer() const noexcept {
        return {*ctx, node.remove_pointer(internal::meta_context::from(*ctx))};
    }

    /**
     * @brief Checks whether a type is a pointer-like type or not.
     * @return True if the underlying type is pointer-like, false otherwise.
     */
    [[nodiscard]] bool is_pointer_like() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_meta_pointer_like);
    }

    /**
     * @brief Checks whether a type refers to a sequence container or not.
     * @return True if the type is a sequence container, false otherwise.
     */
    [[nodiscard]] bool is_sequence_container() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_meta_sequence_container);
    }

    /**
     * @brief Checks whether a type refers to an associative container or not.
     * @return True if the type is an associative container, false otherwise.
     */
    [[nodiscard]] bool is_associative_container() const noexcept {
        return static_cast<bool>(node.traits & internal::meta_traits::is_meta_associative_container);
    }

    /**
     * @brief Checks whether a type refers to a recognized class template
     * specialization or not.
     * @return True if the type is a recognized class template specialization,
     * false otherwise.
     */
    [[nodiscard]] bool is_template_specialization() const noexcept {
        return (node.templ.arity != 0u);
    }

    /**
     * @brief Returns the number of template arguments.
     * @return The number of template arguments.
     */
    [[nodiscard]] size_type template_arity() const noexcept {
        return node.templ.arity;
    }

    /**
     * @brief Returns a tag for the class template of the underlying type.
     * @return The tag for the class template of the underlying type.
     */
    [[nodiscard]] inline meta_type template_type() const noexcept {
        return node.templ.type ? meta_type{*ctx, node.templ.type(internal::meta_context::from(*ctx))} : meta_type{};
    }

    /**
     * @brief Returns the type of the i-th template argument of a type.
     * @param index Index of the template argument of which to return the type.
     * @return The type of the i-th template argument of a type.
     */
    [[nodiscard]] inline meta_type template_arg(const size_type index) const noexcept {
        return index < template_arity() ? meta_type{*ctx, node.templ.arg(internal::meta_context::from(*ctx), index)} : meta_type{};
    }

    /**
     * @brief Checks if a type supports direct casting to another type.
     * @param other The meta type to test for.
     * @return True if direct casting is allowed, false otherwise.
     */
    [[nodiscard]] bool can_cast(const meta_type &other) const noexcept {
        // casting this is UB in all cases but we aren't going to use the resulting pointer, so...
        return internal::try_cast(internal::meta_context::from(*ctx), node, other.node, this);
    }

    /**
     * @brief Checks if a type supports conversion it to another type.
     * @param other The meta type to test for.
     * @return True if the conversion is allowed, false otherwise.
     */
    [[nodiscard]] bool can_convert(const meta_type &other) const noexcept {
        return internal::try_convert(internal::meta_context::from(*ctx), node, other.node, nullptr, [](const void *, auto &&...args) { return ((args, 1) + ... + 0u); });
    }

    /**
     * @brief Returns a range to visit registered top-level base meta types.
     * @return An iterable range to visit registered top-level base meta types.
     */
    [[nodiscard]] meta_range<meta_type, typename decltype(internal::meta_type_descriptor::base)::const_iterator> base() const noexcept {
        using range_type = meta_range<meta_type, typename decltype(internal::meta_type_descriptor::base)::const_iterator>;
        return node.details ? range_type{{*ctx, node.details->base.cbegin()}, {*ctx, node.details->base.cend()}} : range_type{};
    }

    /**
     * @brief Returns a range to visit registered top-level meta data.
     * @return An iterable range to visit registered top-level meta data.
     */
    [[nodiscard]] meta_range<meta_data, typename decltype(internal::meta_type_descriptor::data)::const_iterator> data() const noexcept {
        using range_type = meta_range<meta_data, typename decltype(internal::meta_type_descriptor::data)::const_iterator>;
        return node.details ? range_type{{*ctx, node.details->data.cbegin()}, {*ctx, node.details->data.cend()}} : range_type{};
    }

    /**
     * @brief Lookup utility for meta data (bases are also visited).
     * @param id Unique identifier.
     * @return The registered meta data for the given identifier, if any.
     */
    [[nodiscard]] meta_data data(const id_type id) const {
        if(node.details) {
            if(const auto it = node.details->data.find(id); it != node.details->data.cend()) {
                return meta_data{*ctx, it->second};
            }
        }

        for(auto &&curr: base()) {
            if(auto elem = curr.second.data(id); elem) {
                return elem;
            }
        }

        return meta_data{};
    }

    /**
     * @brief Returns a range to visit registered top-level functions.
     * @return An iterable range to visit registered top-level functions.
     */
    [[nodiscard]] meta_range<meta_func, typename decltype(internal::meta_type_descriptor::func)::const_iterator> func() const noexcept {
        using return_type = meta_range<meta_func, typename decltype(internal::meta_type_descriptor::func)::const_iterator>;
        return node.details ? return_type{{*ctx, node.details->func.cbegin()}, {*ctx, node.details->func.cend()}} : return_type{};
    }

    /**
     * @brief Lookup utility for meta functions (bases are also visited).
     *
     * In case of overloaded functions, a random one is returned.
     *
     * @param id Unique identifier.
     * @return The registered meta function for the given identifier, if any.
     */
    [[nodiscard]] meta_func func(const id_type id) const {
        if(node.details) {
            if(const auto it = node.details->func.find(id); it != node.details->func.cend()) {
                return meta_func{*ctx, it->second};
            }
        }

        for(auto &&curr: base()) {
            if(auto elem = curr.second.func(id); elem) {
                return elem;
            }
        }

        return meta_func{};
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * @warning
     * The context of the arguments is **never** changed.
     *
     * @param args Parameters to use to construct the instance.
     * @param sz Number of parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    [[nodiscard]] meta_any construct(meta_any *const args, const size_type sz) const {
        if(node.details) {
            if(const auto *candidate = lookup(args, sz, false, [first = node.details->ctor.cbegin(), last = node.details->ctor.cend()]() mutable { return first == last ? nullptr : &(first++)->second; }); candidate) {
                return candidate->invoke(*ctx, args);
            }
        }

        if(sz == 0u && node.default_constructor) {
            return node.default_constructor(*ctx);
        }

        return meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @copybrief construct
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A wrapper containing the new instance, if any.
     */
    template<typename... Args>
    [[nodiscard]] meta_any construct(Args &&...args) const {
        meta_any arguments[sizeof...(Args) + !sizeof...(Args)]{{*ctx, std::forward<Args>(args)}...};
        return construct(arguments, sizeof...(Args));
    }

    /**
     * @brief Wraps an opaque element of the underlying type.
     * @param element A valid pointer to an element of the underlying type.
     * @return A wrapper that references the given instance.
     */
    [[nodiscard]] meta_any from_void(void *element) const {
        return (element && node.from_void) ? node.from_void(*ctx, element, nullptr) : meta_any{meta_ctx_arg, *ctx};
    }

    /*! @copydoc from_void */
    [[nodiscard]] meta_any from_void(const void *element) const {
        return (element && node.from_void) ? node.from_void(*ctx, nullptr, element) : meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @brief Invokes a function given an identifier, if possible.
     *
     * @warning
     * The context of the arguments is **never** changed.
     *
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @param sz Number of parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    meta_any invoke(const id_type id, meta_handle instance, meta_any *const args, const size_type sz) const {
        if(node.details) {
            if(auto it = node.details->func.find(id); it != node.details->func.cend()) {
                if(const auto *candidate = lookup(args, sz, instance && (instance->data() == nullptr), [curr = &it->second]() mutable { return curr ? std::exchange(curr, curr->next.get()) : nullptr; }); candidate) {
                    return candidate->invoke(*ctx, meta_handle{*ctx, std::move(instance)}, args);
                }
            }
        }

        for(auto &&curr: base()) {
            if(auto elem = curr.second.invoke(id, *instance.operator->(), args, sz); elem) {
                return elem;
            }
        }

        return meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @copybrief invoke
     * @param id Unique identifier.
     * @tparam Args Types of arguments to use to invoke the function.
     * @param instance An opaque instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A wrapper containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(const id_type id, meta_handle instance, Args &&...args) const {
        meta_any arguments[sizeof...(Args) + !sizeof...(Args)]{{*ctx, std::forward<Args>(args)}...};
        return invoke(id, std::move(instance), arguments, sizeof...(Args));
    }

    /**
     * @brief Sets the value of a given variable.
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
     * @param id Unique identifier.
     * @param instance An opaque instance of the underlying type.
     * @return A wrapper containing the value of the underlying variable.
     */
    [[nodiscard]] meta_any get(const id_type id, meta_handle instance) const {
        const auto candidate = data(id);
        return candidate ? candidate.get(std::move(instance)) : meta_any{meta_ctx_arg, *ctx};
    }

    /**
     * @brief Returns a range to visit registered top-level meta properties.
     * @return An iterable range to visit registered top-level meta properties.
     */
    [[nodiscard]] meta_range<meta_prop, typename decltype(internal::meta_type_descriptor::prop)::const_iterator> prop() const noexcept {
        using range_type = meta_range<meta_prop, typename decltype(internal::meta_type_descriptor::prop)::const_iterator>;
        return node.details ? range_type{{*ctx, node.details->prop.cbegin()}, {*ctx, node.details->prop.cend()}} : range_type{};
    }

    /**
     * @brief Lookup utility for meta properties (bases are also visited).
     * @param key The key to use to search for a property.
     * @return The registered meta property for the given key, if any.
     */
    [[nodiscard]] meta_prop prop(const id_type key) const {
        if(node.details) {
            if(const auto it = node.details->prop.find(key); it != node.details->prop.cend()) {
                return meta_prop{*ctx, it->second};
            }
        }

        for(auto &&curr: base()) {
            if(auto elem = curr.second.prop(key); elem) {
                return elem;
            }
        }

        return meta_prop{};
    }

    /**
     * @brief Returns true if an object is valid, false otherwise.
     * @return True if the object is valid, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(ctx == nullptr);
    }

    /*! @copydoc meta_prop::operator== */
    [[nodiscard]] bool operator==(const meta_type &other) const noexcept {
        return (ctx == other.ctx) && ((!node.info && !other.node.info) || (node.info && other.node.info && *node.info == *other.node.info));
    }

private:
    internal::meta_type_node node;
    const meta_ctx *ctx;
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
    return node.info ? meta_type{*ctx, node} : meta_type{};
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
    return internal::try_convert(internal::meta_context::from(*ctx), node, type.node, data(), [this, &type](const void *instance, auto &&...args) {
        if constexpr((std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(args)>>, internal::meta_type_node> || ...)) {
            return (args.from_void(*ctx, nullptr, instance), ...);
        } else if constexpr((std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(args)>>, internal::meta_conv_node> || ...)) {
            return (args.conv(*ctx, instance), ...);
        } else if constexpr((std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(args)>>, decltype(internal::meta_type_node::conversion_helper)> || ...)) {
            // exploits the fact that arithmetic types and enums are also default constructible
            auto other = type.construct();
            const auto value = (args(nullptr, instance), ...);
            other.node.conversion_helper(other.data(), &value);
            return other;
        } else {
            // forwards to force a compile-time error in case of available arguments
            return meta_any{meta_ctx_arg, *ctx, std::forward<decltype(args)>(args)...};
        }
    });
}

inline bool meta_any::assign(const meta_any &other) {
    auto value = other.allow_cast({*ctx, node});
    return value && storage.assign(value.storage);
}

inline bool meta_any::assign(meta_any &&other) {
    if(*node.info == *other.node.info) {
        return storage.assign(std::move(other.storage));
    }

    return assign(std::as_const(other));
}

[[nodiscard]] inline meta_type meta_data::type() const noexcept {
    return meta_type{*ctx, node->type(internal::meta_context::from(*ctx))};
}

[[nodiscard]] inline meta_type meta_data::arg(const size_type index) const noexcept {
    return index < arity() ? node->arg(*ctx, index) : meta_type{};
}

[[nodiscard]] inline meta_type meta_func::ret() const noexcept {
    return meta_type{*ctx, node->ret(internal::meta_context::from(*ctx))};
}

[[nodiscard]] inline meta_type meta_func::arg(const size_type index) const noexcept {
    return index < arity() ? node->arg(*ctx, index) : meta_type{};
}

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

class meta_sequence_container::meta_iterator final {
    using vtable_type = void(const void *, const std::ptrdiff_t, meta_any *);

    template<typename It>
    static void basic_vtable(const void *value, const std::ptrdiff_t offset, meta_any *other) {
        if(const auto &it = *static_cast<const It *>(value); other) {
            other->emplace<decltype(*it)>(*it);
        } else {
            std::advance(const_cast<It &>(it), offset);
        }
    }

public:
    using difference_type = std::ptrdiff_t;
    using value_type = meta_any;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    meta_iterator() noexcept
        : meta_iterator{locator<meta_ctx>::value_or()} {}

    meta_iterator(const meta_ctx &area) noexcept
        : ctx{&area} {}

    template<typename It>
    void rebind(It iter) noexcept {
        vtable = &basic_vtable<It>;
        handle = iter;
    }

    meta_iterator &operator++() noexcept {
        vtable(handle.data(), 1, nullptr);
        return *this;
    }

    meta_iterator operator++(int value) noexcept {
        meta_iterator orig = *this;
        vtable(handle.data(), ++value, nullptr);
        return orig;
    }

    meta_iterator &operator--() noexcept {
        vtable(handle.data(), -1, nullptr);
        return *this;
    }

    meta_iterator operator--(int value) noexcept {
        meta_iterator orig = *this;
        vtable(handle.data(), --value, nullptr);
        return orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other{meta_ctx_arg, *ctx};
        vtable(handle.data(), 0, &other);
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

    [[nodiscard]] const any &base() const noexcept {
        return handle;
    }

private:
    const meta_ctx *ctx{};
    vtable_type *vtable{};
    any handle{};
};

class meta_associative_container::meta_iterator final {
    using vtable_type = void(const void *, std::pair<meta_any, meta_any> *);

    template<bool KeyOnly, typename It>
    static void basic_vtable(const void *value, std::pair<meta_any, meta_any> *other) {
        if(const auto &it = *static_cast<const It *>(value); other) {
            if constexpr(KeyOnly) {
                other->first.emplace<decltype(*it)>(*it);
            } else {
                other->first.emplace<decltype((it->first))>(it->first);
                other->second.emplace<decltype((it->second))>(it->second);
            }
        } else {
            ++const_cast<It &>(it);
        }
    }

public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<meta_any, meta_any>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    meta_iterator() noexcept
        : meta_iterator{locator<meta_ctx>::value_or()} {}

    meta_iterator(const meta_ctx &area) noexcept
        : ctx{&area} {}

    template<bool KeyOnly, typename It>
    void rebind(It iter) noexcept {
        vtable = &basic_vtable<KeyOnly, It>;
        handle = iter;
    }

    meta_iterator &operator++() noexcept {
        vtable(handle.data(), nullptr);
        return *this;
    }

    meta_iterator operator++(int) noexcept {
        meta_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other{{meta_ctx_arg, *ctx}, {meta_ctx_arg, *ctx}};
        vtable(handle.data(), &other);
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
    const meta_ctx *ctx{};
    vtable_type *vtable{};
    any handle{};
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
    return value_type_node ? meta_type{*ctx, value_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}

/**
 * @brief Returns the size of a container.
 * @return The size of the container.
 */
[[nodiscard]] inline meta_sequence_container::size_type meta_sequence_container::size() const noexcept {
    return vtable(operation::size, std::as_const(storage).data(), nullptr, nullptr);
}

/**
 * @brief Resizes a container to contain a given number of elements.
 * @param sz The new size of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::resize(const size_type sz) {
    return (storage.policy() != any_policy::cref) && vtable(operation::resize, &sz, storage.data(), nullptr);
}

/**
 * @brief Clears the content of a container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::clear() {
    return (storage.policy() != any_policy::cref) && vtable(operation::clear, nullptr, storage.data(), nullptr);
}

/**
 * @brief Reserves storage for at least the given number of elements.
 * @param sz The new capacity of the container.
 * @return True in case of success, false otherwise.
 */
inline bool meta_sequence_container::reserve(const size_type sz) {
    return (storage.policy() != any_policy::cref) && vtable(operation::reserve, &sz, storage.data(), nullptr);
}

/**
 * @brief Returns an iterator to the first element of a container.
 * @return An iterator to the first element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    iterator it{*ctx};
    const void *data = std::as_const(storage).data();
    vtable(operation::begin, data, storage.policy() == any_policy::cref ? nullptr : const_cast<void *>(data), &it);
    return it;
}

/**
 * @brief Returns an iterator that is past the last element of a container.
 * @return An iterator that is past the last element of the container.
 */
[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    iterator it{*ctx};
    const void *data = std::as_const(storage).data();
    vtable(operation::end, data, storage.policy() == any_policy::cref ? nullptr : const_cast<void *>(data), &it);
    return it;
}

/**
 * @brief Inserts an element at a specified location of a container.
 * @param it Iterator before which the element will be inserted.
 * @param value Element value to insert.
 * @return A possibly invalid iterator to the inserted element.
 */
inline meta_sequence_container::iterator meta_sequence_container::insert(iterator it, meta_any value) {
    return ((storage.policy() != any_policy::cref) && vtable(operation::insert, &value, storage.data(), &it)) ? it : iterator{*ctx};
}

/**
 * @brief Removes a given element from a container.
 * @param it Iterator to the element to remove.
 * @return A possibly invalid iterator following the last removed element.
 */
inline meta_sequence_container::iterator meta_sequence_container::erase(iterator it) {
    return ((storage.policy() != any_policy::cref) && vtable(operation::erase, nullptr, storage.data(), &it)) ? it : iterator{*ctx};
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
 * @brief Returns the meta key type of a container.
 * @return The meta key type of the a container.
 */
[[nodiscard]] inline meta_type meta_associative_container::key_type() const noexcept {
    return key_type_node ? meta_type{*ctx, key_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}

/**
 * @brief Returns the meta mapped type of a container.
 * @return The meta mapped type of the a container.
 */
[[nodiscard]] inline meta_type meta_associative_container::mapped_type() const noexcept {
    return mapped_type_node ? meta_type{*ctx, mapped_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}

/*! @copydoc meta_sequence_container::value_type */
[[nodiscard]] inline meta_type meta_associative_container::value_type() const noexcept {
    return value_type_node ? meta_type{*ctx, value_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}

/*! @copydoc meta_sequence_container::size */
[[nodiscard]] inline meta_associative_container::size_type meta_associative_container::size() const noexcept {
    return vtable(operation::size, std::as_const(storage).data(), nullptr, nullptr, nullptr);
}

/*! @copydoc meta_sequence_container::clear */
inline bool meta_associative_container::clear() {
    return (storage.policy() != any_policy::cref) && vtable(operation::clear, nullptr, storage.data(), nullptr, nullptr);
}

/*! @copydoc meta_sequence_container::reserve */
inline bool meta_associative_container::reserve(const size_type sz) {
    return (storage.policy() != any_policy::cref) && vtable(operation::reserve, &sz, storage.data(), nullptr, nullptr);
}

/*! @copydoc meta_sequence_container::begin */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::begin() {
    iterator it{*ctx};
    const void *data = std::as_const(storage).data();
    vtable(operation::begin, data, storage.policy() == any_policy::cref ? nullptr : const_cast<void *>(data), nullptr, &it);
    return it;
}

/*! @copydoc meta_sequence_container::end */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::end() {
    iterator it{*ctx};
    const void *data = std::as_const(storage).data();
    vtable(operation::end, data, storage.policy() == any_policy::cref ? nullptr : const_cast<void *>(data), nullptr, &it);
    return it;
}

/**
 * @brief Inserts a key-only or key/value element into a container.
 * @param key The key of the element to insert.
 * @param value The value of the element to insert, if needed.
 * @return A bool denoting whether the insertion took place.
 */
inline bool meta_associative_container::insert(meta_any key, meta_any value = {}) {
    return ((storage.policy() != any_policy::cref) && vtable(operation::insert, &value, storage.data(), &key, nullptr));
}

/**
 * @brief Removes the specified element from a container.
 * @param key The key of the element to remove.
 * @return A bool denoting whether the removal took place.
 */
inline meta_associative_container::size_type meta_associative_container::erase(meta_any key) {
    return ((storage.policy() != any_policy::cref) && vtable(operation::erase, nullptr, storage.data(), &key, nullptr));
}

/**
 * @brief Returns an iterator to the element with a given key, if any.
 * @param key The key of the element to search.
 * @return An iterator to the element with the given key, if any.
 */
[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::find(meta_any key) {
    iterator it{*ctx};
    const void *data = std::as_const(storage).data();
    vtable(operation::find, data, storage.policy() == any_policy::cref ? nullptr : const_cast<void *>(data), &key, &it);
    return it;
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
