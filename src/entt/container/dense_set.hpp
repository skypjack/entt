#ifndef ENTT_CONTAINER_DENSE_SET_HPP
#define ENTT_CONTAINER_DENSE_SET_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename It>
class dense_set_iterator final {
    template<typename>
    friend class dense_set_iterator;

public:
    using value_type = typename It::value_type::second_type;
    using pointer = const value_type *;
    using reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    constexpr dense_set_iterator() noexcept
        : it{} {}

    constexpr dense_set_iterator(const It iter) noexcept
        : it{iter} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr dense_set_iterator(const dense_set_iterator<Other> &other) noexcept
        : it{other.it} {}

    constexpr dense_set_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr dense_set_iterator operator++(int) noexcept {
        const dense_set_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr dense_set_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr dense_set_iterator operator--(int) noexcept {
        const dense_set_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr dense_set_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr dense_set_iterator operator+(const difference_type value) const noexcept {
        dense_set_iterator copy = *this;
        return (copy += value);
    }

    constexpr dense_set_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr dense_set_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return it[value].second;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(operator[](0));
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    template<typename Lhs, typename Rhs>
    friend constexpr std::ptrdiff_t operator-(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator==(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator<(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

private:
    It it;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

template<typename It>
class dense_set_local_iterator final {
    template<typename>
    friend class dense_set_local_iterator;

public:
    using value_type = typename It::value_type::second_type;
    using pointer = const value_type *;
    using reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr dense_set_local_iterator() noexcept
        : it{},
          offset{} {}

    constexpr dense_set_local_iterator(It iter, const std::size_t pos) noexcept
        : it{iter},
          offset{pos} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr dense_set_local_iterator(const dense_set_local_iterator<Other> &other) noexcept
        : it{other.it},
          offset{other.offset} {}

    constexpr dense_set_local_iterator &operator++() noexcept {
        return offset = it[static_cast<typename It::difference_type>(offset)].first, *this;
    }

    constexpr dense_set_local_iterator operator++(int) noexcept {
        const dense_set_local_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(it[static_cast<typename It::difference_type>(offset)].second);
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr std::size_t index() const noexcept {
        return offset;
    }

private:
    It it;
    std::size_t offset;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const dense_set_local_iterator<Lhs> &lhs, const dense_set_local_iterator<Rhs> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const dense_set_local_iterator<Lhs> &lhs, const dense_set_local_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Associative container for unique objects of a given type.
 *
 * Internally, elements are organized into buckets. Which bucket an element is
 * placed into depends entirely on its hash. Elements with the same hash code
 * appear in the same bucket.
 *
 * @tparam Type Value type of the associative container.
 * @tparam Hash Type of function to use to hash the values.
 * @tparam KeyEqual Type of function to use to compare the values for equality.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Type, typename Hash, typename KeyEqual, typename Allocator>
class dense_set {
    static constexpr float default_threshold = 0.875f;
    static constexpr std::size_t minimum_capacity = 8u;
    static constexpr std::size_t placeholder_position = (std::numeric_limits<std::size_t>::max)();

    using node_type = std::pair<std::size_t, Type>;
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type>, "Invalid value type");
    using sparse_container_type = std::vector<std::size_t, typename alloc_traits::template rebind_alloc<std::size_t>>;
    using packed_container_type = std::vector<node_type, typename alloc_traits::template rebind_alloc<node_type>>;

    template<typename Other>
    [[nodiscard]] std::size_t value_to_bucket(const Other &value) const noexcept {
        return fast_mod(static_cast<size_type>(sparse.second()(value)), bucket_count());
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, std::size_t bucket) {
        for(auto it = begin(bucket), last = end(bucket); it != last; ++it) {
            if(packed.second()(*it, value)) {
                return begin() + static_cast<difference_type>(it.index());
            }
        }

        return end();
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, std::size_t bucket) const {
        for(auto it = cbegin(bucket), last = cend(bucket); it != last; ++it) {
            if(packed.second()(*it, value)) {
                return cbegin() + static_cast<difference_type>(it.index());
            }
        }

        return cend();
    }

    template<typename Other>
    [[nodiscard]] auto insert_or_do_nothing(Other &&value) {
        const auto index = value_to_bucket(value);

        if(auto it = constrained_find(value, index); it != end()) {
            return std::make_pair(it, false);
        }

        packed.first().emplace_back(sparse.first()[index], std::forward<Other>(value));
        sparse.first()[index] = packed.first().size() - 1u;
        rehash_if_required();

        return std::make_pair(--end(), true);
    }

    void move_and_pop(const std::size_t pos) {
        if(const auto last = size() - 1u; pos != last) {
            size_type *curr = &sparse.first()[value_to_bucket(packed.first().back().second)];
            packed.first()[pos] = std::move(packed.first().back());
            for(; *curr != last; curr = &packed.first()[*curr].first) {}
            *curr = pos;
        }

        packed.first().pop_back();
    }

    void rehash_if_required() {
        if(const auto bc = bucket_count(); size() > static_cast<size_type>(static_cast<float>(bc) * max_load_factor())) {
            rehash(bc * 2u);
        }
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Key type of the container. */
    using key_type = Type;
    /*! @brief Value type of the container. */
    using value_type = Type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Signed integer type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Type of function to use to hash the elements. */
    using hasher = Hash;
    /*! @brief Type of function to use to compare the elements for equality. */
    using key_equal = KeyEqual;
    /*! @brief Random access iterator type. */
    using iterator = internal::dense_set_iterator<typename packed_container_type::iterator>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = internal::dense_set_iterator<typename packed_container_type::const_iterator>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    /*! @brief Forward iterator type. */
    using local_iterator = internal::dense_set_local_iterator<typename packed_container_type::iterator>;
    /*! @brief Constant forward iterator type. */
    using const_local_iterator = internal::dense_set_local_iterator<typename packed_container_type::const_iterator>;

    /*! @brief Default constructor. */
    dense_set()
        : dense_set{minimum_capacity} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit dense_set(const allocator_type &allocator)
        : dense_set{minimum_capacity, hasher{}, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator and user
     * supplied minimal number of buckets.
     * @param cnt Minimal number of buckets.
     * @param allocator The allocator to use.
     */
    dense_set(const size_type cnt, const allocator_type &allocator)
        : dense_set{cnt, hasher{}, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator, hash
     * function and user supplied minimal number of buckets.
     * @param cnt Minimal number of buckets.
     * @param hash Hash function to use.
     * @param allocator The allocator to use.
     */
    dense_set(const size_type cnt, const hasher &hash, const allocator_type &allocator)
        : dense_set{cnt, hash, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator, hash
     * function, compare function and user supplied minimal number of buckets.
     * @param cnt Minimal number of buckets.
     * @param hash Hash function to use.
     * @param equal Compare function to use.
     * @param allocator The allocator to use.
     */
    explicit dense_set(const size_type cnt, const hasher &hash = hasher{}, const key_equal &equal = key_equal{}, const allocator_type &allocator = allocator_type{})
        : sparse{allocator, hash},
          packed{allocator, equal} {
        rehash(cnt);
    }

    /*! @brief Default copy constructor. */
    dense_set(const dense_set &) = default;

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    dense_set(const dense_set &other, const allocator_type &allocator)
        : sparse{std::piecewise_construct, std::forward_as_tuple(other.sparse.first(), allocator), std::forward_as_tuple(other.sparse.second())},
          packed{std::piecewise_construct, std::forward_as_tuple(other.packed.first(), allocator), std::forward_as_tuple(other.packed.second())},
          threshold{other.threshold} {}

    /*! @brief Default move constructor. */
    dense_set(dense_set &&) noexcept = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    dense_set(dense_set &&other, const allocator_type &allocator)
        : sparse{std::piecewise_construct, std::forward_as_tuple(std::move(other.sparse.first()), allocator), std::forward_as_tuple(std::move(other.sparse.second()))},
          packed{std::piecewise_construct, std::forward_as_tuple(std::move(other.packed.first()), allocator), std::forward_as_tuple(std::move(other.packed.second()))},
          threshold{other.threshold} {}

    /*! @brief Default destructor. */
    ~dense_set() = default;

    /**
     * @brief Default copy assignment operator.
     * @return This container.
     */
    dense_set &operator=(const dense_set &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This container.
     */
    dense_set &operator=(dense_set &&) noexcept = default;

    /**
     * @brief Exchanges the contents with those of a given container.
     * @param other Container to exchange the content with.
     */
    void swap(dense_set &other) noexcept {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(threshold, other.threshold);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return sparse.first().get_allocator();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the array is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return packed.first().begin();
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        return packed.first().begin();
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last instance of the
     * internal array.
     */
    [[nodiscard]] const_iterator cend() const noexcept {
        return packed.first().end();
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return packed.first().end();
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * If the array is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first instance of the reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(cend());
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(end());
    }

    /**
     * @brief Returns a reverse iterator to the end.
     * @return An iterator to the element following the last instance of the
     * reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(cbegin());
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(begin());
    }

    /**
     * @brief Checks whether a container is empty.
     * @return True if the container is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return packed.first().empty();
    }

    /**
     * @brief Returns the number of elements in a container.
     * @return Number of elements in a container.
     */
    [[nodiscard]] size_type size() const noexcept {
        return packed.first().size();
    }

    /**
     * @brief Returns the maximum possible number of elements.
     * @return Maximum possible number of elements.
     */
    [[nodiscard]] size_type max_size() const noexcept {
        return packed.first().max_size();
    }

    /*! @brief Clears the container. */
    void clear() noexcept {
        sparse.first().clear();
        packed.first().clear();
        rehash(0u);
    }

    /**
     * @brief Inserts an element into the container, if it does not exist.
     * @param value An element to insert into the container.
     * @return A pair consisting of an iterator to the inserted element (or to
     * the element that prevented the insertion) and a bool denoting whether the
     * insertion took place.
     */
    std::pair<iterator, bool> insert(const value_type &value) {
        return insert_or_do_nothing(value);
    }

    /*! @copydoc insert */
    std::pair<iterator, bool> insert(value_type &&value) {
        return insert_or_do_nothing(std::move(value));
    }

    /**
     * @brief Inserts elements into the container, if they do not exist.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     */
    template<typename It>
    void insert(It first, It last) {
        for(; first != last; ++first) {
            insert(*first);
        }
    }

    /**
     * @brief Constructs an element in-place, if it does not exist.
     *
     * The element is also constructed when the container already has the key,
     * in which case the newly constructed object is destroyed immediately.
     *
     * @tparam Args Types of arguments to forward to the constructor of the
     * element.
     * @param args Arguments to forward to the constructor of the element.
     * @return A pair consisting of an iterator to the inserted element (or to
     * the element that prevented the insertion) and a bool denoting whether the
     * insertion took place.
     */
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        if constexpr(((sizeof...(Args) == 1u) && ... && std::is_same_v<std::decay_t<Args>, value_type>)) {
            return insert_or_do_nothing(std::forward<Args>(args)...);
        } else {
            auto &node = packed.first().emplace_back(std::piecewise_construct, std::make_tuple(packed.first().size()), std::forward_as_tuple(std::forward<Args>(args)...));
            const auto index = value_to_bucket(node.second);

            if(auto it = constrained_find(node.second, index); it != end()) {
                packed.first().pop_back();
                return std::make_pair(it, false);
            }

            std::swap(node.first, sparse.first()[index]);
            rehash_if_required();

            return std::make_pair(--end(), true);
        }
    }

    /**
     * @brief Removes an element from a given position.
     * @param pos An iterator to the element to remove.
     * @return An iterator following the removed element.
     */
    iterator erase(const_iterator pos) {
        const auto diff = pos - cbegin();
        erase(*pos);
        return begin() + diff;
    }

    /**
     * @brief Removes the given elements from a container.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     * @return An iterator following the last removed element.
     */
    iterator erase(const_iterator first, const_iterator last) {
        const auto dist = first - cbegin();

        for(auto from = last - cbegin(); from != dist; --from) {
            erase(packed.first()[static_cast<size_type>(from) - 1u].second);
        }

        return (begin() + dist);
    }

    /**
     * @brief Removes the element associated with a given value.
     * @param value Value of an element to remove.
     * @return Number of elements removed (either 0 or 1).
     */
    size_type erase(const value_type &value) {
        for(size_type *curr = &sparse.first()[value_to_bucket(value)]; *curr != placeholder_position; curr = &packed.first()[*curr].first) {
            if(packed.second()(packed.first()[*curr].second, value)) {
                const auto index = *curr;
                *curr = packed.first()[*curr].first;
                move_and_pop(index);
                return 1u;
            }
        }

        return 0u;
    }

    /**
     * @brief Returns the number of elements matching a value (either 1 or 0).
     * @param key Key value of an element to search for.
     * @return Number of elements matching the key (either 1 or 0).
     */
    [[nodiscard]] size_type count(const value_type &key) const {
        return find(key) != end();
    }

    /**
     * @brief Returns the number of elements matching a key (either 1 or 0).
     * @tparam Other Type of the key value of an element to search for.
     * @param key Key value of an element to search for.
     * @return Number of elements matching the key (either 1 or 0).
     */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, size_type>>
    count(const Other &key) const {
        return find(key) != end();
    }

    /**
     * @brief Finds an element with a given value.
     * @param value Value of an element to search for.
     * @return An iterator to an element with the given value. If no such
     * element is found, a past-the-end iterator is returned.
     */
    [[nodiscard]] iterator find(const value_type &value) {
        return constrained_find(value, value_to_bucket(value));
    }

    /*! @copydoc find */
    [[nodiscard]] const_iterator find(const value_type &value) const {
        return constrained_find(value, value_to_bucket(value));
    }

    /**
     * @brief Finds an element that compares _equivalent_ to a given value.
     * @tparam Other Type of an element to search for.
     * @param value Value of an element to search for.
     * @return An iterator to an element with the given value. If no such
     * element is found, a past-the-end iterator is returned.
     */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, iterator>>
    find(const Other &value) {
        return constrained_find(value, value_to_bucket(value));
    }

    /*! @copydoc find */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, const_iterator>>
    find(const Other &value) const {
        return constrained_find(value, value_to_bucket(value));
    }

    /**
     * @brief Returns a range containing all elements with a given value.
     * @param value Value of an element to search for.
     * @return A pair of iterators pointing to the first element and past the
     * last element of the range.
     */
    [[nodiscard]] std::pair<iterator, iterator> equal_range(const value_type &value) {
        const auto it = find(value);
        return {it, it + !(it == end())};
    }

    /*! @copydoc equal_range */
    [[nodiscard]] std::pair<const_iterator, const_iterator> equal_range(const value_type &value) const {
        const auto it = find(value);
        return {it, it + !(it == cend())};
    }

    /**
     * @brief Returns a range containing all elements that compare _equivalent_
     * to a given value.
     * @tparam Other Type of an element to search for.
     * @param value Value of an element to search for.
     * @return A pair of iterators pointing to the first element and past the
     * last element of the range.
     */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, std::pair<iterator, iterator>>>
    equal_range(const Other &value) {
        const auto it = find(value);
        return {it, it + !(it == end())};
    }

    /*! @copydoc equal_range */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, std::pair<const_iterator, const_iterator>>>
    equal_range(const Other &value) const {
        const auto it = find(value);
        return {it, it + !(it == cend())};
    }

    /**
     * @brief Checks if the container contains an element with a given value.
     * @param value Value of an element to search for.
     * @return True if there is such an element, false otherwise.
     */
    [[nodiscard]] bool contains(const value_type &value) const {
        return (find(value) != cend());
    }

    /**
     * @brief Checks if the container contains an element that compares
     * _equivalent_ to a given value.
     * @tparam Other Type of an element to search for.
     * @param value Value of an element to search for.
     * @return True if there is such an element, false otherwise.
     */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, bool>>
    contains(const Other &value) const {
        return (find(value) != cend());
    }

    /**
     * @brief Returns an iterator to the beginning of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the beginning of the given bucket.
     */
    [[nodiscard]] const_local_iterator cbegin(const size_type index) const {
        return {packed.first().begin(), sparse.first()[index]};
    }

    /**
     * @brief Returns an iterator to the beginning of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the beginning of the given bucket.
     */
    [[nodiscard]] const_local_iterator begin(const size_type index) const {
        return cbegin(index);
    }

    /**
     * @brief Returns an iterator to the beginning of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the beginning of the given bucket.
     */
    [[nodiscard]] local_iterator begin(const size_type index) {
        return {packed.first().begin(), sparse.first()[index]};
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] const_local_iterator cend([[maybe_unused]] const size_type index) const {
        return {packed.first().begin(), placeholder_position};
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] const_local_iterator end(const size_type index) const {
        return cend(index);
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] local_iterator end([[maybe_unused]] const size_type index) {
        return {packed.first().begin(), placeholder_position};
    }

    /**
     * @brief Returns the number of buckets.
     * @return The number of buckets.
     */
    [[nodiscard]] size_type bucket_count() const {
        return sparse.first().size();
    }

    /**
     * @brief Returns the maximum number of buckets.
     * @return The maximum number of buckets.
     */
    [[nodiscard]] size_type max_bucket_count() const {
        return sparse.first().max_size();
    }

    /**
     * @brief Returns the number of elements in a given bucket.
     * @param index The index of the bucket to examine.
     * @return The number of elements in the given bucket.
     */
    [[nodiscard]] size_type bucket_size(const size_type index) const {
        return static_cast<size_type>(std::distance(begin(index), end(index)));
    }

    /**
     * @brief Returns the bucket for a given element.
     * @param value The value of the element to examine.
     * @return The bucket for the given element.
     */
    [[nodiscard]] size_type bucket(const value_type &value) const {
        return value_to_bucket(value);
    }

    /**
     * @brief Returns the average number of elements per bucket.
     * @return The average number of elements per bucket.
     */
    [[nodiscard]] float load_factor() const {
        return static_cast<float>(size()) / static_cast<float>(bucket_count());
    }

    /**
     * @brief Returns the maximum average number of elements per bucket.
     * @return The maximum average number of elements per bucket.
     */
    [[nodiscard]] float max_load_factor() const {
        return threshold;
    }

    /**
     * @brief Sets the desired maximum average number of elements per bucket.
     * @param value A desired maximum average number of elements per bucket.
     */
    void max_load_factor(const float value) {
        ENTT_ASSERT(value > 0.f, "Invalid load factor");
        threshold = value;
        rehash(0u);
    }

    /**
     * @brief Reserves at least the specified number of buckets and regenerates
     * the hash table.
     * @param cnt New number of buckets.
     */
    void rehash(const size_type cnt) {
        auto value = cnt > minimum_capacity ? cnt : minimum_capacity;
        const auto cap = static_cast<size_type>(static_cast<float>(size()) / max_load_factor());
        value = value > cap ? value : cap;

        if(const auto sz = next_power_of_two(value); sz != bucket_count()) {
            sparse.first().resize(sz);

            for(auto &&elem: sparse.first()) {
                elem = placeholder_position;
            }

            for(size_type pos{}, last = size(); pos < last; ++pos) {
                const auto index = value_to_bucket(packed.first()[pos].second);
                packed.first()[pos].first = std::exchange(sparse.first()[index], pos);
            }
        }
    }

    /**
     * @brief Reserves space for at least the specified number of elements and
     * regenerates the hash table.
     * @param cnt New number of elements.
     */
    void reserve(const size_type cnt) {
        packed.first().reserve(cnt);
        rehash(static_cast<size_type>(std::ceil(static_cast<float>(cnt) / max_load_factor())));
    }

    /**
     * @brief Returns the function used to hash the elements.
     * @return The function used to hash the elements.
     */
    [[nodiscard]] hasher hash_function() const {
        return sparse.second();
    }

    /**
     * @brief Returns the function used to compare elements for equality.
     * @return The function used to compare elements for equality.
     */
    [[nodiscard]] key_equal key_eq() const {
        return packed.second();
    }

private:
    compressed_pair<sparse_container_type, hasher> sparse;
    compressed_pair<packed_container_type, key_equal> packed;
    float threshold{default_threshold};
};

} // namespace entt

#endif
