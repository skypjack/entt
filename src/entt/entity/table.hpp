#ifndef ENTT_ENTITY_TABLE_HPP
#define ENTT_ENTITY_TABLE_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/compressed_pair.hpp"
#include "../core/iterator.hpp"
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

template<typename... It>
class table_iterator {
    template<typename...>
    friend class table_iterator;

public:
    using value_type = decltype(std::forward_as_tuple(*std::declval<It>()...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr table_iterator() noexcept = default;

    constexpr table_iterator(It... from) noexcept
        : it{from...} {}

    template<typename... Other, typename = std::enable_if_t<(std::is_constructible_v<It, Other> && ...)>>
    constexpr table_iterator(const table_iterator<Other...> &other) noexcept
        : table_iterator{std::get<Other>(other.it)...} {}

    constexpr table_iterator &operator++() noexcept {
        return (++std::get<It>(it), ...), *this;
    }

    constexpr table_iterator operator++(int) noexcept {
        table_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr table_iterator &operator--() noexcept {
        return (--std::get<It>(it), ...), *this;
    }

    constexpr table_iterator operator--(int) noexcept {
        table_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr table_iterator &operator+=(const difference_type value) noexcept {
        return ((std::get<It>(it) += value), ...), *this;
    }

    constexpr table_iterator operator+(const difference_type value) const noexcept {
        table_iterator copy = *this;
        return (copy += value);
    }

    constexpr table_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr table_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return std::forward_as_tuple(std::get<It>(it)[value]...);
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return {operator[](0)};
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr std::ptrdiff_t operator-(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator<(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

private:
    std::tuple<It...> it;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) - std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) == std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) < std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return rhs < lhs;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Basic table implementation.
 *
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a table. Do not make assumption on the order in any case.
 *
 * @tparam Row Element types.
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
    /*! @brief Input iterator type. */
    using iterator = internal::table_iterator<typename container_for<Row>::iterator...>;
    /*! @brief Constant input iterator type. */
    using const_iterator = internal::table_iterator<typename container_for<Row>::const_iterator...>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = internal::table_iterator<typename container_for<Row>::reverse_iterator...>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = internal::table_iterator<typename container_for<Row>::const_reverse_iterator...>;

    /*! @brief Default constructor. */
    basic_table()
        : basic_table{allocator_type{}} {
    }

    /**
     * @brief Constructs an empty table with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_table(const allocator_type &allocator)
        : payload{container_type{container_for<Row>{allocator}...}, allocator} {}

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
        : payload{container_type{container_for<Row>{std::move(std::get<container_for<Row>>(other.payload.first())), allocator}...}, allocator} {
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
        return payload.second();
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
        (std::get<container_for<Row>>(payload.first()).reserve(cap), ...);
    }

    /**
     * @brief Returns the number of rows that a table has currently allocated
     * space for.
     * @return Capacity of the table.
     */
    [[nodiscard]] size_type capacity() const noexcept override {
        return std::get<0>(payload.first()).capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
        (std::get<container_for<Row>>(payload.first()).shrink_to_fit(), ...);
    }

    /**
     * @brief Returns the number of rows in a table.
     * @return Number of rows.
     */
    [[nodiscard]] size_type size() const noexcept {
        return std::get<0>(payload.first()).size();
    }

    /**
     * @brief Checks whether a table is empty.
     * @return True if the table is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return std::get<0>(payload.first()).empty();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the table is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first row of the table.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return {std::get<container_for<Row>>(payload.first()).cbegin()...};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        return {std::get<container_for<Row>>(payload.first()).begin()...};
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last row of the table.
     */
    [[nodiscard]] const_iterator cend() const noexcept {
        return {std::get<container_for<Row>>(payload.first()).cend()...};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return {std::get<container_for<Row>>(payload.first()).end()...};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * If the table is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first row of the reversed table.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return {std::get<container_for<Row>>(payload.first()).crbegin()...};
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return {std::get<container_for<Row>>(payload.first()).rbegin()...};
    }

    /**
     * @brief Returns a reverse iterator to the end.
     * @return An iterator to the element following the last row of the reversed
     * table.
     */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return {std::get<container_for<Row>>(payload.first()).crend()...};
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() noexcept {
        return {std::get<container_for<Row>>(payload.first()).rend()...};
    }

    /**
     * @brief Appends a row to the end of a table.
     * @tparam Args Types of arguments to use to construct the row data.
     * @param args Parameters to use to construct the row data.
     * @return A reference to the newly created row data.
     */
    template<typename... Args>
    std::tuple<Row &...> emplace(Args &&...args) {
        return std::forward_as_tuple(std::get<container_for<Row>>(payload.first()).emplace_back(std::forward<Args>(args))...);
    }

    /**
     * @brief Removes a row from a table.
     * @param pos An iterator to the row to remove.
     * @return An iterator following the removed row.
     */
    iterator erase(const_iterator pos) {
        const auto diff = pos - begin();
        return {std::get<container_for<Row>>(payload.first()).erase(std::get<container_for<Row>>(payload.first()).begin() + diff)...};
    }

    /**
     * @brief Removes a row from a table.
     * @param pos Index of the row to remove.
     */
    void erase(const size_type pos) {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        erase(begin() + static_cast<typename const_iterator::difference_type>(pos));
    }

    /**
     * @brief Returns the row data at specified location.
     * @param pos The row for which to return the data.
     * @return The row data at specified location.
     */
    [[nodiscard]] std::tuple<const Row &...> operator[](const size_type pos) const {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<container_for<Row>>(payload.first())[pos]...);
    }

    /*! @copydoc operator[] */
    [[nodiscard]] std::tuple<Row &...> operator[](const size_type pos) {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<container_for<Row>>(payload.first())[pos]...);
    }

    /*! @brief Clears a table. */
    void clear() {
        (std::get<container_for<Row>>(payload.first()).clear(), ...);
    }

private:
    compressed_pair<container_type, allocator_type> payload;
};

} // namespace entt

#endif
