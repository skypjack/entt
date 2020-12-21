#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP


#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "type_info.hpp"
#include "type_traits.hpp"


namespace entt {


/*! @brief A SBO friendly, type-safe container for single values of any type. */
class any {
    enum class operation { COPY, MOVE, DTOR, COMP, ADDR, CADDR, REF, CREF, TYPE };

    using storage_type = std::aligned_storage_t<sizeof(double[2]), alignof(double[2])>;
    using vtable_type = const void *(const operation, const any &, const void *);

    template<typename Type>
    static constexpr auto in_situ = sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    static type_info & as_type_info(const void *data) {
        return *const_cast<type_info *>(static_cast<const type_info *>(data));
    }

    static any & as_any(const void *data) {
        return *const_cast<any *>(static_cast<const any *>(data));
    }

    template<typename Type>
    static const void * basic_vtable(const operation op, const any &from, const void *to) {
        if constexpr(std::is_void_v<Type>) {
            return nullptr;
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            using base_type = std::remove_reference_t<Type>;

            switch(op) {
            case operation::REF:
            case operation::CREF:
                as_any(to).vtable = (op == operation::REF) ? basic_vtable<base_type &> : basic_vtable<const base_type &>;
                [[fallthrough]];
            case operation::COPY:
            case operation::MOVE:
                as_any(to).instance = from.instance;
                [[fallthrough]];
            case operation::DTOR:
                break;
            case operation::COMP:
                return compare<base_type>(from.instance, to) ? to : nullptr;
            case operation::ADDR:
                return std::is_const_v<base_type> ? nullptr : from.instance;
            case operation::CADDR:
                return from.instance;
            case operation::TYPE:
                as_type_info(to) = type_id<base_type>();
                break;
            }
        } else if constexpr(in_situ<Type>) {
            auto *instance = const_cast<Type *>(std::launder(reinterpret_cast<const Type *>(&from.storage)));

            switch(op) {
            case operation::COPY:
                new (&as_any(to).storage) Type{std::as_const(*instance)};
                break;
            case operation::MOVE:
                new (&as_any(to).storage) Type{std::move(*instance)};
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
            case operation::CREF:
                as_any(to).vtable = (op == operation::REF) ? basic_vtable<Type &> : basic_vtable<const Type &>;
                as_any(to).instance = instance;
                break;
            case operation::TYPE:
                as_type_info(to) = type_id<Type>();
                break;
            }
        } else {
            switch(op) {
            case operation::COPY:
                as_any(to).instance = new Type{*static_cast<const Type *>(from.instance)};
                break;
            case operation::REF:
            case operation::CREF:
                as_any(to).vtable = (op == operation::REF) ? basic_vtable<Type &> : basic_vtable<const Type &>;
                [[fallthrough]];
            case operation::MOVE:
                as_any(to).instance = from.instance;
                break;
            case operation::DTOR:
                delete static_cast<const Type *>(from.instance);
                break;
            case operation::COMP:
                return compare<Type>(from.instance, to) ? to : nullptr;
            case operation::ADDR:
            case operation::CADDR:
                return from.instance;
            case operation::TYPE:
                as_type_info(to) = type_id<Type>();
                break;
            }
        }

        return nullptr;
    }

public:
    /*! @brief Default constructor. */
    any() ENTT_NOEXCEPT
        : vtable{&basic_vtable<void>},
          instance{}
    {}

    /**
     * @brief Constructs an any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit any(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : vtable{&basic_vtable<Type>},
          instance{}
    {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(in_situ<Type>) {
                new (&storage) Type{std::forward<Args>(args)...};
            } else {
                instance = new Type{std::forward<Args>(args)...};
            }
        }
    }

    /**
     * @brief Constructs an any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    any(std::reference_wrapper<Type> value) ENTT_NOEXCEPT
        : vtable{&basic_vtable<Type &>},
          instance{&value.get()}
    {}

    /**
     * @brief Constructs an any from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, any>>>
    any(Type &&value)
        : any{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    any(const any &other)
        : any{}
    {
        vtable = other.vtable;
        vtable(operation::COPY, other, this);
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    any(any &&other) ENTT_NOEXCEPT
        : any{}
    {
        vtable = std::exchange(other.vtable, &basic_vtable<void>);
        vtable(operation::MOVE, other, this);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~any() {
        vtable(operation::DTOR, *this, nullptr);
    }

    /**
     * @brief Assignment operator.
     * @param other The instance to assign from.
     * @return This any object.
     */
    any & operator=(any other) {
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
        *this = any{std::in_place_type<Type>, std::forward<Args>(args)...};
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
    bool operator==(const any &other) const ENTT_NOEXCEPT {
        return type() == other.type() && (vtable(operation::COMP, *this, other.data()) == other.data());
    }

    /**
     * @brief Swaps two any objects.
     * @param lhs A valid any object.
     * @param rhs A valid any object.
     */
    friend void swap(any &lhs, any &rhs) {
        any tmp{};
        lhs.vtable(operation::MOVE, lhs, &tmp);
        rhs.vtable(operation::MOVE, rhs, &lhs);
        lhs.vtable(operation::MOVE, tmp, &rhs);
        std::swap(lhs.vtable, rhs.vtable);
    }

    /**
     * @brief Aliasing constructor.
     * @param other A reference to an object that isn't necessarily initialized.
     * @return An any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] friend any as_ref(any &other) ENTT_NOEXCEPT {
        any ref{};
        other.vtable(operation::REF, other, &ref);
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] friend any as_ref(const any &other) ENTT_NOEXCEPT {
        any ref{};
        other.vtable(operation::CREF, other, &ref);
        return ref;
    }

private:
    vtable_type *vtable;
    union { const void *instance; storage_type storage; };
};


/**
 * @brief Checks if two wrappers differ in their content.
 * @param lhs A wrapper, either empty or not.
 * @param rhs A wrapper, either empty or not.
 * @return True if the two wrappers differ in their content, false otherwise.
 */
[[nodiscard]] inline bool operator!=(const any &lhs, const any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Performs type-safe access to the contained object.
 * @param data Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type>
Type any_cast(const any &data) ENTT_NOEXCEPT {
    auto * const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type>
Type any_cast(any &data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type>
Type any_cast(any &&data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::conditional_t<std::is_reference_v<Type>, std::remove_reference_t<Type>, const Type>>(&data);
    ENTT_ASSERT(instance);
    return static_cast<Type>(std::move(*instance));
}


/*! @copydoc any_cast */
template<typename Type>
const Type * any_cast(const any *data) ENTT_NOEXCEPT {
    return (data->type() == type_id<Type>() ? static_cast<const Type *>(data->data()) : nullptr);
}


/*! @copydoc any_cast */
template<typename Type>
Type * any_cast(any *data) ENTT_NOEXCEPT {
    // last attempt to make wrappers for const references return their values
    return (data->type() == type_id<Type>() ? static_cast<Type *>(static_cast<constness_as_t<any, Type> *>(data)->data()) : nullptr);
}


}


#endif
