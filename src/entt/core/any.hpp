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

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

enum class any_operation : std::uint8_t {
    copy,
    move,
    transfer,
    assign,
    destroy,
    compare,
    get
};

} // namespace internal
/*! @endcond */

/*! @brief Possible modes of an any object. */
enum class any_policy : std::uint8_t {
    /*! @brief Default mode, the object owns the contained element. */
    owner,
    /*! @brief Aliasing mode, the object _points_ to a non-const element. */
    ref,
    /*! @brief Const aliasing mode, the object _points_ to a const element. */
    cref
};

/**
 * @brief A SBO friendly, type-safe container for single values of any type.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 */
template<std::size_t Len, std::size_t Align>
class basic_any {
    using operation = internal::any_operation;
    using vtable_type = const void *(const operation, const basic_any &, const void *);

    struct storage_type {
        alignas(Align) std::byte data[Len + !Len];
    };

    template<typename Type>
    static constexpr bool in_situ = Len && alignof(Type) <= Align && sizeof(Type) <= Len && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    static const void *basic_vtable(const operation op, const basic_any &value, const void *other) {
        static_assert(!std::is_void_v<Type> && std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, Type>, "Invalid type");
        const Type *element = nullptr;

        if constexpr(in_situ<Type>) {
            element = (value.mode == any_policy::owner) ? reinterpret_cast<const Type *>(&value.storage) : static_cast<const Type *>(value.instance);
        } else {
            element = static_cast<const Type *>(value.instance);
        }

        switch(op) {
        case operation::copy:
            if constexpr(std::is_copy_constructible_v<Type>) {
                static_cast<basic_any *>(const_cast<void *>(other))->initialize<Type>(*element);
            }
            break;
        case operation::move:
            if constexpr(in_situ<Type>) {
                if(value.mode == any_policy::owner) {
                    return new(&static_cast<basic_any *>(const_cast<void *>(other))->storage) Type{std::move(*const_cast<Type *>(element))};
                }
            }

            return (static_cast<basic_any *>(const_cast<void *>(other))->instance = std::exchange(const_cast<basic_any &>(value).instance, nullptr));
        case operation::transfer:
            if constexpr(std::is_move_assignable_v<Type>) {
                *const_cast<Type *>(element) = std::move(*static_cast<Type *>(const_cast<void *>(other)));
                return other;
            }
            [[fallthrough]];
        case operation::assign:
            if constexpr(std::is_copy_assignable_v<Type>) {
                *const_cast<Type *>(element) = *static_cast<const Type *>(other);
                return other;
            }
            break;
        case operation::destroy:
            if constexpr(in_situ<Type>) {
                element->~Type();
            } else if constexpr(std::is_array_v<Type>) {
                delete[] element;
            } else {
                delete element;
            }
            break;
        case operation::compare:
            if constexpr(!std::is_function_v<Type> && !std::is_array_v<Type> && is_equality_comparable_v<Type>) {
                return *element == *static_cast<const Type *>(other) ? other : nullptr;
            } else {
                return (element == other) ? other : nullptr;
            }
        case operation::get:
            return element;
        }

        return nullptr;
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&...args) {
        info = &type_id<std::remove_cv_t<std::remove_reference_t<Type>>>();

        if constexpr(!std::is_void_v<Type>) {
            vtable = basic_vtable<std::remove_cv_t<std::remove_reference_t<Type>>>;

            if constexpr(std::is_lvalue_reference_v<Type>) {
                static_assert((std::is_lvalue_reference_v<Args> && ...) && (sizeof...(Args) == 1u), "Invalid arguments");
                mode = std::is_const_v<std::remove_reference_t<Type>> ? any_policy::cref : any_policy::ref;
                instance = (std::addressof(args), ...);
            } else if constexpr(in_situ<std::remove_cv_t<std::remove_reference_t<Type>>>) {
                if constexpr(std::is_aggregate_v<std::remove_cv_t<std::remove_reference_t<Type>>> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>)) {
                    new(&storage) std::remove_cv_t<std::remove_reference_t<Type>>{std::forward<Args>(args)...};
                } else {
                    new(&storage) std::remove_cv_t<std::remove_reference_t<Type>>(std::forward<Args>(args)...);
                }
            } else {
                if constexpr(std::is_aggregate_v<std::remove_cv_t<std::remove_reference_t<Type>>> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>)) {
                    instance = new std::remove_cv_t<std::remove_reference_t<Type>>{std::forward<Args>(args)...};
                } else {
                    instance = new std::remove_cv_t<std::remove_reference_t<Type>>(std::forward<Args>(args)...);
                }
            }
        }
    }

    basic_any(const basic_any &other, const any_policy pol) noexcept
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
    constexpr basic_any() noexcept
        : basic_any{std::in_place_type<void>} {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, Args &&...args)
        : instance{},
          info{},
          vtable{},
          mode{any_policy::owner} {
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : basic_any{std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)} {}

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
    basic_any(basic_any &&other) noexcept
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
        if(vtable && (mode == any_policy::owner)) {
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
    basic_any &operator=(basic_any &&other) noexcept {
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
    [[nodiscard]] const type_info &type() const noexcept {
        return *info;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data() const noexcept {
        return vtable ? vtable(operation::get, *this, nullptr) : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data(const type_info &req) const noexcept {
        return *info == req ? data() : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data() noexcept {
        return mode == any_policy::cref ? nullptr : const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data(const type_info &req) noexcept {
        return mode == any_policy::cref ? nullptr : const_cast<void *>(std::as_const(*this).data(req));
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
     * @brief Assigns a value to the contained object without replacing it.
     * @param other The value to assign to the contained object.
     * @return True in case of success, false otherwise.
     */
    bool assign(const basic_any &other) {
        if(vtable && mode != any_policy::cref && *info == *other.info) {
            return (vtable(operation::assign, *this, other.data()) != nullptr);
        }

        return false;
    }

    /*! @copydoc assign */
    bool assign(basic_any &&other) {
        if(vtable && mode != any_policy::cref && *info == *other.info) {
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
        if(vtable && (mode == any_policy::owner)) {
            vtable(operation::destroy, *this, nullptr);
        }

        // unnecessary but it helps to detect nasty bugs
        ENTT_ASSERT((instance = nullptr) == nullptr, "");
        info = &type_id<void>();
        vtable = nullptr;
        mode = any_policy::owner;
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return vtable != nullptr;
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    [[nodiscard]] bool operator==(const basic_any &other) const noexcept {
        if(vtable && *info == *other.info) {
            return (vtable(operation::compare, *this, other.data()) != nullptr);
        }

        return (!vtable && !other.vtable);
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return True if the two objects differ in their content, false otherwise.
     */
    [[nodiscard]] bool operator!=(const basic_any &other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Aliasing constructor.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_any as_ref() noexcept {
        return basic_any{*this, (mode == any_policy::cref ? any_policy::cref : any_policy::ref)};
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const noexcept {
        return basic_any{*this, any_policy::cref};
    }

    /**
     * @brief Returns true if a wrapper owns its object, false otherwise.
     * @return True if the wrapper owns its object, false otherwise.
     */
    [[deprecated("use policy() and any_policy instead")]] [[nodiscard]] bool owner() const noexcept {
        return (mode == any_policy::owner);
    }

    /**
     * @brief Returns the current mode of an any object.
     * @return The current mode of the any object.
     */
    [[nodiscard]] any_policy policy() const noexcept {
        return mode;
    }

private:
    union {
        const void *instance;
        storage_type storage;
    };
    const type_info *info;
    vtable_type *vtable;
    any_policy mode;
};

/**
 * @brief Performs type-safe access to the contained object.
 * @tparam Type Type to which conversion is required.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Alignment requirement.
 * @param data Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type any_cast(const basic_any<Len, Align> &data) noexcept {
    const auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type any_cast(basic_any<Len, Align> &data) noexcept {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto *const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type any_cast(basic_any<Len, Align> &&data) noexcept {
    if constexpr(std::is_copy_constructible_v<std::remove_cv_t<std::remove_reference_t<Type>>>) {
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
[[nodiscard]] const Type *any_cast(const basic_any<Len, Align> *data) noexcept {
    const auto &info = type_id<std::remove_cv_t<Type>>();
    return static_cast<const Type *>(data->data(info));
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type *any_cast(basic_any<Len, Align> *data) noexcept {
    if constexpr(std::is_const_v<Type>) {
        // last attempt to make wrappers for const references return their values
        return any_cast<Type>(&std::as_const(*data));
    } else {
        const auto &info = type_id<std::remove_cv_t<Type>>();
        return static_cast<Type *>(data->data(info));
    }
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
[[nodiscard]] basic_any<Len, Align> make_any(Args &&...args) {
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
[[nodiscard]] basic_any<Len, Align> forward_as_any(Type &&value) {
    return basic_any<Len, Align>{std::in_place_type<Type &&>, std::forward<Type>(value)};
}

} // namespace entt

#endif
