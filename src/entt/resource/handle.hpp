#ifndef ENTT_RESOURCE_HANDLE_HPP
#define ENTT_RESOURCE_HANDLE_HPP


#include <memory>
#include <utility>
#include <cassert>
#include "../config/config.h"


namespace entt {


template<typename Resource>
class ResourceCache;


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
class ResourceHandle final {
    /*! @brief Resource handles are friends of their caches. */
    friend class ResourceCache<Resource>;

    ResourceHandle(std::shared_ptr<Resource> res) ENTT_NOEXCEPT
        : resource{std::move(res)}
    {}

public:
    /*! @brief Default copy constructor. */
    ResourceHandle(const ResourceHandle &) ENTT_NOEXCEPT = default;
    /*! @brief Default move constructor. */
    ResourceHandle(ResourceHandle &&) ENTT_NOEXCEPT = default;

    /*! @brief Default copy assignment operator. @return This handle. */
    ResourceHandle & operator=(const ResourceHandle &) ENTT_NOEXCEPT = default;
    /*! @brief Default move assignment operator. @return This handle. */
    ResourceHandle & operator=(ResourceHandle &&) ENTT_NOEXCEPT = default;

    /**
     * @brief Gets a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A reference to the managed resource.
     */
    const Resource & get() const ENTT_NOEXCEPT {
        assert(static_cast<bool>(resource));
        return *resource;
    }

    /**
     * @brief Casts a handle and gets a reference to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     */
    inline operator const Resource &() const ENTT_NOEXCEPT { return get(); }

    /**
     * @brief Dereferences a handle to obtain the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A reference to the managed resource.
     */
    inline const Resource & operator *() const ENTT_NOEXCEPT { return get(); }

    /**
     * @brief Gets a pointer to the managed resource from a handle .
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A pointer to the managed resource or `nullptr` if the handle
     * contains no resource at all.
     */
    inline const Resource * operator ->() const ENTT_NOEXCEPT {
        assert(static_cast<bool>(resource));
        return resource.get();
    }

    /**
     * @brief Returns true if the handle contains a resource, false otherwise.
     */
    explicit operator bool() const { return static_cast<bool>(resource); }

private:
    std::shared_ptr<Resource> resource;
};


}


#endif // ENTT_RESOURCE_HANDLE_HPP
