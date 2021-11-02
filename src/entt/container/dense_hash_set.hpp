#ifndef ENTT_CONTAINER_DENSE_HASH_SET_HPP
#define ENTT_CONTAINER_DENSE_HASH_SET_HPP

#include <algorithm>
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
#include "../core/compressed_pair.hpp"
#include "../core/memory.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type>
struct dense_hash_set_node final {
    template<typename... Args>
    dense_hash_set_node(const std::size_t pos, Args &&...args)
        : next{pos},
          element{std::forward<Args>(args)...} {}

    std::size_t next;
    Type element;
};

template<typename It>
class dense_hash_set_iterator {
    friend dense_hash_set_iterator<const std::remove_pointer_t<It> *>;

    using iterator_traits = std::iterator_traits<decltype(std::addressof(std::as_const(std::declval<It>()->element)))>;

public:
    using iterator_type = It;
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    dense_hash_set_iterator() ENTT_NOEXCEPT = default;

    dense_hash_set_iterator(const iterator_type iter) ENTT_NOEXCEPT
        : it{iter} {}

    template<bool Const = std::is_const_v<std::remove_pointer_t<iterator_type>>, typename = std::enable_if_t<Const>>
    dense_hash_set_iterator(const dense_hash_set_iterator<std::remove_const_t<std::remove_pointer_t<iterator_type>> *> &other)
        : it{other.it} {}

    dense_hash_set_iterator &operator++() ENTT_NOEXCEPT {
        return ++it, *this;
    }

    dense_hash_set_iterator operator++(int) ENTT_NOEXCEPT {
        dense_hash_set_iterator orig = *this;
        return ++(*this), orig;
    }

    dense_hash_set_iterator &operator--() ENTT_NOEXCEPT {
        return --it, *this;
    }

    dense_hash_set_iterator operator--(int) ENTT_NOEXCEPT {
        dense_hash_set_iterator orig = *this;
        return operator--(), orig;
    }

    dense_hash_set_iterator &operator+=(const difference_type value) ENTT_NOEXCEPT {
        it += value;
        return *this;
    }

    dense_hash_set_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
        dense_hash_set_iterator copy = *this;
        return (copy += value);
    }

    dense_hash_set_iterator &operator-=(const difference_type value) ENTT_NOEXCEPT {
        return (*this += -value);
    }

    dense_hash_set_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
        return (*this + -value);
    }

    [[nodiscard]] reference operator[](const difference_type value) const {
        return it[value].element;
    }

    [[nodiscard]] pointer operator->() const {
        return std::addressof(it->element);
    }

    [[nodiscard]] reference operator*() const {
        return *operator->();
    }

    [[nodiscard]] iterator_type base() const ENTT_NOEXCEPT {
        return it;
    }

private:
    iterator_type it;
};

