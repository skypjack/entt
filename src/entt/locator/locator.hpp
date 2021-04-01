#ifndef ENTT_LOCATOR_LOCATOR_HPP
#define ENTT_LOCATOR_LOCATOR_HPP


#include <memory>
#include <utility>
#include "../config/config.h"


namespace entt {


/**
 * @brief Service locator, nothing more.
 *
 * A service locator can be used to do what it promises: locate services.<br/>
 * Usually service locators are tightly bound to the services they expose and
 * thus it's hard to define a general purpose class to do that. This template
 * based implementation tries to fill the gap and to get rid of the burden of
 * defining a different specific locator for each application.
 *
 * @tparam Service Type of service managed by the locator.
 */
template<typename Service>
struct service_locator {
    /*! @brief Type of service offered. */
    using service_type = Service;

    /*! @brief Default constructor, deleted on purpose. */
    service_locator() = delete;
    /*! @brief Default destructor, deleted on purpose. */
    ~service_locator() = delete;

    /**
     * @brief Tests if a valid service implementation is set.
     * @return True if the service is set, false otherwise.
     */
    [[nodiscard]] static bool empty() ENTT_NOEXCEPT {
        return !static_cast<bool>(service);
    }

    /**
     * @brief Returns a weak pointer to a service implementation, if any.
     *
     * Clients of a service shouldn't retain references to it. The recommended
     * way is to retrieve the service implementation currently set each and
     * every time the need of using it arises. Otherwise users can incur in
     * unexpected behaviors.
     *
     * @return A reference to the service implementation currently set, if any.
     */
    [[nodiscard]] static std::weak_ptr<Service> get() ENTT_NOEXCEPT {
        return service;
    }

    /**
     * @brief Returns a weak reference to a service implementation, if any.
     *
     * Clients of a service shouldn't retain references to it. The recommended
     * way is to retrieve the service implementation currently set each and
     * every time the need of using it arises. Otherwise users can incur in
     * unexpected behaviors.
     *
     * @warning
     * In case no service implementation has been set, a call to this function
     * results in undefined behavior.
     *
     * @return A reference to the service implementation currently set, if any.
     */
    [[nodiscard]] static Service & ref() ENTT_NOEXCEPT {
        return *service;
    }

    /**
     * @brief Sets or replaces a service.
     * @tparam Impl Type of the new service to use.
     * @tparam Args Types of arguments to use to construct the service.
     * @param args Parameters to use to construct the service.
     */
    template<typename Impl = Service, typename... Args>
    static void set(Args &&... args) {
        service = std::make_shared<Impl>(std::forward<Args>(args)...);
    }

    /**
     * @brief Sets or replaces a service.
     * @param ptr Service to use to replace the current one.
     */
    static void set(std::shared_ptr<Service> ptr) {
        ENTT_ASSERT(static_cast<bool>(ptr), "Null service not allowed");
        service = std::move(ptr);
    }

    /**
     * @brief Resets a service.
     *
     * The service is no longer valid after a reset.
     */
    static void reset() {
        service.reset();
    }

private:
    inline static std::shared_ptr<Service> service = nullptr;
};


}


#endif
