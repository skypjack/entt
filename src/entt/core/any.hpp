#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP


#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"
#include "type_info.hpp"
#include "type_traits.hpp"


namespace entt {


/**
 * @brief A SBO friendly, type-safe container for single values of any type.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 */
template<std::size_t Len>
class basic_any {
    enum class operation { COPY, MOVE, DTOR, COMP, ADDR, CADDR, REF, CREF, TYPE };

    using storage_type = std::aligned_storage_t<Len == 0u ? 1u : Len>;
    using vtable_type = const void *(const operation, const basic_any &, const void *);

    template<typename Type>
    static constexpr bool in_situ = Len && sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    template<typename Type>
    static Type & as(const void *to) {
        return *const_cast<Type *>(static_cast<const Type *>(to));
    }

    template<typename Type>
    static const void * basic_vtable([[maybe_unused]] const operation op, [[maybe_unused]] const basic_any &from, [[maybe_unused]] const void *to) {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(std::is_lvalue_reference_v<Type>) {
                using base_type = std::remove_const_t<std::remove_reference_t<Type>>;

                switch(op) {
                case operation::COPY:
                    if constexpr(std::is_copy_constructible_v<base_type>) {
                        as<basic_any>(to).template emplace<base_type>(*static_cast<const base_type *>(from.instance));
                    }
                    break;
                case operation::MOVE:
                    as<basic_any>(to).instance = from.instance;
                    [[fallthrough]];
                case operation::DTOR:
                    break;
                case operation::COMP:
                    return compare<base_type>(from.instance, to) ? to : nullptr;
                case operation::ADDR:
                    return std::is_const_v<std::remove_reference_t<Type>> ? nullptr : from.instance;
                case operation::CADDR:
                    return from.instance;
                case operation::REF:
                    as<basic_any>(to).instance = from.instance;
                    as<basic_any>(to).vtable = basic_vtable<Type>;
                    break;
                case operation::CREF:
                    as<basic_any>(to).instance = from.instance;
                    as<basic_any>(to).vtable = basic_vtable<const base_type &>;
                    break;
                case operation::TYPE:
                    as<type_info>(to) = type_id<base_type>();
                    break;
                }
            } else if constexpr(in_situ<Type>) {
                #if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606L
                auto *instance = const_cast<Type *>(std::launder(reinterpret_cast<const Type *>(&from.storage)));
                #else
                auto *instance = const_cast<Type *>(reinterpret_cast<const Type *>(&from.storage));
                #endif

                switch(op) {
                case operation::COPY:
                    if constexpr(std::is_copy_constructible_v<Type>) {
                        new (&as<basic_any>(to).storage) Type{std::as_const(*instance)};
                        as<basic_any>(to).vtable = from.vtable;
                    }
                    break;
                case operation::MOVE:
                    new (&as<basic_any>(to).storage) Type{std::move(*instance)};
                    [[fallthrough]];
                case operation::DTOR:
                    instance->~Type();
                    break;
                case operation::COMP:
                    return compare<Type>(instance, to) ? to : nullptr;
                case operation::ADDR:
                case operation::CADDR:
                    return instance;
                case operation::REF:
                    as<basic_any>(to).instance = instance;
                    as<basic_any>(to).vtable = basic_vtable<Type &>;
                    break;
                case operation::CREF:
                    as<basic_any>(to).instance = instance;
                    as<basic_any>(to).vtable = basic_vtable<const Type &>;
                    break;
                case operation::TYPE:
                    as<type_info>(to) = type_id<Type>();
                    break;
                }
            } else {
                switch(op) {
                case operation::COPY:
                    if constexpr(std::is_copy_constructible_v<Type>) {
                        as<basic_any>(to).instance = new Type{*static_cast<const Type *>(from.instance)};
                        as<basic_any>(to).vtable = from.vtable;
                    }
                    break;
                case operation::MOVE:
                    as<basic_any>(to).instance = from.instance;
                    break;
                case operation::DTOR:
                    if constexpr(std::is_array_v<Type>) {
                        delete[] static_cast<const Type *>(from.instance);
                    } else {
                        delete static_cast<const Type *>(from.instance);
                    }
                    break;
                case operation::COMP:
                    return compare<Type>(from.instance, to) ? to : nullptr;
                case operation::ADDR:
                case operation::CADDR:
                    return from.instance;
                case operation::REF:
                    as<basic_any>(to).instance = from.instance;
                    as<basic_any>(to).vtable = basic_vtable<Type &>;
                    break;
                case operation::CREF:
                    as<basic_any>(to).instance = from.instance;
                    as<basic_any>(to).vtable = basic_vtable<const Type &>;
                    break;
                case operation::TYPE:
                    as<type_info>(to) = type_id<Type>();
                    break;
                }
            }
        }

