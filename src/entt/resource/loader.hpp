#ifndef ENTT_RESOURCE_LOADER_HPP
#define ENTT_RESOURCE_LOADER_HPP

#include "../config/module.h"

#ifndef ENTT_MODULE
#   include <memory>
#   include <utility>
#   include "fwd.hpp"
#endif // ENTT_MODULE

ENTT_MODULE_EXPORT namespace entt {

/**
 * @brief Transparent loader for shared resources.
 * @tparam Type Type of resources created by the loader.
 */
template<typename Type>
struct resource_loader {
    /*! @brief Result type. */
    using result_type = std::shared_ptr<Type>;

    /**
     * @brief Constructs a shared pointer to a resource from its arguments.
     * @tparam Args Types of arguments to use to construct the resource.
     * @param args Parameters to use to construct the resource.
     * @return A shared pointer to a resource of the given type.
     */
    template<typename... Args>
    result_type operator()(Args &&...args) const {
        return std::make_shared<Type>(std::forward<Args>(args)...);
    }
};

} // namespace entt

#endif
