#ifndef ENTT_RESOURCE_CACHE_HPP
#define ENTT_RESOURCE_CACHE_HPP


#include <memory>
#include <utility>
#include <type_traits>
#include <unordered_map>
#include "../config/config.h"
#include "../core/hashed_string.hpp"
#include "handle.hpp"
#include "loader.hpp"


namespace entt {


/**
 * @brief Simple cache for resources of a given type.
 *
 * Minimal implementation of a cache for resources of a given type. It doesn't
 * offer much functionalities but it's suitable for small or medium sized
 * applications and can be freely inherited to add targeted functionalities for
 * large sized applications.
 *
 * @tparam Resource Type of resources managed by a cache.
 */
template<typename Resource>
class ResourceCache {
    using container_type = std::unordered_map<HashedString::hash_type, std::shared_ptr<Resource>>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename container_type::size_type;
    /*! @brief Type of resources managed by a cache. */
    using resource_type = HashedString;

    /*! @brief Default constructor. */
    ResourceCache() = default;

    /*! @brief Copying a cache isn't allowed. */
    ResourceCache(const ResourceCache &) ENTT_NOEXCEPT = delete;
    /*! @brief Default move constructor. */
    ResourceCache(ResourceCache &&) ENTT_NOEXCEPT = default;

    /*! @brief Copying a cache isn't allowed. @return This cache. */
    ResourceCache & operator=(const ResourceCache &) ENTT_NOEXCEPT = delete;
    /*! @brief Default move assignment operator. @return This cache. */
    ResourceCache & operator=(ResourceCache &&) ENTT_NOEXCEPT = default;

    /**
     * @brief Number of resources managed by a cache.
     * @return Number of resources currently stored.
     */
    size_type size() const ENTT_NOEXCEPT {
        return resources.size();
    }

    /**
     * @brief Returns true if a cache contains no resources, false otherwise.
     * @return True if the cache contains no resources, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return resources.empty();
    }

    /**
     * @brief Clears a cache and discards all its resources.
     *
     * Handles are not invalidated and the memory used by a resource isn't
     * freed as long as at least a handle keeps the resource itself alive.
     */
    void clear() ENTT_NOEXCEPT {
        resources.clear();
    }

    /**
     * @brief Loads the resource that corresponds to a given identifier.
     *
     * In case an identifier isn't already present in the cache, it loads its
     * resource and stores it aside for future uses. Arguments are forwarded
     * directly to the loader in order to construct properly the requested
     * resource.
     *
     * @note
     * If the identifier is already present in the cache, this function does
     * nothing and the arguments are simply discarded.
     *
     * @tparam Loader Type of loader to use to load the resource if required.
     * @tparam Args Types of arguments to use to load the resource if required.
     * @param id Unique resource identifier.
     * @param args Arguments to use to load the resource if required.
     * @return True if the resource is ready to use, false otherwise.
     */
    template<typename Loader, typename... Args>
    bool load(const resource_type id, Args &&... args) {
        static_assert(std::is_base_of<ResourceLoader<Loader, Resource>, Loader>::value, "!");

        bool loaded = true;

        if(resources.find(id) == resources.cend()) {
            std::shared_ptr<Resource> resource = Loader{}.get(std::forward<Args>(args)...);
            loaded = (static_cast<bool>(resource) ? (resources[id] = std::move(resource), loaded) : false);
        }

        return loaded;
    }

    /**
     * @brief Reloads a resource or loads it for the first time if not present.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * cache.discard(id);
     * cache.load(id, args...);
     * @endcode
     *
     * Arguments are forwarded directly to the loader in order to construct
     * properly the requested resource.
     *
     * @tparam Loader Type of loader to use to load the resource.
     * @tparam Args Types of arguments to use to load the resource.
     * @param id Unique resource identifier.
     * @param args Arguments to use to load the resource.
     * @return True if the resource is ready to use, false otherwise.
     */
    template<typename Loader, typename... Args>
    bool reload(const resource_type id, Args &&... args) {
        return (discard(id), load<Loader>(id, std::forward<Args>(args)...));
    }

    /**
     * @brief Creates a temporary handle for a resource.
     *
     * Arguments are forwarded directly to the loader in order to construct
     * properly the requested resource. The handle isn't stored aside and the
     * cache isn't in charge of the lifetime of the resource itself.
     *
     * @tparam Loader Type of loader to use to load the resource.
     * @tparam Args Types of arguments to use to load the resource.
     * @param args Arguments to use to load the resource.
     * @return A handle for the given resource.
     */
    template<typename Loader, typename... Args>
    ResourceHandle<Resource> temp(Args &&... args) const {
        return { Loader{}.get(std::forward<Args>(args)...) };
    }

    /**
     * @brief Creates a handle for a given resource identifier.
     *
     * A resource handle can be in a either valid or invalid state. In other
     * terms, a resource handle is properly initialized with a resource if the
     * cache contains the resource itself. Otherwise the returned handle is
     * uninitialized and accessing it results in undefined behavior.
     *
     * @sa ResourceHandle
     *
     * @param id Unique resource identifier.
     * @return A handle for the given resource.
     */
    ResourceHandle<Resource> handle(const resource_type id) const {
        auto it = resources.find(id);
        return { it == resources.end() ? nullptr : it->second };
    }

    /**
     * @brief Checks if a cache contains a given identifier.
     * @param id Unique resource identifier.
     * @return True if the cache contains the resource, false otherwise.
     */
    bool contains(const resource_type id) const ENTT_NOEXCEPT {
        return (resources.find(id) != resources.cend());
    }

    /**
     * @brief Discards the resource that corresponds to a given identifier.
     *
     * Handles are not invalidated and the memory used by the resource isn't
     * freed as long as at least a handle keeps the resource itself alive.
     *
     * @param id Unique resource identifier.
     */
    void discard(const resource_type id) ENTT_NOEXCEPT {
        auto it = resources.find(id);

        if(it != resources.end()) {
            resources.erase(it);
        }
    }

private:
    container_type resources;
};


}


#endif // ENTT_RESOURCE_CACHE_HPP
