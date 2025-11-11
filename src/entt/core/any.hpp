#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"
#include "type_info.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

enum class any_request : std::uint8_t {
    info,
    transfer,
    assign,
    compare,
    copy,
    move
};

template<std::size_t Len, std::size_t Align>
struct basic_any_storage {
    static constexpr bool has_buffer = true;
    union {
        const void *instance{};
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        alignas(Align) std::byte buffer[Len];
    };
};

template<std::size_t Align>
struct basic_any_storage<0u, Align> {
    static constexpr bool has_buffer = false;
    const void *instance{};
};

template<typename Type, std::size_t Len, std::size_t Align>
// NOLINTNEXTLINE(bugprone-sizeof-expression)
struct in_situ: std::bool_constant<(Len != 0u) && alignof(Type) <= Align && sizeof(Type) <= Len && std::is_nothrow_move_constructible_v<Type>> {};

template<std::size_t Len, std::size_t Align>
struct in_situ<void, Len, Align>: std::false_type {};

} // namespace internal
/*! @endcond */

/**
 * @brief A SBO friendly, type-safe container for single values of any type.
 * @tparam Len Size of the buffer reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 */
template<std::size_t Len, std::size_t Align>
class basic_any: private internal::basic_any_storage<Len, Align> {
    using request = internal::any_request;
    using base_type = internal::basic_any_storage<Len, Align>;
    using vtable_type = const void *(const request, const basic_any &, const void *);
    using deleter_type = void(const basic_any &);

    template<typename Type>
    static constexpr bool in_situ_v = internal::in_situ<Type, Len, Align>::value;

    template<typename Type>
    static const void *basic_vtable(const request req, const basic_any &value, const void *other) {
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Invalid type");

        switch(const auto *elem = static_cast<const Type *>(value.data()); req) {
        case request::info:
            return &type_id<Type>();
        case request::transfer:
            if constexpr(std::is_move_assignable_v<Type>) {
                // NOLINTNEXTLINE(bugprone-casting-through-void)
                *const_cast<Type *>(elem) = std::move(*static_cast<Type *>(const_cast<void *>(other)));
                return other;
            }
            [[fallthrough]];
        case request::assign:
            if constexpr(std::is_copy_assignable_v<Type>) {
                *const_cast<Type *>(elem) = *static_cast<const Type *>(other);
                return other;
            }
            break;
        case request::compare:
            if constexpr(!std::is_function_v<Type> && !std::is_array_v<Type> && is_equality_comparable_v<Type>) {
                return (*elem == *static_cast<const Type *>(other)) ? other : nullptr;
            } else {
                return (elem == other) ? other : nullptr;
            }
        case request::copy:
            if constexpr(std::is_copy_constructible_v<Type>) {
                // NOLINTNEXTLINE(bugprone-casting-through-void)
                static_cast<basic_any *>(const_cast<void *>(other))->initialize<Type>(*elem);
            }
            break;
        case request::move:
            ENTT_ASSERT(value.mode == any_policy::embedded, "Unexpected policy");
            if constexpr(in_situ_v<Type>) {
                // NOLINTNEXTLINE(bugprone-casting-through-void, bugprone-multi-level-implicit-pointer-conversion)
                return ::new(&static_cast<basic_any *>(const_cast<void *>(other))->buffer) Type{std::move(*const_cast<Type *>(elem))};
            }
        }

        return nullptr;
    }

    template<typename Type>
    static void basic_deleter(const basic_any &value) {
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Invalid type");
        ENTT_ASSERT((value.mode == any_policy::dynamic) || ((value.mode == any_policy::embedded) && !std::is_trivially_destructible_v<Type>), "Unexpected policy");

        const auto *elem = static_cast<const Type *>(value.data());

        if constexpr(in_situ_v<Type>) {
            (value.mode == any_policy::embedded) ? elem->~Type() : (delete elem);
        } else if constexpr(std::is_array_v<Type>) {
            delete[] elem;
        } else {
            delete elem;
        }
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&...args) {
        using plain_type = std::remove_const_t<std::remove_reference_t<Type>>;

        vtable = basic_vtable<plain_type>;

        if constexpr(std::is_void_v<Type>) {
            deleter = nullptr;
            mode = any_policy::empty;
            this->instance = nullptr;
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            deleter = nullptr;
            mode = std::is_const_v<std::remove_reference_t<Type>> ? any_policy::cref : any_policy::ref;
            static_assert((std::is_lvalue_reference_v<Args> && ...) && (sizeof...(Args) == 1u), "Invalid arguments");
            // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
            this->instance = (std::addressof(args), ...);
        } else if constexpr(in_situ_v<plain_type>) {
            if constexpr(std::is_trivially_destructible_v<plain_type>) {
                deleter = nullptr;
            } else {
                deleter = &basic_deleter<plain_type>;
            }

            mode = any_policy::embedded;

            if constexpr(std::is_aggregate_v<plain_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<plain_type>)) {
                ::new(&this->buffer) plain_type{std::forward<Args>(args)...};
            } else {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                ::new(&this->buffer) plain_type(std::forward<Args>(args)...);
            }
        } else {
            deleter = &basic_deleter<plain_type>;
            mode = any_policy::dynamic;

            if constexpr(std::is_aggregate_v<plain_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<plain_type>)) {
                this->instance = new plain_type{std::forward<Args>(args)...};
            } else if constexpr(std::is_array_v<plain_type>) {
                static_assert(sizeof...(Args) == 0u, "Invalid arguments");
                this->instance = new plain_type[std::extent_v<plain_type>]();
            } else {
                this->instance = new plain_type(std::forward<Args>(args)...);
            }
        }
    }

    void invoke_deleter_if_exists() {
        if(deleter != nullptr) {
            deleter(*this);
        }
    }

