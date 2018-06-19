#include <gtest/gtest.h>
#include <entt/locator/locator.hpp>

struct AService {};

struct AnotherService {
    virtual ~AnotherService() = default;
    virtual void f(bool) = 0;
    bool check{false};
};

struct DerivedService: AnotherService {
    DerivedService(int): AnotherService{} {}
    void f(bool b) override { check = b; }
};

TEST(ServiceLocator, Functionalities) {
    using entt::ServiceLocator;

    ASSERT_TRUE(ServiceLocator<AService>::empty());
    ASSERT_TRUE(ServiceLocator<AnotherService>::empty());

    ServiceLocator<AService>::set();

    ASSERT_FALSE(ServiceLocator<AService>::empty());
    ASSERT_TRUE(ServiceLocator<AnotherService>::empty());

    ServiceLocator<AService>::reset();

    ASSERT_TRUE(ServiceLocator<AService>::empty());
    ASSERT_TRUE(ServiceLocator<AnotherService>::empty());

    ServiceLocator<AService>::set(std::make_shared<AService>());

    ASSERT_FALSE(ServiceLocator<AService>::empty());
    ASSERT_TRUE(ServiceLocator<AnotherService>::empty());

    ServiceLocator<AnotherService>::set<DerivedService>(42);

    ASSERT_FALSE(ServiceLocator<AService>::empty());
    ASSERT_FALSE(ServiceLocator<AnotherService>::empty());

    ServiceLocator<AnotherService>::get().lock()->f(!ServiceLocator<AnotherService>::get().lock()->check);

    ASSERT_TRUE(ServiceLocator<AnotherService>::get().lock()->check);

    ServiceLocator<AnotherService>::ref().f(!ServiceLocator<AnotherService>::get().lock()->check);

    ASSERT_FALSE(ServiceLocator<AnotherService>::get().lock()->check);
}
