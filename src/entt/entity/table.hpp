#ifndef ENTT_ENTITY_TABLE_HPP
#define ENTT_ENTITY_TABLE_HPP

#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

struct basic_common_table {
    using size_type = std::size_t;

    virtual void reserve(const size_type) = 0;
    [[nodiscard]] virtual size_type capacity() const noexcept = 0;
    virtual void shrink_to_fit() = 0;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Basic table implementation.
 *
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a table. Do not make assumption on the order in any case.
 *
 * @tparam Type Element types.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename... Type, typename Allocator>
class basic_table<type_list<Type...>, Allocator> {
    using alloc_traits = std::allocator_traits<Allocator>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;

    /*! @brief Default constructor. */
    basic_table()
        : basic_table{allocator_type{}} {
    }

    /**
     * @brief Constructs an empty table with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_table(const allocator_type &allocator)
        : alloc{allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_table(basic_table &&other) noexcept
        : alloc{std::move(other.alloc)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_table(basic_table &&other, const allocator_type &allocator) noexcept
        : alloc{allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a table is not allowed");
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_table &operator=(basic_table &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a table is not allowed");

        alloc = std::move(other.alloc)
    }

    /**
     * @brief Exchanges the contents with those of a given table.
     * @param other Table to exchange the content with.
     */
    void swap(basic_table &other) {
        using std::swap;
        swap(alloc, other.alloc);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return alloc;
    }

private:
    allocator_type alloc;
};

} // namespace entt

#endif
