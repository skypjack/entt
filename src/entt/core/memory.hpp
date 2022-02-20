#ifndef ENTT_CORE_MEMORY_HPP
#define ENTT_CORE_MEMORY_HPP

#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"

namespace entt {

/**
 * @brief Unwraps fancy pointers, does nothing otherwise (waiting for C++20).
 * @tparam Type Pointer type.
 * @param ptr Fancy or raw pointer.
 * @return A raw pointer that represents the address of the original pointer.
 */
template<typename Type>
[[nodiscard]] constexpr auto to_address(Type &&ptr) ENTT_NOEXCEPT {
    if constexpr(std::is_pointer_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
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
constexpr void propagate_on_container_copy_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) ENTT_NOEXCEPT {
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
constexpr void propagate_on_container_move_assignment([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) ENTT_NOEXCEPT {
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
constexpr void propagate_on_container_swap([[maybe_unused]] Allocator &lhs, [[maybe_unused]] Allocator &rhs) ENTT_NOEXCEPT {
    ENTT_ASSERT(std::allocator_traits<Allocator>::propagate_on_container_swap::value || lhs == rhs, "Cannot swap the containers");

    if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
        using std::swap;
        swap(lhs, rhs);
    }
}

/**
 * @brief Checks whether a value is a power of two or not.
 * @param value A value that may or may not be a power of two.
 * @return True if the value is a power of two, false otherwise.
 */
[[nodiscard]] inline constexpr bool is_power_of_two(const std::size_t value) ENTT_NOEXCEPT {
    return value && ((value & (value - 1)) == 0);
}

/**
 * @brief Computes the smallest power of two greater than or equal to a value.
 * @param value The value to use.
 * @return The smallest power of two greater than or equal to the given value.
 */
[[nodiscard]] inline constexpr std::size_t next_power_of_two(const std::size_t value) ENTT_NOEXCEPT {
    ENTT_ASSERT(value < (std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)), "Numeric limits exceeded");
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
[[nodiscard]] inline constexpr std::size_t fast_mod(const std::size_t value, const std::size_t mod) ENTT_NOEXCEPT {
    ENTT_ASSERT(is_power_of_two(mod), "Value must be a power of two");
    return value & (mod - 1u);
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
    allocation_deleter(const allocator_type &alloc)
        : Allocator{alloc} {}

    /**
     * @brief Destroys the pointed object and deallocates its memory.
     * @param ptr A valid pointer to an object of the given type.
     */
    void operator()(pointer ptr) {
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
auto allocate_unique(Allocator &allocator, Args &&...args) {
    static_assert(!std::is_array_v<Type>, "Array types are not supported");

    using alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Type>;
    using alloc_traits = typename std::allocator_traits<alloc>;

    alloc type_allocator{allocator};
    auto ptr = alloc_traits::allocate(type_allocator, 1u);

    ENTT_TRY {
        alloc_traits::construct(type_allocator, to_address(ptr), std::forward<Args>(args)...);
    }
    ENTT_CATCH {
        alloc_traits::deallocate(type_allocator, ptr, 1u);
        ENTT_THROW;
    }

    return std::unique_ptr<Type, allocation_deleter<alloc>>{ptr, type_allocator};
}

} // namespace entt

#endif
