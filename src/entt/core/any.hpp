#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/utility.hpp"
#include "fwd.hpp"
#include "type_info.hpp"
#include "type_traits.hpp"

namespace entt {

/**
 * @brief A SBO friendly, type-safe container for single values of any type.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 */
template<std::size_t Len, std::size_t Align>
class basic_any {
    enum class operation : std::uint8_t {
        copy,
        move,
        transfer,
        assign,
        destroy,
        compare,
        hash,
        get
    };

    enum class policy : std::uint8_t {
        owner,
        ref,
        cref
    };

    using storage_type = std::aligned_storage_t<Len + !Len, Align>;
    using vtable_type = const void *(const operation, const basic_any &, const void *);

    template<typename Type>
    static constexpr bool in_situ = Len && alignof(Type) <= alignof(storage_type) && sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    static const void *basic_vtable([[maybe_unused]] const operation op, [[maybe_unused]] const basic_any &from, [[maybe_unused]] const void *to) {
        static_assert(!std::is_same_v<Type, void> && std::is_same_v<std::remove_reference_t<std::remove_const_t<Type>>, Type>, "Invalid type");
        const Type *instance = nullptr;

        if constexpr(in_situ<Type>) {
            instance = (from.mode == policy::owner) ? ENTT_LAUNDER(reinterpret_cast<const Type *>(&from.storage)) : static_cast<const Type *>(from.instance);
        } else {
            instance = static_cast<const Type *>(from.instance);
        }

        switch(op) {
        case operation::copy:
            if constexpr(std::is_copy_constructible_v<Type>) {
                static_cast<basic_any *>(const_cast<void *>(to))->initialize<Type>(*instance);
            }
            break;
        case operation::move:
            if constexpr(in_situ<Type>) {
                if(from.mode == policy::owner) {
                    return new(&static_cast<basic_any *>(const_cast<void *>(to))->storage) Type{std::move(*const_cast<Type *>(instance))};
                }
            }

            return (static_cast<basic_any *>(const_cast<void *>(to))->instance = std::exchange(const_cast<basic_any &>(from).instance, nullptr));
        case operation::transfer:
            if constexpr(std::is_move_assignable_v<Type>) {
                *const_cast<Type *>(instance) = std::move(*static_cast<Type *>(const_cast<void *>(to)));
                return to;
            }
            [[fallthrough]];
        case operation::assign:
            if constexpr(std::is_copy_assignable_v<Type>) {
                *const_cast<Type *>(instance) = *static_cast<const Type *>(to);
                return to;
            }
            break;
        case operation::destroy:
            if constexpr(in_situ<Type>) {
                instance->~Type();
            } else if constexpr(std::is_array_v<Type>) {
                delete[] instance;
            } else {
                delete instance;
            }
            break;
        case operation::compare:
            if constexpr(!std::is_function_v<Type> && !std::is_array_v<Type> && is_equality_comparable_v<Type>) {
                return *static_cast<const Type *>(instance) == *static_cast<const Type *>(to) ? to : nullptr;
            } else {
                return (instance == to) ? to : nullptr;
            }
        case operation::hash:
            if constexpr(is_std_hashable_v<Type>) {
                std::hash<Type> h;
                return reinterpret_cast<const void*>(h(*static_cast<const Type *>(instance)));
            } else {
                return 0;
            }
        case operation::get:
            return instance;
        }

        return nullptr;
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&...args) {
        if constexpr(!std::is_void_v<Type>) {
            vtable = basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>;
            info = &type_id<Type>();

            if constexpr(std::is_lvalue_reference_v<Type>) {
                static_assert(sizeof...(Args) == 1u && (std::is_lvalue_reference_v<Args> && ...), "Invalid arguments");
                mode = std::is_const_v<std::remove_reference_t<Type>> ? policy::cref : policy::ref;
                instance = (std::addressof(args), ...);
            } else if constexpr(in_situ<Type>) {
                if constexpr(sizeof...(Args) != 0u && std::is_aggregate_v<Type>) {
                    new(&storage) Type{std::forward<Args>(args)...};
                } else {
                    new(&storage) Type(std::forward<Args>(args)...);
                }
            } else {
                if constexpr(sizeof...(Args) != 0u && std::is_aggregate_v<Type>) {
                    instance = new Type{std::forward<Args>(args)...};
                } else {
                    instance = new Type(std::forward<Args>(args)...);
                }
            }
        }
    }

