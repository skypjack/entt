#include <memory>
#include <gtest/gtest.h>
#include <entt/locator/locator.hpp>
#include "../../common/config.h"

struct base_service {
    virtual ~base_service() = default;
    virtual int invoke(int) = 0;
};

struct derived_service: base_service {
    derived_service(int val)
        : value{val} {}

    int invoke(int other) override {
        return value + other;
    }

private:
    int value;
};

struct ServiceLocator: ::testing::Test {
    void SetUp() override {
        entt::locator<base_service>::reset();
    }
};

using ServiceLocatorDeathTest = ServiceLocator;

TEST_F(ServiceLocator, ValueAndTheLike) {
    ASSERT_FALSE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value_or<derived_service>(1).invoke(3), 4);
    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(9), 10);
}

TEST_F(ServiceLocator, Emplace) {
    ASSERT_FALSE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::emplace<derived_service>(5).invoke(1), 6);
    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(3), 8);

    entt::locator<base_service>::reset();

    ASSERT_FALSE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::emplace<derived_service>(std::allocator_arg, std::allocator<derived_service>{}, 5).invoke(1), 6);
    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(3), 8);
}

TEST_F(ServiceLocator, ResetHandle) {
    entt::locator<base_service>::emplace<derived_service>(1);
    auto handle = entt::locator<base_service>::handle();

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(3), 4);

    entt::locator<base_service>::reset();

    ASSERT_FALSE(entt::locator<base_service>::has_value());

    entt::locator<base_service>::reset(handle);

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(3), 4);
}

TEST_F(ServiceLocator, ElementWithDeleter) {
    derived_service service{1};
    entt::locator<base_service>::reset(&service, [&service](base_service *serv) {
        ASSERT_EQ(serv, &service);
        service = derived_service{2};
    });

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_EQ(entt::locator<base_service>::value().invoke(1), 2);

    entt::locator<base_service>::reset();

    ASSERT_EQ(service.invoke(1), 3);
}

ENTT_DEBUG_TEST_F(ServiceLocatorDeathTest, UninitializedValue) {
    ASSERT_EQ(entt::locator<base_service>::value_or<derived_service>(1).invoke(1), 2);

    entt::locator<base_service>::reset();

    ASSERT_DEATH(entt::locator<base_service>::value().invoke(4), "");
}
