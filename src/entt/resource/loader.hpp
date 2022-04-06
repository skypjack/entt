#ifndef ENTT_RESOURCE_LOADEr_HPP
#define ENTT_RESOURCE_LOADEr_HPP

#include <memory>
#include <utility>
#include "fwd.hpp"

namespace entt {

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
