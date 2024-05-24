#ifndef ENTT_CONTAINER_TABLE_HPP
#define ENTT_CONTAINER_TABLE_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

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

    constexpr table_iterator() noexcept
        : it{} {}

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
 * @tparam Container Sequence container row types.
 */
template<typename... Container>
class basic_table {
    using container_type = std::tuple<Container...>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator = internal::table_iterator<typename Container::iterator...>;
    /*! @brief Constant input iterator type. */
    using const_iterator = internal::table_iterator<typename Container::const_iterator...>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = internal::table_iterator<typename Container::reverse_iterator...>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = internal::table_iterator<typename Container::const_reverse_iterator...>;

    /*! @brief Default constructor. */
    basic_table()
        : payload{} {
    }

    /**
     * @brief Copy constructs the underlying containers.
     * @param container The containers to copy from.
     */
    explicit basic_table(const Container &...container) noexcept
        : payload{container...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    /**
     * @brief Move constructs the underlying containers.
     * @param container The containers to move from.
     */
    explicit basic_table(Container &&...container) noexcept
        : payload{std::move(container)...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_table(const basic_table &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_table(basic_table &&other) noexcept
        : payload{std::move(other.payload)} {}

    /**
     * @brief Constructs the underlying containers using a given allocator.
     * @tparam Allocator Type of allocator.
     * @param allocator A valid allocator.
     */
    template<typename Allocator>
    explicit basic_table(const Allocator &allocator)
        : payload{Container{allocator}...} {}

    /**
     * @brief Copy constructs the underlying containers using a given allocator.
     * @tparam Allocator Type of allocator.
     * @param container The containers to copy from.
     * @param allocator A valid allocator.
     */
    template<class Allocator>
    basic_table(const Container &...container, const Allocator &allocator) noexcept
        : payload{Container{container, allocator}...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    /**
     * @brief Move constructs the underlying containers using a given allocator.
     * @tparam Allocator Type of allocator.
     * @param container The containers to move from.
     * @param allocator A valid allocator.
     */
    template<class Allocator>
    basic_table(Container &&...container, const Allocator &allocator) noexcept
        : payload{Container{std::move(container), allocator}...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    /**
     * @brief Allocator-extended move constructor.
     * @tparam Allocator Type of allocator.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    template<class Allocator>
    basic_table(basic_table &&other, const Allocator &allocator)
        : payload{Container{std::move(std::get<Container>(other.payload)), allocator}...} {}

    /*! @brief Default destructor. */
    ~basic_table() noexcept = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This container.
     */
    basic_table &operator=(const basic_table &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This container.
     */
    basic_table &operator=(basic_table &&other) noexcept {
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
     * @brief Increases the capacity of a table.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        (std::get<Container>(payload).reserve(cap), ...);
    }

    /**
     * @brief Returns the number of rows that a table has currently allocated
     * space for.
     * @return Capacity of the table.
     */
    [[nodiscard]] size_type capacity() const noexcept {
        return std::get<0>(payload).capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        (std::get<Container>(payload).shrink_to_fit(), ...);
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

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the table is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first row of the table.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return {std::get<Container>(payload).cbegin()...};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        return {std::get<Container>(payload).begin()...};
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last row of the table.
     */
    [[nodiscard]] const_iterator cend() const noexcept {
        return {std::get<Container>(payload).cend()...};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return {std::get<Container>(payload).end()...};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * If the table is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first row of the reversed table.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return {std::get<Container>(payload).crbegin()...};
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return {std::get<Container>(payload).rbegin()...};
    }

    /**
     * @brief Returns a reverse iterator to the end.
     * @return An iterator to the element following the last row of the reversed
     * table.
     */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return {std::get<Container>(payload).crend()...};
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() noexcept {
        return {std::get<Container>(payload).rend()...};
    }

    /**
     * @brief Appends a row to the end of a table.
     * @tparam Args Types of arguments to use to construct the row data.
     * @param args Parameters to use to construct the row data.
     * @return A reference to the newly created row data.
     */
    template<typename... Args>
    std::tuple<typename Container::value_type &...> emplace(Args &&...args) {
        if constexpr(sizeof...(Args) == 0u) {
            return std::forward_as_tuple(std::get<Container>(payload).emplace_back()...);
        } else {
            return std::forward_as_tuple(std::get<Container>(payload).emplace_back(std::forward<Args>(args))...);
        }
    }

    /**
     * @brief Removes a row from a table.
     * @param pos An iterator to the row to remove.
     * @return An iterator following the removed row.
     */
    iterator erase(const_iterator pos) {
        const auto diff = pos - begin();
        return {std::get<Container>(payload).erase(std::get<Container>(payload).begin() + diff)...};
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
    [[nodiscard]] std::tuple<const typename Container::value_type &...> operator[](const size_type pos) const {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<Container>(payload)[pos]...);
    }

    /*! @copydoc operator[] */
    [[nodiscard]] std::tuple<typename Container::value_type &...> operator[](const size_type pos) {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<Container>(payload)[pos]...);
    }

    /*! @brief Clears a table. */
    void clear() {
        (std::get<Container>(payload).clear(), ...);
    }

private:
    container_type payload;
};

} // namespace entt

/*! @cond TURN_OFF_DOXYGEN */
namespace std {

template<typename... Container, typename Allocator>
struct uses_allocator<entt::basic_table<Container...>, Allocator>
    : std::bool_constant<(std::uses_allocator_v<Container, Allocator> && ...)> {};

} // namespace std
/*! @endcond */

#endif
