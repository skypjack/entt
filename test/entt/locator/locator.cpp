#include <gtest/gtest.h>
#include <entt/locator/locator.hpp>

struct a_service {};

struct another_service {
    virtual ~another_service() = default;
    virtual void f(bool) = 0;
    bool check{false};
};

struct derived_service: another_service {
    derived_service(int): another_service{} {}
    void f(bool b) override { check = b; }
};

TEST(ServiceLocator, Functionalities) {
    ASSERT_TRUE(entt::service_locator<a_service>::empty());
    ASSERT_TRUE(entt::service_locator<another_service>::empty());

    entt::service_locator<a_service>::set();

    ASSERT_FALSE(entt::service_locator<a_service>::empty());
    ASSERT_TRUE(entt::service_locator<another_service>::empty());

    entt::service_locator<a_service>::reset();

    ASSERT_TRUE(entt::service_locator<a_service>::empty());
    ASSERT_TRUE(entt::service_locator<another_service>::empty());

    entt::service_locator<a_service>::set(std::make_shared<a_service>());

    ASSERT_FALSE(entt::service_locator<a_service>::empty());
    ASSERT_TRUE(entt::service_locator<another_service>::empty());

    entt::service_locator<another_service>::set<derived_service>(42);

    ASSERT_FALSE(entt::service_locator<a_service>::empty());
    ASSERT_FALSE(entt::service_locator<another_service>::empty());

    entt::service_locator<another_service>::get().lock()->f(!entt::service_locator<another_service>::get().lock()->check);

    ASSERT_TRUE(entt::service_locator<another_service>::get().lock()->check);

    entt::service_locator<another_service>::ref().f(!entt::service_locator<another_service>::get().lock()->check);

    ASSERT_FALSE(entt::service_locator<another_service>::get().lock()->check);
}
