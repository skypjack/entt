#ifndef ENTT_CORE_MEMORY_HPP
#define ENTT_CORE_MEMORY_HPP

#include <cstddef>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"

namespace entt {

/**
 * @brief Checks whether a value is a power of two or not.
 * @param value A value that may or may not be a power of two.
 * @return True if the value is a power of two, false otherwise.
 */
[[nodiscard]] inline constexpr bool is_power_of_two(const std::size_t value) noexcept {
    return value && ((value & (value - 1)) == 0);
}

/**
 * @brief Computes the smallest power of two greater than or equal to a value.
 * @param value The value to use.
 * @return The smallest power of two greater than or equal to the given value.
 */
[[nodiscard]] inline constexpr std::size_t next_power_of_two(const std::size_t value) noexcept {
    ENTT_ASSERT_CONSTEXPR(value < (std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)), "Numeric limits exceeded");
    std::size_t curr = value - (value != 0u);

    for(int next = 1; next < std::numeric_limits<std::size_t>::digits; next = next * 2) {
        curr |= curr >> next;
    }

    return ++curr;
}

/**
 * @brief Fast module utility function (powers of two only).
 * @param value A value for which to calculate the modulus.
 * @param mod _Modulus_, it must be a power of two.
 * @return The common remainder.
 */
[[nodiscard]] inline constexpr std::size_t fast_mod(const std::size_t value, const std::size_t mod) noexcept {
    ENTT_ASSERT_CONSTEXPR(is_power_of_two(mod), "Value must be a power of two");
    return value & (mod - 1u);
}

/**
 * @brief Unwraps fancy pointers, does nothing otherwise (waiting for C++20).
 * @tparam Type Pointer type.
 * @param ptr Fancy or raw pointer.
 * @return A raw pointer that represents the address of the original pointer.
 */
template<typename Type>
[[nodiscard]] constexpr auto to_address(Type &&ptr) noexcept {
    if constexpr(std::is_pointer_v<std::decay_t<Type>>) {
        return ptr;
    } else {
        return to_address(std::forward<Type>(ptr).operator->());
    }
}

/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_copy_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) noexcept {
    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
        lhs = rhs;
    }
}

/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_move_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) noexcept {
    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
        lhs = std::move(rhs);
    }
}

/**
 * @brief Utility function to design allocation-aware containers.
 * @tparam Allocator Type of allocator.
 * @param lhs A valid allocator.
 * @param rhs Another valid allocator.
 */
template<typename Allocator>
constexpr void propagate_on_container_swap([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) noexcept {
    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
        using std::swap;
        swap(lhs, rhs);
    } else {
        ENTT_ASSERT_CONSTEXPR(lhs == rhs, "Cannot swap the containers");
    }
}

/**
 * @brief Deleter for allocator-aware unique pointers (waiting for C++20).
 * @tparam Args Types of arguments to use to construct the object.
 */
template<typename Allocator>
struct allocation_deleter: private Allocator {
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Pointer type. */
    using pointer = typename std::allocator_traits<Allocator>::pointer;

    /**
     * @brief Inherited constructors.
     * @param alloc The allocator to use.
     */
    constexpr allocation_deleter(const allocator_type &alloc) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
        : Allocator{alloc} {}

    /**
     * @brief Destroys the pointed object and deallocates its memory.
     * @param ptr A valid pointer to an object of the given type.
     */
    constexpr void operator()(pointer ptr) noexcept(std::is_nothrow_destructible_v<typename allocator_type::value_type>) {
        using alloc_traits = typename std::allocator_traits<Allocator>;
        alloc_traits::destroy(*this, to_address(ptr));
        alloc_traits::deallocate(*this, ptr, 1u);
    }
};

/**
 * @brief Allows `std::unique_ptr` to use allocators (waiting for C++20).
 * @tparam Type Type of object to allocate for and to construct.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 * @tparam Args Types of arguments to use to construct the object.
 * @param allocator The allocator to use.
 * @param args Parameters to use to construct the object.
 * @return A properly initialized unique pointer with a custom deleter.
 */
template<typename Type, typename Allocator, typename... Args>
ENTT_CONSTEXPR auto allocate_unique(Allocator &allocator, Args &&...args) {
    static_assert(!std::is_array_v<Type>, "Array types are not supported");

    using alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<Type>;
    using allocator_type = typename alloc_traits::allocator_type;

    allocator_type alloc{allocator};
    auto ptr = alloc_traits::allocate(alloc, 1u);

    ENTT_TRY {
        alloc_traits::construct(alloc, to_address(ptr), std::forward<Args>(args)...);
    }
    ENTT_CATCH {
        alloc_traits::deallocate(alloc, ptr, 1u);
        ENTT_THROW;
    }

    return std::unique_ptr<Type, allocation_deleter<allocator_type>>{ptr, alloc};
}

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type>
struct uses_allocator_construction {
    template<typename Allocator, typename... Params>
    static constexpr auto args([[maybe_unused]] const Allocator &allocator, Params &&...params) noexcept {
        if constexpr(!std::uses_allocator_v<Type, Allocator> && std::is_constructible_v<Type, Params...>) {
            return std::forward_as_tuple(std::forward<Params>(params)...);
        } else {
            static_assert(std::uses_allocator_v<Type, Allocator>, "Ill-formed request");

            if constexpr(std::is_constructible_v<Type, std::allocator_arg_t, const Allocator &, Params...>) {
                return std::tuple<std::allocator_arg_t, const Allocator &, Params &&...>{std::allocator_arg, allocator, std::forward<Params>(params)...};
            } else {
                static_assert(std::is_constructible_v<Type, Params..., const Allocator &>, "Ill-formed request");
                return std::forward_as_tuple(std::forward<Params>(params)..., allocator);
            }
        }
    }
};

