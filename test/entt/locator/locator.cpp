#include <gtest/gtest.h>
#include <entt/locator/locator.hpp>

struct A {};

struct B {
    virtual void f(bool) = 0;
    bool check{false};
};

struct D: B {
    D(int): B{} {}
    void f(bool b) override { check = b; }
};

TEST(ServiceLocator, Functionalities) {
    using entt::ServiceLocator;

    ASSERT_TRUE(ServiceLocator<A>::empty());
    ASSERT_TRUE(ServiceLocator<B>::empty());

    ServiceLocator<A>::set();

    ASSERT_FALSE(ServiceLocator<A>::empty());
    ASSERT_TRUE(ServiceLocator<B>::empty());

    ServiceLocator<A>::reset();

    ASSERT_TRUE(ServiceLocator<A>::empty());
    ASSERT_TRUE(ServiceLocator<B>::empty());

    ServiceLocator<A>::set(std::make_shared<A>());

    ASSERT_FALSE(ServiceLocator<A>::empty());
    ASSERT_TRUE(ServiceLocator<B>::empty());

    ServiceLocator<B>::set<D>(42);

    ASSERT_FALSE(ServiceLocator<A>::empty());
    ASSERT_FALSE(ServiceLocator<B>::empty());

    ServiceLocator<B>::get().lock()->f(!ServiceLocator<B>::get().lock()->check);

    ASSERT_TRUE(ServiceLocator<B>::get().lock()->check);

    ServiceLocator<B>::ref().f(!ServiceLocator<B>::get().lock()->check);

    ASSERT_FALSE(ServiceLocator<B>::get().lock()->check);
}