template<typename ILhs, typename IRhs>
[[nodiscard]] auto operator-(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.base() - rhs.base();
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator==(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.base() == rhs.base();
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator!=(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator<(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.base() < rhs.base();
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator>(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.base() > rhs.base();
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator<=(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs > rhs);
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator>=(const dense_hash_set_iterator<ILhs> &lhs, const dense_hash_set_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs < rhs);
}

template<typename It>
class dense_hash_set_local_iterator {
    friend dense_hash_set_local_iterator<const std::remove_pointer_t<It> *>;

    using iterator_traits = std::iterator_traits<decltype(std::addressof(std::as_const(std::declval<It>()->element)))>;

public:
    using iterator_type = It;
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::forward_iterator_tag;

    dense_hash_set_local_iterator() ENTT_NOEXCEPT = default;

    dense_hash_set_local_iterator(iterator_type iter, const std::size_t pos) ENTT_NOEXCEPT
        : it{iter},
          curr{pos} {}

    template<bool Const = std::is_const_v<std::remove_pointer_t<iterator_type>>, typename = std::enable_if_t<Const>>
    dense_hash_set_local_iterator(const dense_hash_set_local_iterator<std::remove_const_t<std::remove_pointer_t<iterator_type>> *> &other)
        : it{other.it},
          curr{other.curr} {}

    dense_hash_set_local_iterator &operator++() ENTT_NOEXCEPT {
        return curr = it[curr].next, *this;
    }

    dense_hash_set_local_iterator operator++(int) ENTT_NOEXCEPT {
        dense_hash_set_local_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] pointer operator->() const {
        return std::addressof(it[curr].element);
    }

    [[nodiscard]] reference operator*() const {
        return *operator->();
    }

    [[nodiscard]] iterator_type base() const ENTT_NOEXCEPT {
        return (it + curr);
    }

private:
    iterator_type it;
    std::size_t curr;
};

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator==(const dense_hash_set_local_iterator<ILhs> &lhs, const dense_hash_set_local_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.base() == rhs.base();
}

template<typename ILhs, typename IRhs>
[[nodiscard]] bool operator!=(const dense_hash_set_local_iterator<ILhs> &lhs, const dense_hash_set_local_iterator<IRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

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
class dense_hash_set final {
    static constexpr float default_threshold = 0.875f;
    static constexpr std::size_t minimum_capacity = 8u;

    using allocator_traits = std::allocator_traits<Allocator>;
    using alloc = typename allocator_traits::template rebind_alloc<Type>;
    using alloc_traits = typename std::allocator_traits<alloc>;

    using node_type = internal::dense_hash_set_node<Type>;
    using sparse_container_type = std::vector<std::size_t, typename alloc_traits::template rebind_alloc<std::size_t>>;
    using packed_container_type = std::vector<node_type, typename alloc_traits::template rebind_alloc<node_type>>;

    [[nodiscard]] std::size_t hash_to_bucket(const std::size_t hash) const ENTT_NOEXCEPT {
        return fast_mod(hash, bucket_count());
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, std::size_t bucket) {
        for(auto it = begin(bucket), last = end(bucket); it != last; ++it) {
            if(packed.second()(*it, value)) {
                return iterator{it.base()};
            }
        }

        return end();
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, std::size_t bucket) const {
        for(auto it = begin(bucket), last = end(bucket); it != last; ++it) {
            if(packed.second()(*it, value)) {
                return const_iterator{it.base()};
            }
        }

        return cend();
    }

    template<typename Arg>
    [[nodiscard]] auto get_or_emplace(Arg &&arg) {
        const auto hash = sparse.second()(arg);
        auto index = hash_to_bucket(hash);

        if(auto it = constrained_find(arg, index); it != end()) {
            return std::make_pair(it, false);
        }

        if(const auto count = size() + 1u; count > (bucket_count() * max_load_factor())) {
            rehash(bucket_count() * 2u);
            index = hash_to_bucket(hash);
        }

        packed.first().emplace_back(sparse.first()[index], std::forward<Arg>(arg));
        // update goes after emplace to enforce exception guarantees
        sparse.first()[index] = size() - 1u;

        return std::make_pair(--end(), true);
    }

    template<typename Other>
    bool do_erase(const Other &value) {
        for(size_type *curr = sparse.first().data() + bucket(value); *curr != std::numeric_limits<size_type>::max(); curr = &packed.first()[*curr].next) {
            if(packed.second()(packed.first()[*curr].element, value)) {
                const auto index = *curr;
                *curr = packed.first()[*curr].next;
                move_and_pop(index);
                return true;
            }
        }

        return false;
    }

    void move_and_pop(const std::size_t pos) {
        if(const auto last = size() - 1u; pos != last) {
            size_type *curr = sparse.first().data() + bucket(packed.first().back().element);
            for(; *curr != last; curr = &packed.first()[*curr].next) {}
            *curr = pos;

            // basic exception guarantees when value type has a throwing move constructor
            packed.first()[pos] = std::move(packed.first().back());
        }

        packed.first().pop_back();
    }

public:
    /*! @brief Key type of the container. */
    using key_type = Type;
    /*! @brief Value type of the container. */
    using value_type = Type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Type of function to use to hash the elements. */
    using hasher = Hash;
    /*! @brief Type of function to use to compare the elements for equality. */
    using key_equal = KeyEqual;
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Random access iterator type. */
    using iterator = internal::dense_hash_set_iterator<typename packed_container_type::pointer>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = internal::dense_hash_set_iterator<typename packed_container_type::const_pointer>;
    /*! @brief Forward iterator type. */
    using local_iterator = internal::dense_hash_set_local_iterator<typename packed_container_type::pointer>;
    /*! @brief Constant forward iterator type. */
    using const_local_iterator = internal::dense_hash_set_local_iterator<typename packed_container_type::const_pointer>;

    /*! @brief Default constructor. */
    dense_hash_set()
        : dense_hash_set(minimum_capacity) {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit dense_hash_set(const allocator_type &allocator)
        : dense_hash_set{minimum_capacity, hasher{}, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator and user
     * supplied minimal number of buckets.
     * @param bucket_count Minimal number of buckets.
     * @param allocator The allocator to use.
     */
    dense_hash_set(const size_type bucket_count, const allocator_type &allocator)
        : dense_hash_set{bucket_count, hasher{}, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator, hash
     * function and user supplied minimal number of buckets.
     * @param bucket_count Minimal number of buckets.
     * @param hash Hash function to use.
     * @param allocator The allocator to use.
     */
    dense_hash_set(const size_type bucket_count, const hasher &hash, const allocator_type &allocator)
        : dense_hash_set{bucket_count, hash, key_equal{}, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator, hash
     * function, compare function and user supplied minimal number of buckets.
     * @param bucket_count Minimal number of buckets.
     * @param hash Hash function to use.
     * @param equal Compare function to use.
     * @param allocator The allocator to use.
     */
    explicit dense_hash_set(const size_type bucket_count, const hasher &hash = hasher{}, const key_equal &equal = key_equal{}, const allocator_type &allocator = allocator_type())
        : sparse{allocator, hash},
          packed{allocator, equal},
          threshold{default_threshold} {
        rehash(bucket_count);
    }

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    dense_hash_set(const dense_hash_set &other)
        : dense_hash_set{other, alloc_traits::select_on_container_copy_construction(other.get_allocator())} {}

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    dense_hash_set(const dense_hash_set &other, const allocator_type &allocator)
        : sparse{sparse_container_type{other.sparse.first(), allocator}, other.sparse.second()},
          // cannot copy the container directly due to a nasty issue of apple clang :(
          packed{packed_container_type{other.packed.first().begin(), other.packed.first().end(), allocator}, other.packed.second()},
          threshold{other.threshold} {
    }

    /**
     * @brief Default move constructor.
     * @param other The instance to move from.
     */
    dense_hash_set(dense_hash_set &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    dense_hash_set(dense_hash_set &&other, const allocator_type &allocator) ENTT_NOEXCEPT
        : sparse{sparse_container_type{std::move(other.sparse.first()), allocator}, std::move(other.sparse.second())},
          // cannot move the container directly due to a nasty issue of apple clang :(
          packed{packed_container_type{std::make_move_iterator(other.packed.first().begin()), std::make_move_iterator(other.packed.first().end()), allocator}, std::move(other.packed.second())},
          threshold{other.threshold} {}

    /*! @brief Default destructor. */
    ~dense_hash_set() = default;

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This container.
     */
    dense_hash_set &operator=(const dense_hash_set &other) {
        threshold = other.threshold;
        sparse.first().clear();
        packed.first().clear();
        rehash(other.bucket_count());
        packed.first().reserve(other.packed.first().size());
        insert(other.cbegin(), other.cend());
        return *this;
    }

    /**
     * @brief Default move assignment operator.
     * @param other The instance to move from.
     * @return This container.
     */
    dense_hash_set &operator=(dense_hash_set &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const ENTT_NOEXCEPT {
        return sparse.first().get_allocator();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the internal array.
     * If the array is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const ENTT_NOEXCEPT {
        return packed.first().data();
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        return packed.first().data();
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the internal array. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the element following the last instance of the
     * internal array.
     */
    [[nodiscard]] const_iterator cend() const ENTT_NOEXCEPT {
        return packed.first().data() + size();
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return packed.first().data() + size();
    }

    /**
     * @brief Checks whether a container is empty.
     * @return True if the container is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return packed.first().empty();
    }

    /**
     * @brief Returns the number of elements in a container.
     * @return Number of elements in a container.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return packed.first().size();
    }

    /*! @brief Clears the container. */
    void clear() ENTT_NOEXCEPT {
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
        return emplace(value);
    }

    /*! @copydoc insert */
    std::pair<iterator, bool> insert(value_type &&value) {
        return emplace(std::move(value));
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
            emplace(*first);
        }
    }

    /**
     * @brief Constructs an element in-place, if it does not exist.
     * @tparam Args Types of arguments to forward to the constructor of the
     * element.
     * @param args Arguments to forward to the constructor of the element.
     * @return A pair consisting of an iterator to the inserted element (or to
     * the element that prevented the insertion) and a bool denoting whether the
     * insertion took place.
     */
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        if constexpr(((sizeof...(Args) == 1u) && ... && std::is_same_v<std::remove_const_t<std::remove_reference_t<Args>>, value_type>)) {
            return get_or_emplace(std::forward<Args>(args)...);
        } else {
            return get_or_emplace(value_type{std::forward<Args>(args)...});
        }
    }

    /**
     * @brief Removes an element from a given position.
     * @param pos An iterator to the element to remove.
     * @return An iterator following the removed element.
     */
    iterator erase(const_iterator pos) {
        const auto dist = std::distance(cbegin(), pos);
        erase(*pos);
        return begin() + dist;
    }

    /**
     * @brief Removes the given elements from a container.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     * @return An iterator following the last removed element.
     */
    iterator erase(const_iterator first, const_iterator last) {
        const auto dist = std::distance(cbegin(), first);

        for(auto rfirst = std::make_reverse_iterator(last), rlast = std::make_reverse_iterator(first); rfirst != rlast; ++rfirst) {
            erase(*rfirst);
        }

        return dist > static_cast<decltype(dist)>(size()) ? end() : (begin() + dist);
    }

    /**
     * @brief Removes the element associated with a given value.
     * @param value Value of an element to remove.
     * @return Number of elements removed (either 0 or 1).
     */
    size_type erase(const value_type &value) {
        return do_erase(value);
    }

    /**
     * @brief Exchanges the contents with those of a given container.
     * @param other Container to exchange the content with.
     */
    void swap(dense_hash_set &other) {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(threshold, other.threshold);
    }

    /**
     * @brief Finds an element with a given value.
     * @param value Value of an element to search for.
     * @return An iterator to an element with the given value. If no such
     * element is found, a past-the-end iterator is returned.
     */
    [[nodiscard]] iterator find(const value_type &value) {
        return constrained_find(value, bucket(value));
    }

    /*! @copydoc find */
    [[nodiscard]] const_iterator find(const value_type &value) const {
        return constrained_find(value, bucket(value));
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
        return constrained_find(value, bucket(value));
    }

    /*! @copydoc find */
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, const_iterator>>
    find(const Other &value) const {
        return constrained_find(value, bucket(value));
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
        return {packed.first().data(), sparse.first()[index]};
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
        return {packed.first().data(), sparse.first()[index]};
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] const_local_iterator cend([[maybe_unused]] const size_type index) const {
        return {packed.first().data(), std::numeric_limits<size_type>::max()};
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] const_local_iterator end([[maybe_unused]] const size_type index) const {
        return cend(index);
    }

    /**
     * @brief Returns an iterator to the end of a given bucket.
     * @param index An index of a bucket to access.
     * @return An iterator to the end of the given bucket.
     */
    [[nodiscard]] local_iterator end([[maybe_unused]] const size_type index) {
        return {packed.first().data(), std::numeric_limits<size_type>::max()};
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
        return hash_to_bucket(sparse.second()(value));
    }

    /**
     * @brief Returns the average number of elements per bucket.
     * @return The average number of elements per bucket.
     */
    [[nodiscard]] float load_factor() const {
        return size() / static_cast<float>(bucket_count());
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
     * @param count New number of buckets.
     */
    void rehash(const size_type count) {
        auto value = (std::max)(count, minimum_capacity);
        value = (std::max)(value, static_cast<size_type>(size() / max_load_factor()));

        if(const auto sz = next_power_of_two(value); sz != bucket_count()) {
            sparse.first().resize(sz);
            std::fill(sparse.first().begin(), sparse.first().end(), std::numeric_limits<size_type>::max());

            for(size_type pos{}, last = size(); pos < last; ++pos) {
                const auto index = bucket(packed.first()[pos].element);
                packed.first()[pos].next = std::exchange(sparse.first()[index], pos);
            }
        }
    }

    /**
     * @brief Reserves space for at least the specified number of elements and
     * regenerates the hash table.
     * @param count New number of elements.
     */
    void reserve(const size_type count) {
        packed.first().reserve(count);
        rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
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
    float threshold;
};

} // namespace entt

#endif
