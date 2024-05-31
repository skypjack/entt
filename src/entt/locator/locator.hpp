#ifndef ENTT_LOCATOR_LOCATOR_HPP
#define ENTT_LOCATOR_LOCATOR_HPP

#include <memory>
#include <utility>
#include "../config/config.h"

namespace entt {

/**
 * @brief Service locator, nothing more.
 *
 * A service locator is used to do what it promises: locate services.<br/>
 * Usually service locators are tightly bound to the services they expose and
 * thus it's hard to define a general purpose class to do that. This tiny class
 * tries to fill the gap and to get rid of the burden of defining a different
 * specific locator for each application.
 *
 * @note
 * Users shouldn't retain references to a service. The recommended way is to
 * retrieve the service implementation currently set each and every time the
 * need for it arises. The risk is to incur in unexpected behaviors otherwise.
 *
 * @tparam Service Service type.
 */
template<typename Service>
class locator final {
    class service_handle {
        friend class locator<Service>;
        std::shared_ptr<Service> value{};
    };

public:
    /*! @brief Service type. */
    using type = Service;
    /*! @brief Service node type. */
    using node_type = service_handle;

    /*! @brief Default constructor, deleted on purpose. */
    locator() = delete;

    /*! @brief Default copy constructor, deleted on purpose. */
    locator(const locator &) = delete;

    /*! @brief Default destructor, deleted on purpose. */
    ~locator() noexcept = delete;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This locator.
     */
    locator &operator=(const locator &) = delete;

    /**
     * @brief Checks whether a service locator contains a value.
     * @return True if the service locator contains a value, false otherwise.
     */
    [[nodiscard]] static bool has_value() noexcept {
        return (service != nullptr);
    }

    /**
     * @brief Returns a reference to a valid service, if any.
     *
     * @warning
     * Invoking this function can result in undefined behavior if the service
     * hasn't been set yet.
     *
     * @return A reference to the service currently set, if any.
     */
    [[nodiscard]] static Service &value() noexcept {
        ENTT_ASSERT(has_value(), "Service not available");
        return *service;
    }

    /**
     * @brief Returns a service if available or sets it from a fallback type.
     *
     * Arguments are used only if a service doesn't already exist. In all other
     * cases, they are discarded.
     *
     * @tparam Args Types of arguments to use to construct the fallback service.
     * @tparam Type Fallback service type.
     * @param args Parameters to use to construct the fallback service.
     * @return A reference to a valid service.
     */
    template<typename Type = Service, typename... Args>
    [[nodiscard]] static Service &value_or(Args &&...args) {
        return service ? *service : emplace<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Sets or replaces a service.
     * @tparam Type Service type.
     * @tparam Args Types of arguments to use to construct the service.
     * @param args Parameters to use to construct the service.
     * @return A reference to a valid service.
     */
    template<typename Type = Service, typename... Args>
    static Service &emplace(Args &&...args) {
        service = std::make_shared<Type>(std::forward<Args>(args)...);
        return *service;
    }

    /**
     * @brief Sets or replaces a service using a given allocator.
     * @tparam Type Service type.
     * @tparam Allocator Type of allocator used to manage memory and elements.
     * @tparam Args Types of arguments to use to construct the service.
     * @param alloc The allocator to use.
     * @param args Parameters to use to construct the service.
     * @return A reference to a valid service.
     */
    template<typename Type = Service, typename Allocator, typename... Args>
    static Service &emplace(std::allocator_arg_t, Allocator alloc, Args &&...args) {
        service = std::allocate_shared<Type>(alloc, std::forward<Args>(args)...);
        return *service;
    }

    /**
     * @brief Returns a handle to the underlying service.
     * @return A handle to the underlying service.
     */
    static node_type handle() noexcept {
        node_type node{};
        node.value = service;
        return node;
    }

    /**
     * @brief Resets or replaces a service.
     * @param other Optional handle with which to replace the service.
     */
    static void reset(const node_type &other = {}) noexcept {
        service = other.value;
    }

    /**
     * @brief Resets or replaces a service.
     * @tparam Type Service type.
     * @tparam Deleter Deleter type.
     * @param elem A pointer to a service to manage.
     * @param deleter A deleter to use to destroy the service.
     */
    template<typename Type, typename Deleter = std::default_delete<Type>>
    static void reset(Type *elem, Deleter deleter = {}) {
        service = std::shared_ptr<Service>{elem, std::move(deleter)};
    }

private:
    // std::shared_ptr because of its type erased allocator which is useful here
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    inline static std::shared_ptr<Service> service{};
};

} // namespace entt

#endif
