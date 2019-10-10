#ifndef ENTT_RESOURCE_HANDLE_HPP
#define ENTT_RESOURCE_HANDLE_HPP


#include <memory>
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
class handle {
    /*! @brief Resource handles are friends of their caches. */
    friend struct cache<Resource>;

    handle(std::shared_ptr<Resource> res) ENTT_NOEXCEPT
        : resource{std::move(res)}
    {}

public:
    /*! @brief Default constructor. */
    handle() ENTT_NOEXCEPT = default;

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
        ENTT_ASSERT(static_cast<bool>(resource));
        return *resource;
    }

    /*! @copydoc get */
    Resource & get() ENTT_NOEXCEPT {
        return const_cast<Resource &>(std::as_const(*this).get());
    }

    /*! @copydoc get */
    operator const Resource & () const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    operator Resource & () ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    const Resource & operator *() const ENTT_NOEXCEPT { return get(); }

    /*! @copydoc get */
    Resource & operator *() ENTT_NOEXCEPT { return get(); }

    /**
     * @brief Gets a pointer to the managed resource.
     *
     * @warning
     * The behavior is undefined if the handle doesn't contain a resource.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * handle is empty.
     *
     * @return A pointer to the managed resource or `nullptr` if the handle
     * contains no resource at all.
     */
    const Resource * operator->() const ENTT_NOEXCEPT {
        ENTT_ASSERT(static_cast<bool>(resource));
        return resource.get();
    }

    /*! @copydoc operator-> */
    Resource * operator->() ENTT_NOEXCEPT {
        return const_cast<Resource *>(std::as_const(*this).operator->());
    }

    /**
     * @brief Returns true if a handle contains a resource, false otherwise.
     * @return True if the handle contains a resource, false otherwise.
     */
    explicit operator bool() const { return static_cast<bool>(resource); }

private:
    std::shared_ptr<Resource> resource;
};


}


#endif // ENTT_RESOURCE_HANDLE_HPP