    basic_any(const basic_any &other, const policy pol) ENTT_NOEXCEPT
        : instance{other.data()},
          info{other.info},
          vtable{other.vtable},
          mode{pol} {}

public:
    /*! @brief Size of the internal storage. */
    static constexpr auto length = Len;
    /*! @brief Alignment requirement. */
    static constexpr auto alignment = Align;

    /*! @brief Default constructor. */
    basic_any() ENTT_NOEXCEPT
        : instance{},
          info{&type_id<void>()},
          vtable{},
          mode{policy::owner} {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, Args &&...args)
        : basic_any{} {
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : basic_any{} {
        initialize<std::decay_t<Type>>(std::forward<Type>(value));
    }

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    basic_any(const basic_any &other)
        : basic_any{} {
        if(other.vtable) {
            other.vtable(operation::copy, other, this);
        }
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_any(basic_any &&other) ENTT_NOEXCEPT
        : instance{},
          info{other.info},
          vtable{other.vtable},
          mode{other.mode} {
        if(other.vtable) {
            other.vtable(operation::move, other, this);
        }
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~basic_any() {
        if(vtable && mode == policy::owner) {
            vtable(operation::destroy, *this, nullptr);
        }
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This any object.
     */
    basic_any &operator=(const basic_any &other) {
        reset();

        if(other.vtable) {
            other.vtable(operation::copy, other, this);
        }

        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This any object.
     */
    basic_any &operator=(basic_any &&other) ENTT_NOEXCEPT {
        reset();

        if(other.vtable) {
            other.vtable(operation::move, other, this);
            info = other.info;
            vtable = other.vtable;
            mode = other.mode;
        }

        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This any object.
     */
    template<typename Type>
    std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>, basic_any &>
    operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    /**
     * @brief Returns the object type if any, `type_id<void>()` otherwise.
     * @return The object type if any, `type_id<void>()` otherwise.
     */
    [[nodiscard]] const type_info &type() const ENTT_NOEXCEPT {
        return *info;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data() const ENTT_NOEXCEPT {
        return vtable ? vtable(operation::get, *this, nullptr) : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data(const type_info &req) const ENTT_NOEXCEPT {
        return *info == req ? data() : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data() ENTT_NOEXCEPT {
        return (!vtable || mode == policy::cref) ? nullptr : const_cast<void *>(vtable(operation::get, *this, nullptr));
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data(const type_info &req) ENTT_NOEXCEPT {
        return *info == req ? data() : nullptr;
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        reset();
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Copy assigns a value to the contained object without replacing it.
     * @param other The value to assign to the contained object.
     * @return True in case of success, false otherwise.
     */
    bool assign(const any &other) {
        if(vtable && mode != policy::cref && *info == *other.info) {
            return (vtable(operation::assign, *this, other.data()) != nullptr);
        }

        return false;
    }

    /**
     * @brief Move assigns a value to the contained object without replacing it.
     * @param other The value to assign to the contained object.
     * @return True in case of success, false otherwise.
     */
    bool assign(any &&other) {
        if(vtable && mode != policy::cref && *info == *other.info) {
            if(auto *val = other.data(); val) {
                return (vtable(operation::transfer, *this, val) != nullptr);
            } else {
                return (vtable(operation::assign, *this, std::as_const(other).data()) != nullptr);
            }
        }

        return false;
    }

    /*! @brief Destroys contained object */
    void reset() {
        if(vtable && mode == policy::owner) {
            vtable(operation::destroy, *this, nullptr);
        }

        info = &type_id<void>();
        vtable = nullptr;
        mode = policy::owner;
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return vtable != nullptr;
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    bool operator==(const basic_any &other) const ENTT_NOEXCEPT {
        if(vtable && *info == *other.info) {
            return (vtable(operation::compare, *this, other.data()) != nullptr);
        }

        return (!vtable && !other.vtable);
    }

    /**
     * @brief Returns std::hash of the contained object, 0 if std::hash is not defined for contained object type
     * @return std::hash of the contained object, 0 if std::hash is not defined for contained object type
     */
    size_t hash() const ENTT_NOEXCEPT {
        return reinterpret_cast<uintptr_t>(vtable(operation::hash, *this, nullptr));
    }

    /**
     * @brief Aliasing constructor.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_any as_ref() ENTT_NOEXCEPT {
        return basic_any{*this, (mode == policy::cref ? policy::cref : policy::ref)};
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const ENTT_NOEXCEPT {
        return basic_any{*this, policy::cref};
    }

    /**
     * @brief Returns true if a wrapper owns its object, false otherwise.
     * @return True if the wrapper owns its object, false otherwise.
     */
    [[nodiscard]] bool owner() const ENTT_NOEXCEPT {
        return (mode == policy::owner);
    }

private:
    union {
        const void *instance;
        storage_type storage;
    };
    const type_info *info;
    vtable_type *vtable;
    policy mode;
};

/**
 * @brief Checks if two wrappers differ in their content.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Alignment requirement.
 * @param lhs A wrapper, either empty or not.
 * @param rhs A wrapper, either empty or not.
 * @return True if the two wrappers differ in their content, false otherwise.
 */
template<std::size_t Len, std::size_t Align>
[[nodiscard]] inline bool operator!=(const basic_any<Len, Align> &lhs, const basic_any<Len, Align> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

/**
 * @brief Performs type-safe access to the contained object.
 * @tparam Type Type to which conversion is required.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Alignment requirement.
 * @param data Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type, std::size_t Len, std::size_t Align>
Type any_cast(const basic_any<Len, Align> &data) ENTT_NOEXCEPT {
    const auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type any_cast(basic_any<Len, Align> &data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto *const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type any_cast(basic_any<Len, Align> &&data) ENTT_NOEXCEPT {
    if constexpr(std::is_copy_constructible_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
        if(auto *const instance = any_cast<std::remove_reference_t<Type>>(&data); instance) {
            return static_cast<Type>(std::move(*instance));
        } else {
            return any_cast<Type>(data);
        }
    } else {
        auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(std::move(*instance));
    }
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
const Type *any_cast(const basic_any<Len, Align> *data) ENTT_NOEXCEPT {
    const auto &info = type_id<std::remove_const_t<std::remove_reference_t<Type>>>();
    return static_cast<const Type *>(data->data(info));
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type *any_cast(basic_any<Len, Align> *data) ENTT_NOEXCEPT {
    const auto &info = type_id<std::remove_const_t<std::remove_reference_t<Type>>>();
    // last attempt to make wrappers for const references return their values
    return static_cast<Type *>(static_cast<constness_as_t<basic_any<Len, Align>, Type> *>(data)->data(info));
}

/**
 * @brief Constructs a wrapper from a given type, passing it all arguments.
 * @tparam Type Type of object to use to initialize the wrapper.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 * @tparam Args Types of arguments to use to construct the new instance.
 * @param args Parameters to use to construct the instance.
 * @return A properly initialized wrapper for an object of the given type.
 */
template<typename Type, std::size_t Len = basic_any<>::length, std::size_t Align = basic_any<Len>::alignment, typename... Args>
basic_any<Len, Align> make_any(Args &&...args) {
    return basic_any<Len, Align>{std::in_place_type<Type>, std::forward<Args>(args)...};
}

/**
 * @brief Forwards its argument and avoids copies for lvalue references.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 * @tparam Type Type of argument to use to construct the new instance.
 * @param value Parameter to use to construct the instance.
 * @return A properly initialized and not necessarily owning wrapper.
 */
template<std::size_t Len = basic_any<>::length, std::size_t Align = basic_any<Len>::alignment, typename Type>
basic_any<Len, Align> forward_as_any(Type &&value) {
    return basic_any<Len, Align>{std::in_place_type<std::conditional_t<std::is_rvalue_reference_v<Type>, std::decay_t<Type>, Type>>, std::forward<Type>(value)};
}

} // namespace entt

namespace std
{


template<> struct hash<entt::any>
{
    std::size_t operator()(const entt::any& s) const ENTT_NOEXCEPT {
        return s.hash();
    }
};


}


#endif
