#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP


#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "type_info.hpp"


namespace entt {


/*! @brief A SBO friendly, type-safe container for single values of any type. */
class any {
    enum class operation { COPY, MOVE, DTOR, ADDR, REF, TYPE };

    using storage_type = std::aligned_storage_t<sizeof(double[2]), alignof(double[2])>;
    using vtable_type = void *(const operation, const any &, void *);

    template<typename Type>
    static constexpr auto in_situ = sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    static void * basic_vtable(const operation op, const any &from, void *to) {
        if constexpr(std::is_void_v<Type>) {
            return nullptr;
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            switch(op) {
            case operation::REF:
                static_cast<any *>(to)->vtable = from.vtable;
                [[fallthrough]];
            case operation::COPY:
            case operation::MOVE:
                static_cast<any *>(to)->instance = from.instance;
                [[fallthrough]];
            case operation::DTOR:
                break;
            case operation::ADDR:
                return from.instance;
            case operation::TYPE:
                *static_cast<type_info *>(to) = type_id<std::remove_reference_t<Type>>();
                break;
            }
        } else if constexpr(in_situ<Type>) {
            auto *instance = const_cast<Type *>(std::launder(reinterpret_cast<const Type *>(&from.storage)));

            switch(op) {
            case operation::COPY:
                new (&static_cast<any *>(to)->storage) Type{std::as_const(*instance)};
                break;
            case operation::MOVE:
                new (&static_cast<any *>(to)->storage) Type{std::move(*instance)};
                [[fallthrough]];
            case operation::DTOR:
                instance->~Type();
                break;
            case operation::ADDR:
                return instance;
            case operation::REF:
                static_cast<any *>(to)->vtable = basic_vtable<std::add_lvalue_reference_t<Type>>;
                static_cast<any *>(to)->instance = instance;
                break;
            case operation::TYPE:
                *static_cast<type_info *>(to) = type_id<Type>();
                break;
            }
        } else {
            switch(op) {
            case operation::COPY:
                static_cast<any *>(to)->instance = new Type{std::as_const(*static_cast<Type *>(from.instance))};
                break;
            case operation::MOVE:
                static_cast<any *>(to)->instance = from.instance;
                break;
            case operation::DTOR:
                delete static_cast<Type *>(from.instance);
                break;
            case operation::ADDR:
                return from.instance;
            case operation::REF:
                static_cast<any *>(to)->vtable = basic_vtable<std::add_lvalue_reference_t<Type>>;
                static_cast<any *>(to)->instance = from.instance;
                break;
            case operation::TYPE:
                *static_cast<type_info *>(to) = type_id<Type>();
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
        : vtable{&basic_vtable<std::add_lvalue_reference_t<Type>>},
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
     * @return This any any object.
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
        return vtable(operation::ADDR, *this, nullptr);
    }

    /*! @copydoc data */
    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
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
        return !(vtable(operation::ADDR, *this, nullptr) == nullptr);
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
    [[nodiscard]] friend any as_ref(const any &other) ENTT_NOEXCEPT {
        any ref{};
        other.vtable(operation::REF, other, &ref);
        return ref;
    }

private:
    vtable_type *vtable;
    union { void *instance; storage_type storage; };
};


/**
 * @brief Performs type-safe access to the contained object.
 * @param any Target any object.
 * @return The element converted to the requested type.
 */
template<typename Type>
Type any_cast(const any &any) ENTT_NOEXCEPT {
    auto *instance = any_cast<std::remove_cv_t<std::remove_reference_t<Type>>>(&any);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type>
Type any_cast(any &any) ENTT_NOEXCEPT {
    auto *instance = any_cast<std::remove_cv_t<std::remove_reference_t<Type>>>(&any);
    ENTT_ASSERT(instance);
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type>
Type any_cast(any &&any) ENTT_NOEXCEPT {
    auto *instance = any_cast<std::remove_cv_t<std::remove_reference_t<Type>>>(&any);
    ENTT_ASSERT(instance);
    return static_cast<Type>(std::move(*instance));
}


/*! @copydoc any_cast */
template<typename Type>
const Type * any_cast(const any *any) ENTT_NOEXCEPT {
    return (any->type() == type_id<Type>() ? static_cast<const Type *>(any->data()) : nullptr);
}


/*! @copydoc any_cast */
template<typename Type>
Type * any_cast(any *any) ENTT_NOEXCEPT {
    return (any->type() == type_id<Type>() ? static_cast<Type *>(any->data()) : nullptr);
}


}


#endif