template<typename Type, typename Other>
struct uses_allocator_construction<std::pair<Type, Other>> {
    using type = std::pair<Type, Other>;

    template<typename Allocator, typename First, typename Second>
    static constexpr auto args(const Allocator &allocator, std::piecewise_construct_t, First &&first, Second &&second) noexcept {
        return std::make_tuple(
            std::piecewise_construct,
            std::apply([&allocator](auto &&...curr) { return uses_allocator_construction<Type>::args(allocator, std::forward<decltype(curr)>(curr)...); }, std::forward<First>(first)),
            std::apply([&allocator](auto &&...curr) { return uses_allocator_construction<Other>::args(allocator, std::forward<decltype(curr)>(curr)...); }, std::forward<Second>(second)));
    }

    template<typename Allocator>
    static constexpr auto args(const Allocator &allocator) noexcept {
        return uses_allocator_construction<type>::args(allocator, std::piecewise_construct, std::tuple<>{}, std::tuple<>{});
    }

    template<typename Allocator, typename First, typename Second>
    static constexpr auto args(const Allocator &allocator, First &&first, Second &&second) noexcept {
        return uses_allocator_construction<type>::args(allocator, std::piecewise_construct, std::forward_as_tuple(std::forward<First>(first)), std::forward_as_tuple(std::forward<Second>(second)));
    }

    template<typename Allocator, typename First, typename Second>
    static constexpr auto args(const Allocator &allocator, const std::pair<First, Second> &value) noexcept {
        return uses_allocator_construction<type>::args(allocator, std::piecewise_construct, std::forward_as_tuple(value.first), std::forward_as_tuple(value.second));
    }

    template<typename Allocator, typename First, typename Second>
    static constexpr auto args(const Allocator &allocator, std::pair<First, Second> &&value) noexcept {
        return uses_allocator_construction<type>::args(allocator, std::piecewise_construct, std::forward_as_tuple(std::move(value.first)), std::forward_as_tuple(std::move(value.second)));
    }
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Uses-allocator construction utility (waiting for C++20).
 *
 * Primarily intended for internal use. Prepares the argument list needed to
 * create an object of a given type by means of uses-allocator construction.
 *
 * @tparam Type Type to return arguments for.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 * @tparam Args Types of arguments to use to construct the object.
 * @param allocator The allocator to use.
 * @param args Parameters to use to construct the object.
 * @return The arguments needed to create an object of the given type.
 */
template<typename Type, typename Allocator, typename... Args>
constexpr auto uses_allocator_construction_args(const Allocator &allocator, Args &&...args) noexcept {
    return internal::uses_allocator_construction<Type>::args(allocator, std::forward<Args>(args)...);
}

/**
 * @brief Uses-allocator construction utility (waiting for C++20).
 *
 * Primarily intended for internal use. Creates an object of a given type by
 * means of uses-allocator construction.
 *
 * @tparam Type Type of object to create.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 * @tparam Args Types of arguments to use to construct the object.
 * @param allocator The allocator to use.
 * @param args Parameters to use to construct the object.
 * @return A newly created object of the given type.
 */
template<typename Type, typename Allocator, typename... Args>
constexpr Type make_obj_using_allocator(const Allocator &allocator, Args &&...args) {
    return std::make_from_tuple<Type>(internal::uses_allocator_construction<Type>::args(allocator, std::forward<Args>(args)...));
}

/**
 * @brief Uses-allocator construction utility (waiting for C++20).
 *
 * Primarily intended for internal use. Creates an object of a given type by
 * means of uses-allocator construction at an uninitialized memory location.
 *
 * @tparam Type Type of object to create.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 * @tparam Args Types of arguments to use to construct the object.
 * @param value Memory location in which to place the object.
 * @param allocator The allocator to use.
 * @param args Parameters to use to construct the object.
 * @return A pointer to the newly created object of the given type.
 */
template<typename Type, typename Allocator, typename... Args>
constexpr Type *uninitialized_construct_using_allocator(Type *value, const Allocator &allocator, Args &&...args) {
    return std::apply([value](auto &&...curr) { return new(value) Type(std::forward<decltype(curr)>(curr)...); }, internal::uses_allocator_construction<Type>::args(allocator, std::forward<Args>(args)...));
}

} // namespace entt

#endif
