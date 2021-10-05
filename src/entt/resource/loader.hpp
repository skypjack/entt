#ifndef ENTT_RESOURCE_LOADER_HPP
#define ENTT_RESOURCE_LOADER_HPP

#include "fwd.hpp"
#include "handle.hpp"

namespace entt {

/**
 * @brief Base class for resource loaders.
 *
 * Resource loaders must inherit from this class and stay true to the CRTP
 * idiom. Moreover, a resource loader must expose a public, const member
 * function named `load` that accepts a variable number of arguments and returns
 * a handle to the resource just created.<br/>
 * As an example:
 *
 * @code{.cpp}
 * struct my_resource {};
 *
 * struct my_loader: entt::resource_loader<my_loader, my_resource> {
 *     resource_handle<my_resource> load(int value) const {
 *         // use the integer value somehow
 *         return std::make_shared<my_resource>();
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
class resource_loader {
    /*! @brief Resource loaders are friends of their caches. */
    template<typename Other>
    friend class resource_cache;

    /**
     * @brief Loads the resource and returns it.
     * @tparam Args Types of arguments for the loader.
     * @param args Arguments for the loader.
     * @return The resource just loaded or an empty pointer in case of errors.
     */
    template<typename... Args>
    [[nodiscard]] resource_handle<Resource> get(Args &&...args) const {
        return static_cast<const Loader *>(this)->load(std::forward<Args>(args)...);
    }
};

} // namespace entt

#endif
