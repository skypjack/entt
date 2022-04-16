#ifndef ENTT_RESOURCE_RESOURCE_HPP
#define ENTT_RESOURCE_RESOURCE_HPP

#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
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
    /*! @brief Resource handles are friends with each other. */
    template<typename>
    friend class resource;

    template<typename Other>
    static constexpr bool is_acceptable_v = !std::is_same_v<Type, Other> && std::is_constructible_v<Type &, Other &>;

public:
    /*! @brief Default constructor. */
    resource() ENTT_NOEXCEPT
        : value{} {}

    /**
     * @brief Creates a handle from a weak pointer, namely a resource.
     * @param res A weak pointer to a resource.
     */
    explicit resource(std::shared_ptr<Type> res) ENTT_NOEXCEPT
        : value{std::move(res)} {}

    /*! @brief Default copy constructor. */
    resource(const resource &) ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    resource(resource &&) ENTT_NOEXCEPT = default;

    /**
     * @brief Aliasing constructor.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle with which to share ownership information.
     * @param res Unrelated and unmanaged resources.
     */
    template<typename Other>
    resource(const resource<Other> &other, Type &res) ENTT_NOEXCEPT
        : value{other.value, std::addressof(res)} {}

    /**
     * @brief Copy constructs a handle which shares ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable_v<Other>>>
    resource(const resource<Other> &other) ENTT_NOEXCEPT
        : value{other.value} {}

    /**
     * @brief Move constructs a handle which takes ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     */
    template<typename Other, typename = std::enable_if_t<is_acceptable_v<Other>>>
    resource(resource<Other> &&other) ENTT_NOEXCEPT
        : value{std::move(other.value)} {}

    /**
     * @brief Default copy assignment operator.
     * @return This resource handle.
     */
    resource &operator=(const resource &) ENTT_NOEXCEPT = default;

    /**
     * @brief Default move assignment operator.
     * @return This resource handle.
     */
    resource &operator=(resource &&) ENTT_NOEXCEPT = default;

    /**
     * @brief Copy assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     * @return This resource handle.
     */
    template<typename Other>
    std::enable_if_t<is_acceptable_v<Other>, resource &>
    operator=(const resource<Other> &other) ENTT_NOEXCEPT {
        value = other.value;
        return *this;
    }

    /**
     * @brief Move assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     * @return This resource handle.
     */
    template<typename Other>
    std::enable_if_t<is_acceptable_v<Other>, resource &>
    operator=(resource<Other> &&other) ENTT_NOEXCEPT {
        value = std::move(other.value);
        return *this;
    }

    /**
     * @brief Returns a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.
     *
     * @return A reference to the managed resource.
     */
    [[nodiscard]] Type &operator*() const ENTT_NOEXCEPT {
        return *value;
    }

    /*! @copydoc operator* */
    [[nodiscard]] operator Type &() const ENTT_NOEXCEPT {
        return *value;
    }

    /**
     * @brief Returns a pointer to the managed resource.
     * @return A pointer to the managed resource.
     */
    [[nodiscard]] Type *operator->() const ENTT_NOEXCEPT {
        return value.get();
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(value);
    }

    /**
     * @brief Returns the number of handles pointing the same resource.
     * @return The number of handles pointing the same resource.
     */
    [[nodiscard]] long use_count() const ENTT_NOEXCEPT {
        return value.use_count();
    }

private:
    std::shared_ptr<Type> value;
};

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if both handles refer to the same resource, false otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator==(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return (std::addressof(*lhs) == std::addressof(*rhs));
}

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return False if both handles refer to the same registry, true otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator!=(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is less than the second, false otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator<(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return (std::addressof(*lhs) < std::addressof(*rhs));
}

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is greater than the second, false otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator>(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return (std::addressof(*lhs) > std::addressof(*rhs));
}

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is less than or equal to the second, false
 * otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator<=(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return !(lhs > rhs);
}

/**
 * @brief Compares two handles.
 * @tparam Res Type of resource managed by the first handle.
 * @tparam Other Type of resource managed by the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if the first handle is greater than or equal to the second,
 * false otherwise.
 */
template<typename Res, typename Other>
[[nodiscard]] bool operator>=(const resource<Res> &lhs, const resource<Other> &rhs) ENTT_NOEXCEPT {
    return !(lhs < rhs);
}

} // namespace entt

#endif
