#ifndef ENTT_RESOURCE_RESOURCE_HPP
#define ENTT_RESOURCE_RESOURCE_HPP

#include <memory>
#include <type_traits>
#include <utility>
#include "fwd.hpp"

namespace entt {

/**
 * @brief Basic resource handle.
 *
 * A handle wraps a resource and extends its lifetime. It also shares the same
 * resource with all other handles constructed from the same element.<br/>
 * As a rule of thumb, resources should never be copied nor moved. Handles are
 * the way to go to push references around.
 *
 * @tparam Type Type of resource managed by a handle.
 */
template<typename Type>
class resource {
    template<typename>
    friend class resource;

    template<typename Other>
    static constexpr bool is_acceptable = !std::is_same_v<Type, Other> && std::is_constructible_v<Type &, Other &>;

public:
    /*! @brief Resource type. */
    using element_type = Type;
    /*! @brief Handle type. */
    using handle_type = std::shared_ptr<element_type>;

    /*! @brief Default constructor. */
    resource() noexcept
        : value{} {}

    /**
     * @brief Creates a new resource handle.
     * @param res A handle to a resource.
     */
    explicit resource(handle_type res) noexcept
        : value{std::move(res)} {}

    /*! @brief Default copy constructor. */
    resource(const resource &) noexcept = default;

    /*! @brief Default move constructor. */
    resource(resource &&) noexcept = default;

    /**
     * @brief Aliasing constructor.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle with which to share ownership information.
     * @param res Unrelated and unmanaged resources.
     */
    template<typename Other>
    resource(const resource<Other> &other, element_type &res) noexcept
        : value{other.value, std::addressof(res)} {}

    /**
     * @brief Copy constructs a handle which shares ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource(const resource<Other> &other) noexcept
        : value{other.value} {}

    /**
     * @brief Move constructs a handle which takes ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource(resource<Other> &&other) noexcept
        : value{std::move(other.value)} {}

    /*! @brief Default destructor. */
    ~resource() = default;

    /**
     * @brief Default copy assignment operator.
     * @return This resource handle.
     */
    resource &operator=(const resource &) noexcept = default;

    /**
     * @brief Default move assignment operator.
     * @return This resource handle.
     */
    resource &operator=(resource &&) noexcept = default;

    /**
     * @brief Copy assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     * @return This resource handle.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource &operator=(const resource<Other> &other) noexcept {
        value = other.value;
        return *this;
    }

    /**
     * @brief Move assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     * @return This resource handle.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource &operator=(resource<Other> &&other) noexcept {
        value = std::move(other.value);
        return *this;
    }

    /**
     * @brief Exchanges the content with that of a given resource.
     * @param other Resource to exchange the content with.
     */
    void swap(resource &other) noexcept {
        using std::swap;
        swap(value, other.value);
    }

    /**
     * @brief Returns a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.
     *
     * @return A reference to the managed resource.
     */
    [[nodiscard]] element_type &operator*() const noexcept {
        return *value;
    }

    /*! @copydoc operator* */
    [[nodiscard]] operator element_type &() const noexcept {
        return *value;
    }

    /**
     * @brief Returns a pointer to the managed resource.
     * @return A pointer to the managed resource.
     */
    [[nodiscard]] element_type *operator->() const noexcept {
        return value.get();
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(value);
    }

    /*! @brief Releases the ownership of the managed resource. */
    void reset() {
        value.reset();
    }

    /**
     * @brief Replaces the managed resource.
     * @param other A handle to a resource.
     */
    void reset(handle_type other) {
        value = std::move(other);
    }

    /**
     * @brief Returns the underlying resource handle.
     * @return The underlying resource handle.
     */
    [[nodiscard]] const handle_type &handle() const noexcept {
        return value;
    }

private:
    handle_type value;
};

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if both handles refer to the same resource, false otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator==(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return (std::addressof(*lhs) == std::addressof(*rhs));
}

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return False if both handles refer to the same resource, true otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator!=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is less than the second, false otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator<(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return (std::addressof(*lhs) < std::addressof(*rhs));
}

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is greater than the second, false otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator>(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is less than or equal to the second, false
 * otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator<=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

/**
 * @brief Compares two handles.
 * @tparam Lhs Type of resource managed by the first handle.
 * @tparam Rhs Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is greater than or equal to the second,
 * false otherwise.
 */
template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator>=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace entt

#endif
