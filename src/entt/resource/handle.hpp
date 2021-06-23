#ifndef ENTT_RESOURCE_HANDLE_HPP
#define ENTT_RESOURCE_HANDLE_HPP


#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Shared resource handle.
 *
 * A shared resource handle is a small class that wraps a resource and keeps it
 * alive even if it's deleted from the cache. It can be either copied or
 * moved. A handle shares a reference to the same resource with all the other
 * handles constructed for the same identifier.<br/>
 * As a rule of thumb, resources should never be copied nor moved. Handles are
 * the way to go to keep references to them.
 *
 * @tparam Resource Type of resource managed by a handle.
 */
template<typename Resource>
class resource_handle {
    /*! @brief Resource handles are friends with each other. */
    template<typename>
    friend class resource_handle;

public:
    /*! @brief Default constructor. */
    resource_handle() ENTT_NOEXCEPT = default;

    /**
     * @brief Creates a handle from a shared pointer, namely a resource.
     * @param res A pointer to a properly initialized resource.
     */
    resource_handle(std::shared_ptr<Resource> res) ENTT_NOEXCEPT
        : resource{std::move(res)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    resource_handle(const resource_handle<Resource> &other) ENTT_NOEXCEPT = default;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    resource_handle(resource_handle<Resource> &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Copy constructs a handle which shares ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     */
    template<typename Other, typename = std::enable_if_t<!std::is_same_v<Other, Resource> && std::is_base_of_v<Resource, Other>>>
    resource_handle(const resource_handle<Other> &other) ENTT_NOEXCEPT
        : resource{other.resource}
    {}

    /**
     * @brief Move constructs a handle which takes ownership of the resource.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     */
    template<typename Other, typename = std::enable_if_t<!std::is_same_v<Other, Resource> && std::is_base_of_v<Resource, Other>>>
    resource_handle(resource_handle<Other> &&other) ENTT_NOEXCEPT
        : resource{std::move(other.resource)}
    {}

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This resource handle.
     */
    resource_handle & operator=(const resource_handle<Resource> &other) ENTT_NOEXCEPT = default;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This resource handle.
     */
    resource_handle & operator=(resource_handle<Resource> &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Copy assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to copy from.
     * @return This resource handle.
     */
    template<typename Other>
    std::enable_if_t<!std::is_same_v<Other, Resource> && std::is_base_of_v<Resource, Other>, resource_handle &>
    operator=(const resource_handle<Other> &other) ENTT_NOEXCEPT {
        resource = other.resource;
        return *this;
    }

    /**
     * @brief Move assignment operator from foreign handle.
     * @tparam Other Type of resource managed by the received handle.
     * @param other The handle to move from.
     * @return This resource handle.
     */
    template<typename Other>
    std::enable_if_t<!std::is_same_v<Other, Resource> && std::is_base_of_v<Resource, Other>, resource_handle &>
    operator=(resource_handle<Other> &&other) ENTT_NOEXCEPT {
        resource = std::move(other.resource);
        return *this;
    }

    /**
     * @brief Gets a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.
     *
     * @return A reference to the managed resource.
     */
    [[nodiscard]] const Resource & get() const ENTT_NOEXCEPT {
        ENTT_ASSERT(static_cast<bool>(resource), "Invalid resource");
        return *resource;
    }

    /*! @copydoc get */
    [[nodiscard]] Resource & get() ENTT_NOEXCEPT {
        return const_cast<Resource &>(std::as_const(*this).get());
    }

    /*! @copydoc get */
    [[nodiscard]] operator const Resource & () const ENTT_NOEXCEPT {
        return get();
    }

    /*! @copydoc get */
    [[nodiscard]] operator Resource & () ENTT_NOEXCEPT {
        return get();
    }

    /*! @copydoc get */
    [[nodiscard]] const Resource & operator *() const ENTT_NOEXCEPT {
        return get();
    }

    /*! @copydoc get */
    [[nodiscard]] Resource & operator *() ENTT_NOEXCEPT {
        return get();
    }

    /**
     * @brief Gets a pointer to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.
     *
     * @return A pointer to the managed resource or `nullptr` if the handle
     * contains no resource at all.
     */
    [[nodiscard]] const Resource * operator->() const ENTT_NOEXCEPT {
        return resource.get();
    }

    /*! @copydoc operator-> */
    [[nodiscard]] Resource * operator->() ENTT_NOEXCEPT {
        return const_cast<Resource *>(std::as_const(*this).operator->());
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(resource);
    }

private:
    std::shared_ptr<Resource> resource;
};


}


#endif
