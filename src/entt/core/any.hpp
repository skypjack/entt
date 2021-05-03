#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP


#include <cstddef>
#include <functional>
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
    enum class operation: std::uint8_t { COPY, MOVE, DTOR, COMP, ADDR, CADDR, TYPE };
    enum class policy: std::uint8_t { OWNER, REF, CREF };

    using storage_type = std::aligned_storage_t<Len + !Len, Align>;
    using vtable_type = const void *(const operation, const basic_any &, void *);

    template<typename Type>
    static constexpr bool in_situ = Len && alignof(Type) <= alignof(storage_type) && sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    [[nodiscard]] static constexpr policy type_to_policy() {
        if constexpr(std::is_lvalue_reference_v<Type>) {
            if constexpr(std::is_const_v<std::remove_reference_t<Type>>) {
                return policy::CREF;
            } else {
                return policy::REF;
            }
        } else {
            return policy::OWNER;
        }
    }

    template<typename Type>
    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    template<typename Type>
    static const void * basic_vtable([[maybe_unused]] const operation op, [[maybe_unused]] const basic_any &from, [[maybe_unused]] void *to) {
        static_assert(std::is_same_v<std::remove_reference_t<std::remove_const_t<Type>>, Type>, "Invalid type");

        if constexpr(!std::is_void_v<Type>) {
            const Type *instance = (in_situ<Type> && from.mode == policy::OWNER)
                ? ENTT_LAUNDER(reinterpret_cast<const Type *>(&from.storage))
                : static_cast<const Type *>(from.instance);

            switch(op) {
            case operation::COPY:
                if constexpr(std::is_copy_constructible_v<Type>) {
                    static_cast<basic_any *>(to)->emplace<Type>(*instance);
                }
                break;
            case operation::MOVE:
                if constexpr(in_situ<Type>) {
                    if(from.mode == policy::OWNER) {
                        return new (&static_cast<basic_any *>(to)->storage) Type{std::move(*const_cast<Type *>(instance))};
                    }
                }

                return (static_cast<basic_any *>(to)->instance = std::exchange(const_cast<basic_any &>(from).instance, nullptr));
            case operation::DTOR:
                if(from.mode == policy::OWNER) {
                    if constexpr(in_situ<Type>) {
                        instance->~Type();
                    } else if constexpr(std::is_array_v<Type>) {
                        delete[] instance;
                    } else {
                        delete instance;
                    }
                }
                break;
            case operation::COMP:
                return compare<Type>(instance, (*static_cast<const basic_any **>(to))->data()) ? to : nullptr;
            case operation::ADDR:
                if(from.mode == policy::CREF) {
                    return nullptr;
                }
                [[fallthrough]];
            case operation::CADDR:
                return instance;
            case operation::TYPE:
                *static_cast<type_info *>(to) = type_id<Type>();
                break;
            }
        }

        return nullptr;
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&... args) {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(std::is_lvalue_reference_v<Type>) {
                static_assert(sizeof...(Args) == 1u && (std::is_lvalue_reference_v<Args> && ...), "Invalid arguments");
                instance = (std::addressof(args), ...);
            } else if constexpr(in_situ<Type>) {
                if constexpr(sizeof...(Args) != 0u && std::is_aggregate_v<Type>) {
                    new (&storage) Type{std::forward<Args>(args)...};
                } else {
                    new (&storage) Type(std::forward<Args>(args)...);
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
          vtable{other.vtable},
          mode{pol}
    {}

public:
    /*! @brief Size of the internal storage. */
    static constexpr auto length = Len;
    /*! @brief Alignment requirement. */
    static constexpr auto alignment = Align;

    /*! @brief Default constructor. */
    basic_any() ENTT_NOEXCEPT
        : instance{},
          vtable{&basic_vtable<void>},
          mode{policy::OWNER}
    {}

    /**
     * @brief Constructs a wrapper by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, Args &&... args)
        : instance{},
          vtable{&basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>},
          mode{type_to_policy<Type>()}
    {
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs a wrapper that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    basic_any(std::reference_wrapper<Type> value) ENTT_NOEXCEPT
        : basic_any{}
    {
        // invokes deprecated assignment operator (and avoids issues with vs2017)
        *this = value;
    }

    /**
     * @brief Constructs a wrapper from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : instance{},
          vtable{&basic_vtable<std::decay_t<Type>>},
          mode{policy::OWNER}
    {
        initialize<std::decay_t<Type>>(std::forward<Type>(value));
    }

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    basic_any(const basic_any &other)
        : instance{},
          vtable{&basic_vtable<void>},
          mode{policy::OWNER}
    {
        other.vtable(operation::COPY, other, this);
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_any(basic_any &&other) ENTT_NOEXCEPT
        : instance{},
          vtable{other.vtable},
          mode{other.mode}
    {
        vtable(operation::MOVE, other, this);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~basic_any() {
        vtable(operation::DTOR, *this, nullptr);
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This any object.
     */
    basic_any & operator=(const basic_any &other) {
        reset();
        other.vtable(operation::COPY, other, this);
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This any object.
     */
    basic_any & operator=(basic_any &&other) ENTT_NOEXCEPT {
        std::exchange(vtable, other.vtable)(operation::DTOR, *this, nullptr);
        other.vtable(operation::MOVE, other, this);
        mode = other.mode;
        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This any object.
     */
    template<typename Type>
    [[deprecated("Use std::in_place_type<T &>, entt::make_any<T &>, emplace<Type &> or forward_as_any instead")]]
    basic_any & operator=(std::reference_wrapper<Type> value) ENTT_NOEXCEPT {
        emplace<Type &>(value.get());
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
     * @brief Returns the type of the contained object.
     * @return The type of the contained object, if any.
     */
    [[nodiscard]] type_info type() const ENTT_NOEXCEPT {
        type_info info{};
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
        std::exchange(vtable, &basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>)(operation::DTOR, *this, nullptr);
        mode = type_to_policy<Type>();
        initialize<Type>(std::forward<Args>(args)...);
    }

    /*! @brief Destroys contained object */
    void reset() {
        std::exchange(vtable, &basic_vtable<void>)(operation::DTOR, *this, nullptr);
        mode = policy::OWNER;
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
        const basic_any *trampoline = &other;
        return type() == other.type() && (vtable(operation::COMP, *this, &trampoline) || !other.data());
    }

    /**
     * @brief Aliasing constructor.
     * @return A wrapper that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_any as_ref() ENTT_NOEXCEPT {
        return basic_any{*this, (mode == policy::CREF ? policy::CREF : policy::REF)};
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const ENTT_NOEXCEPT {
        return basic_any{*this, policy::CREF};
    }

    /**
     * @brief Returns true if a wrapper owns its object, false otherwise.
     * @return True if the wrapper owns its object, false otherwise.
     */
    [[nodiscard]] bool owner() const ENTT_NOEXCEPT {
        return (mode == policy::OWNER);
    }

private:
    union { const void *instance; storage_type storage; };
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
    const auto * const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type any_cast(basic_any<Len, Align> &data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type any_cast(basic_any<Len, Align> &&data) ENTT_NOEXCEPT {
    // forces const on non-reference types to make them work also with wrappers for const references
    auto * const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(std::move(*instance));
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
const Type * any_cast(const basic_any<Len, Align> *data) ENTT_NOEXCEPT {
    return (data->type() == type_id<Type>() ? static_cast<const Type *>(data->data()) : nullptr);
}


/*! @copydoc any_cast */
template<typename Type, std::size_t Len, std::size_t Align>
Type * any_cast(basic_any<Len, Align> *data) ENTT_NOEXCEPT {
    // last attempt to make wrappers for const references return their values
    return (data->type() == type_id<Type>() ? static_cast<Type *>(static_cast<constness_as_t<basic_any<Len, Align>, Type> *>(data)->data()) : nullptr);
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
basic_any<Len, Align> make_any(Args &&... args) {
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


}


#endif
