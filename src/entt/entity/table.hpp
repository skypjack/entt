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
template<typename... Row, typename Allocator>
class basic_table<type_list<Row...>, Allocator>: internal::basic_common_table {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(sizeof...(Row) != 0u, "Empty tables not allowed");

    template<typename Type>
    using container_for = std::vector<Type, typename alloc_traits::template rebind_alloc<Type>>;

    using container_type = std::tuple<container_for<Row>...>;
    using underlying_type = internal::basic_common_table;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Base type. */
    using base_type = underlying_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;

    /*! @brief Default constructor. */
    basic_table()
        : basic_table{allocator_type{}} {
    }

    /**
     * @brief Constructs an empty table with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_table(const allocator_type &allocator)
        : payload{container_for<Row>{allocator}...} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_table(basic_table &&other) noexcept
        : payload{std::move(other.payload)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_table(basic_table &&other, const allocator_type &allocator) noexcept
        : payload{std::move(other.payload), allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a table is not allowed");
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This table.
     */
    basic_table &operator=(basic_table &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a table is not allowed");

        payload = std::move(other.payload);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given table.
     * @param other Table to exchange the content with.
     */
    void swap(basic_table &other) {
        using std::swap;
        swap(payload, other.payload);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return std::get<0>(payload).get_allocator();
    }

    /**
     * @brief Increases the capacity of a table.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) override {
        (std::get<container_for<Row>>(payload).reserve(cap), ...);
    }

    /**
     * @brief Returns the number of rows that a table has currently allocated
     * space for.
     * @return Capacity of the table.
     */
    [[nodiscard]] size_type capacity() const noexcept override {
        return std::get<0>(payload).capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
        (std::get<container_for<Row>>(payload).shrink_to_fit(), ...);
    }

    /**
     * @brief Returns the number of rows in a table.
     * @return Number of rows.
     */
    [[nodiscard]] size_type size() const noexcept {
        return std::get<0>(payload).size();
    }

    /**
     * @brief Checks whether a table is empty.
     * @return True if the table is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return std::get<0>(payload).empty();
    }

    /*! @brief Clears a table. */
    void clear() {
        (std::get<container_for<Row>>(payload).clear(), ...);
    }

private:
    container_type payload;
};

} // namespace entt

#endif
