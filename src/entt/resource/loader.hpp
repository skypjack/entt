#ifndef ENTT_RESOURCE_LOADER_HPP
#define ENTT_RESOURCE_LOADER_HPP


#include <memory>


namespace entt {


template<typename Resource>
class ResourceCache;


/**
 * @brief Base class for resource loaders.
 *
 * Resource loaders must inherit from this class and stay true to the CRTP
 * idiom. Moreover, a resource loader must expose a public, const member
 * function named `load` that accepts a variable number of arguments and returns
 * a shared pointer to the resource just created.<br/>
 * As an example:
 *
 * @code{.cpp}
 * struct MyResource {};
 *
 * struct MyLoader: entt::ResourceLoader<MyLoader, MyResource> {
 *     std::shared_ptr<MyResource> load(int) const {
 *         // use the integer value somehow
 *         return std::make_shared<MyResource>();
 *     }
 * };
 * @endcode
 *
 * In general, resource loaders should not have a state or retain data of any
 * type. They should let the cache manage their resources instead.
 *
 * @note
 * Base class and CRTP idiom aren't strictly required with the current
 * implementation. One could argue that a cache can easily work with loaders of
 * any type. However, future changes won't be breaking ones by forcing the use
 * of a base class today and that's why the model is already in its place.
 *
 * @tparam Loader Type of the derived class.
 * @tparam Resource Type of resource for which to use the loader.
 */
template<typename Loader, typename Resource>
class ResourceLoader {
    /*! @brief Resource loaders are friends of their caches. */
    friend class ResourceCache<Resource>;

    template<typename... Args>
    std::shared_ptr<Resource> get(Args &&... args) const {
        return static_cast<const Loader *>(this)->load(std::forward<Args>(args)...);
    }
};


}


#endif // ENTT_RESOURCE_LOADER_HPP
