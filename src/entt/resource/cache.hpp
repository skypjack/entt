#ifndef ENTT_RESOURCE_RESOURCE_CACHE_HPP
#define ENTT_RESOURCE_RESOURCE_CACHE_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../container/dense_map.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"
#include "loader.hpp"
#include "resource.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type, typename It>
class resource_cache_iterator final {
    template<typename, typename>
    friend class resource_cache_iterator;

public:
    using value_type = std::pair<id_type, resource<Type>>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr resource_cache_iterator() noexcept = default;

    constexpr resource_cache_iterator(const It iter) noexcept
        : it{iter} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr resource_cache_iterator(const resource_cache_iterator<std::remove_const_t<Type>, Other> &other) noexcept
        : it{other.it} {}

    constexpr resource_cache_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr resource_cache_iterator operator++(int) noexcept {
        resource_cache_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr resource_cache_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr resource_cache_iterator operator--(int) noexcept {
        resource_cache_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr resource_cache_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr resource_cache_iterator operator+(const difference_type value) const noexcept {
        resource_cache_iterator copy = *this;
        return (copy += value);
    }

    constexpr resource_cache_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr resource_cache_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return {it[value].first, resource<Type>{it[value].second}};
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return (*this)[0];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr std::ptrdiff_t operator-(const resource_cache_iterator<Lhs...> &, const resource_cache_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const resource_cache_iterator<Lhs...> &, const resource_cache_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator<(const resource_cache_iterator<Lhs...> &, const resource_cache_iterator<Rhs...> &) noexcept;

private:
    It it;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return rhs < lhs;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<=(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>=(const resource_cache_iterator<Lhs...> &lhs, const resource_cache_iterator<Rhs...> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Basic cache for resources of any type.
 * @tparam Type Type of resources managed by a cache.
 * @tparam Loader Type of loader used to create the resources.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Type, typename Loader, typename Allocator>
class resource_cache {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type>, "Invalid value type");
    using container_allocator = typename alloc_traits::template rebind_alloc<std::pair<const id_type, typename Loader::result_type>>;
    using container_type = dense_map<id_type, typename Loader::result_type, identity, std::equal_to<id_type>, container_allocator>;

public:
    /*! @brief Resource type. */
    using value_type = Type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Loader type. */
    using loader_type = Loader;
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Input iterator type. */
    using iterator = internal::resource_cache_iterator<Type, typename container_type::iterator>;
    /*! @brief Constant input iterator type. */
    using const_iterator = internal::resource_cache_iterator<const Type, typename container_type::const_iterator>;

    /*! @brief Default constructor. */
    resource_cache()
        : resource_cache{loader_type{}} {}

    /**
     * @brief Constructs an empty cache with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit resource_cache(const allocator_type &allocator)
        : resource_cache{loader_type{}, allocator} {}

    /**
     * @brief Constructs an empty cache with a given allocator and loader.
     * @param callable The loader to use.
     * @param allocator The allocator to use.
     */
    explicit resource_cache(const loader_type &callable, const allocator_type &allocator = allocator_type{})
        : pool{container_type{allocator}, callable} {}

    /*! @brief Default copy constructor. */
    resource_cache(const resource_cache &) = default;

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    resource_cache(const resource_cache &other, const allocator_type &allocator)
        : pool{std::piecewise_construct, std::forward_as_tuple(other.pool.first(), allocator), std::forward_as_tuple(other.pool.second())} {}

    /*! @brief Default move constructor. */
    resource_cache(resource_cache &&) = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    resource_cache(resource_cache &&other, const allocator_type &allocator)
        : pool{std::piecewise_construct, std::forward_as_tuple(std::move(other.pool.first()), allocator), std::forward_as_tuple(std::move(other.pool.second()))} {}

    /**
     * @brief Default copy assignment operator.
     * @return This cache.
     */
    resource_cache &operator=(const resource_cache &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This cache.
     */
    resource_cache &operator=(resource_cache &&) = default;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return pool.first().get_allocator();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the cache is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal cache.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return pool.first().begin();
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        return pool.first().begin();
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last instance of the
     * internal cache.
     */
    [[nodiscard]] const_iterator cend() const noexcept {
        return pool.first().end();
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return pool.first().end();
    }

    /**
     * @brief Returns true if a cache contains no resources, false otherwise.
     * @return True if the cache contains no resources, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return pool.first().empty();
    }

    /**
     * @brief Number of resources managed by a cache.
     * @return Number of resources currently stored.
     */
    [[nodiscard]] size_type size() const noexcept {
        return pool.first().size();
    }

    /*! @brief Clears a cache. */
    void clear() noexcept {
        pool.first().clear();
    }

    /**
     * @brief Loads a resource, if its identifier does not exist.
     *
     * Arguments are forwarded directly to the loader and _consumed_ only if the
     * resource doesn't already exist.
     *
     * @warning
     * If the resource isn't loaded correctly, the returned handle could be
     * invalid and any use of it will result in undefined behavior.
     *
     * @tparam Args Types of arguments to use to load the resource if required.
     * @param id Unique resource identifier.
     * @param args Arguments to use to load the resource if required.
     * @return A pair consisting of an iterator to the inserted element (or to
     * the element that prevented the insertion) and a bool denoting whether the
     * insertion took place.
     */
    template<typename... Args>
    std::pair<iterator, bool> load(const id_type id, Args &&...args) {
        if(auto it = pool.first().find(id); it != pool.first().end()) {
            return {it, false};
        }

        return pool.first().emplace(id, pool.second()(std::forward<Args>(args)...));
    }

    /**
     * @brief Force loads a resource, if its identifier does not exist.
     * @copydetails load
     */
    template<typename... Args>
    std::pair<iterator, bool> force_load(const id_type id, Args &&...args) {
        return {pool.first().insert_or_assign(id, pool.second()(std::forward<Args>(args)...)).first, true};
    }

    /**
     * @brief Returns a handle for a given resource identifier.
     *
     * @warning
     * There is no guarantee that the returned handle is valid.<br/>
     * If it is not, any use will result in indefinite behavior.
     *
     * @param id Unique resource identifier.
     * @return A handle for the given resource.
     */
    [[nodiscard]] resource<const value_type> operator[](const id_type id) const {
        if(auto it = pool.first().find(id); it != pool.first().cend()) {
            return resource<const value_type>{it->second};
        }

        return {};
    }

    /*! @copydoc operator[] */
    [[nodiscard]] resource<value_type> operator[](const id_type id) {
        if(auto it = pool.first().find(id); it != pool.first().end()) {
            return resource<value_type>{it->second};
        }

        return {};
    }

    /**
     * @brief Checks if a cache contains a given identifier.
     * @param id Unique resource identifier.
     * @return True if the cache contains the resource, false otherwise.
     */
    [[nodiscard]] bool contains(const id_type id) const {
        return pool.first().contains(id);
    }

    /**
     * @brief Removes an element from a given position.
     * @param pos An iterator to the element to remove.
     * @return An iterator following the removed element.
     */
    iterator erase(const_iterator pos) {
        const auto it = pool.first().begin();
        return pool.first().erase(it + (pos - const_iterator{it}));
    }

    /**
     * @brief Removes the given elements from a cache.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     * @return An iterator following the last removed element.
     */
    iterator erase(const_iterator first, const_iterator last) {
        const auto it = pool.first().begin();
        return pool.first().erase(it + (first - const_iterator{it}), it + (last - const_iterator{it}));
    }

    /**
     * @brief Removes the given elements from a cache.
     * @param id Unique resource identifier.
     * @return Number of resources erased (either 0 or 1).
     */
    size_type erase(const id_type id) {
        return pool.first().erase(id);
    }

    /**
     * @brief Returns the loader used to create resources.
     * @return The loader used to create resources.
     */
    [[nodiscard]] loader_type loader() const {
        return pool.second();
    }

private:
    compressed_pair<container_type, loader_type> pool;
};

} // namespace entt

#endif