public:
    /*! @brief Size of the internal buffer. */
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
        : base_type{} {
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs a wrapper taking ownership of the passed object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value A pointer to an object to take ownership of.
     */
    template<typename Type>
    explicit basic_any(std::in_place_t, Type *value)
        : base_type{} {
        static_assert(!std::is_const_v<Type> && !std::is_void_v<Type>, "Non-const non-void pointer required");

        if(value == nullptr) {
            initialize<void>();
        } else {
            initialize<Type &>(*value);
            deleter = &basic_deleter<Type>;
            mode = any_policy::dynamic;
        }
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
        other.vtable(request::copy, other, this);
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_any(basic_any &&other) noexcept
        : base_type{},
          vtable{other.vtable},
          deleter{other.deleter},
          mode{other.mode} {
        if(other.mode == any_policy::embedded) {
            other.vtable(request::move, other, this);
        } else if(other.mode != any_policy::empty) {
            this->instance = std::exchange(other.instance, nullptr);
        }
    }

    /*! @brief Frees the internal buffer, whatever it means. */
    ~basic_any() {
        invoke_deleter_if_exists();
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This any object.
     */
    basic_any &operator=(const basic_any &other) {
        if(this != &other) {
            invoke_deleter_if_exists();

            if(other) {
                other.vtable(request::copy, other, this);
            } else {
                initialize<void>();
            }
        }

        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This any object.
     */
    basic_any &operator=(basic_any &&other) noexcept {
        if(this != &other) {
            invoke_deleter_if_exists();

            if(other.mode == any_policy::embedded) {
                other.vtable(request::move, other, this);
            } else if(other.mode != any_policy::empty) {
                this->instance = std::exchange(other.instance, nullptr);
            }

            vtable = other.vtable;
            deleter = other.deleter;
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
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any &operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] bool has_value() const noexcept {
        return (mode != any_policy::empty);
    }

    /**
     * @brief Returns false if the wrapper does not contain the expected type,
     * true otherwise.
     * @param req Expected type.
     * @return False if the wrapper does not contain the expected type, true
     * otherwise.
     */
    [[nodiscard]] bool has_value(const type_info &req) const noexcept {
        return (info() == req);
    }

    /**
     * @brief Returns false if the wrapper does not contain the expected type,
     * true otherwise.
     * @tparam Type Expected type.
     * @return False if the wrapper does not contain the expected type, true
     * otherwise.
     */
    template<typename Type>
    [[nodiscard]] bool has_value() const noexcept {
        static_assert(std::is_same_v<std::remove_const_t<Type>, Type>, "Invalid type");
        // it could be a call across boundaries, but still for the same type
        return (vtable == &basic_vtable<Type>) || has_value(type_id<Type>());
    }

    /**
     * @brief Returns the object type info if any, `type_id<void>()` otherwise.
     * @return The object type info if any, `type_id<void>()` otherwise.
     */
    [[nodiscard]] const type_info &info() const noexcept {
        return *static_cast<const type_info *>(vtable(request::info, *this, nullptr));
    }

    /*! @copydoc info */
    [[deprecated("use ::info instead")]] [[nodiscard]] const type_info &type() const noexcept {
        return info();
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data() const noexcept {
        if constexpr(base_type::has_buffer) {
            return (mode == any_policy::embedded) ? &this->buffer : this->instance;
        } else {
            return this->instance;
        }
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data(const type_info &req) const noexcept {
        return has_value(req) ? data() : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @tparam Type Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    template<typename Type>
    [[nodiscard]] const Type *data() const noexcept {
        return has_value<std::remove_const_t<Type>>() ? static_cast<const Type *>(data()) : nullptr;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data() noexcept {
        return (mode == any_policy::cref) ? nullptr : const_cast<void *>(std::as_const(*this).data());
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @param req Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] void *data(const type_info &req) noexcept {
        return (mode == any_policy::cref) ? nullptr : const_cast<void *>(std::as_const(*this).data(req));
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @tparam Type Expected type.
     * @return An opaque pointer the contained instance, if any.
     */
    template<typename Type>
    [[nodiscard]] Type *data() noexcept {
        if constexpr(std::is_const_v<Type>) {
            return std::as_const(*this).template data<std::remove_const_t<Type>>();
        } else {
            return (mode == any_policy::cref) ? nullptr : const_cast<Type *>(std::as_const(*this).template data<std::remove_const_t<Type>>());
        }
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        invoke_deleter_if_exists();
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns a value to the contained object without replacing it.
     * @param other The value to assign to the contained object.
     * @return True in case of success, false otherwise.
     */
    bool assign(const basic_any &other) {
        // it could be a call across boundaries, but still for the same type
        if(other && (mode != any_policy::cref) && ((vtable == other.vtable) || has_value(other.info()))) {
            return (vtable(request::assign, *this, other.data()) != nullptr);
        }

        return false;
    }

    /*! @copydoc assign */
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    bool assign(basic_any &&other) {
        // it could be a call across boundaries, but still for the same type
        if(other && (mode != any_policy::cref) && ((vtable == other.vtable) || has_value(other.info()))) {
            return (other.mode == any_policy::cref) ? (vtable(request::assign, *this, std::as_const(other).data()) != nullptr) : (vtable(request::transfer, *this, other.data()) != nullptr);
        }

        return false;
    }

    /*! @brief Destroys contained object */
    void reset() {
        invoke_deleter_if_exists();
        initialize<void>();
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    [[nodiscard]] bool operator==(const basic_any &other) const noexcept {
        // it could be a call across boundaries, but still for the same type
        if(other && ((vtable == other.vtable) || has_value(other.info()))) {
            return (vtable(request::compare, *this, other.data()) != nullptr);
        }

        return (!*this && !other);
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
        basic_any other = std::as_const(*this).as_ref();
        other.mode = (mode == any_policy::cref ? any_policy::cref : any_policy::ref);
        return other;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const noexcept {
        basic_any other{};
        other.instance = data();
        other.vtable = vtable;
        other.mode = any_policy::cref;
        return other;
    }

    /**
     * @brief Returns true if a wrapper owns its object, false otherwise.
     * @return True if the wrapper owns its object, false otherwise.
     */
    [[nodiscard]] bool owner() const noexcept {
        return (mode == any_policy::dynamic || mode == any_policy::embedded);
    }

    /**
     * @brief Returns the current mode of an any object.
     * @return The current mode of the any object.
     */
    [[nodiscard]] any_policy policy() const noexcept {
        return mode;
    }

private:
    vtable_type *vtable{};
    deleter_type *deleter{};
    any_policy mode{};
};

/**
 * @brief Performs type-safe access to the contained object.
 * @tparam Type Type to which conversion is required.
 * @tparam Len Size of the buffer reserved for the small buffer optimization.
 * @tparam Align Alignment requirement.
 * @param data Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] std::remove_const_t<Type> any_cast(const basic_any<Len, Align> &data) noexcept {
    const auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] std::remove_const_t<Type> any_cast(basic_any<Len, Align> &data) noexcept {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto *const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
[[nodiscard]] std::remove_const_t<Type> any_cast(basic_any<Len, Align> &&data) noexcept {
    if constexpr(std::is_copy_constructible_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
        if(auto *const instance = any_cast<std::remove_reference_t<Type>>(&data); instance) {
            return static_cast<Type>(std::move(*instance));
        }

        return any_cast<Type>(data);
    } else {
        auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(std::move(*instance));
    }
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] const Type *any_cast(const basic_any<Len, Align> *data) noexcept {
    return data->template data<std::remove_const_t<Type>>();
}

/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type *any_cast(basic_any<Len, Align> *data) noexcept {
    if constexpr(std::is_const_v<Type>) {
        // last attempt to make wrappers for const references return their values
        return any_cast<Type>(&std::as_const(*data));
    } else {
        return data->template data<Type>();
    }
}

/**
 * @brief Constructs a wrapper from a given type, passing it all arguments.
 * @tparam Type Type of object to use to initialize the wrapper.
 * @tparam Len Size of the buffer reserved for the small buffer optimization.
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
 * @tparam Len Size of the buffer reserved for the small buffer optimization.
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