        return nullptr;
    }

public:
    /*! @brief Default constructor. */
    basic_any() ENTT_NOEXCEPT
        : basic_any{std::in_place_type<void>}
    {}

    /**
     * @brief Constructs an any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : instance{},
          vtable{&basic_vtable<Type>}
    {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(std::is_lvalue_reference_v<Type>) {
                static_assert(sizeof...(Args) == 1u && (std::is_lvalue_reference_v<Args> && ...));
                instance = (&args, ...);
            } else if constexpr(in_situ<Type>) {
                new (&storage) Type(std::forward<Args>(args)...);
            } else {
                instance = new Type(std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Constructs an any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    basic_any(std::reference_wrapper<Type> value) ENTT_NOEXCEPT
        : basic_any{std::in_place_type<Type &>, value.get()}
    {}

    /**
     * @brief Constructs an any from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : basic_any{std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    basic_any(const basic_any &other)
        : basic_any{}
    {
        other.vtable(operation::COPY, other, this);
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_any(basic_any &&other) ENTT_NOEXCEPT
        : basic_any{}
    {
        other.vtable(operation::MOVE, other, this);
        vtable = std::exchange(other.vtable, &basic_vtable<void>);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~basic_any() {
        vtable(operation::DTOR, *this, nullptr);
    }

    /**
     * @brief Assignment operator.
     * @param other The instance to assign from.
     * @return This any object.
     */
    basic_any & operator=(basic_any other) {
        swap(*this, other);
        return *this;
    }

    /**
     * @brief Returns the type of the contained object.
     * @return The type of the contained object, if any.
     */
    [[nodiscard]] type_info type() const ENTT_NOEXCEPT {
        type_info info;
        vtable(operation::TYPE, *this, &info);
        return info;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void * data() const ENTT_NOEXCEPT {
        return vtable(operation::CADDR, *this, nullptr);
    }

    /*! @copydoc data */
    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(vtable(operation::ADDR, *this, nullptr));
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = basic_any{std::in_place_type<Type>, std::forward<Args>(args)...};
    }

    /*! @brief Destroys contained object */
    void reset() {
        *this = basic_any{};
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(vtable(operation::CADDR, *this, nullptr) == nullptr);
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    bool operator==(const basic_any &other) const ENTT_NOEXCEPT {
        return type() == other.type() && (vtable(operation::COMP, *this, other.data()) == other.data());
    }

    /**
     * @brief Swaps two any objects.
     * @param lhs A valid any object.
     * @param rhs A valid any object.
     */
    friend void swap(basic_any &lhs, basic_any &rhs) {
        basic_any tmp{};
        lhs.vtable(operation::MOVE, lhs, &tmp);
        rhs.vtable(operation::MOVE, rhs, &lhs);
        lhs.vtable(operation::MOVE, tmp, &rhs);
        std::swap(lhs.vtable, rhs.vtable);
    }

    /**
     * @brief Aliasing constructor.
     * @return An any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_any as_ref() ENTT_NOEXCEPT {
        basic_any ref{};
        vtable(operation::REF, *this, &ref);
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const ENTT_NOEXCEPT {
        basic_any ref{};
        vtable(operation::CREF, *this, &ref);
        return ref;
    }

private:
    union { const void *instance; storage_type storage; };
    vtable_type *vtable;
};


/**
 * @brief Checks if two wrappers differ in their content.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @param lhs A wrapper, either empty or not.
 * @param rhs A wrapper, either empty or not.
 * @return True if the two wrappers differ in their content, false otherwise.
 */
template<std::size_t Len>
[[nodiscard]] inline bool operator!=(const basic_any<Len> &lhs, const basic_any<Len> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Performs type-safe access to the contained object.
 * @tparam Type Type to which conversion is required.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @param data Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type, std::size_t Len>
Type any_cast(const basic_any<Len> &data) ENTT_NOEXCEPT {
    const auto * const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len>
Type any_cast(basic_any<Len> &data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len>
Type any_cast(basic_any<Len> &&data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(std::move(*instance));
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len>
const Type * any_cast(const basic_any<Len> *data) ENTT_NOEXCEPT {
    return (data->type() == type_id<Type>() ? static_cast<const Type *>(data->data()) : nullptr);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len>
Type * any_cast(basic_any<Len> *data) ENTT_NOEXCEPT {
    // last attempt to make wrappers for const references return their values
    return (data->type() == type_id<Type>() ? static_cast<Type *>(static_cast<constness_as_t<basic_any<Len>, Type> *>(data)->data()) : nullptr);
}


}


#endif
