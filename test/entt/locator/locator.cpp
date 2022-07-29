#include <memory>
#include <gtest/gtest.h>
#include <entt/locator/locator.hpp>
#include "../common/config.h"

struct base_service {
    virtual ~base_service() = default;
    virtual void invoke() {}
};

struct null_service: base_service {
    void invoke() override {
        invoked = true;
    }

    static inline bool invoked{};
};

struct derived_service: base_service {
    void invoke() override {
        invoked = true;
    }

    static inline bool invoked{};
};

struct ServiceLocator: ::testing::Test {
    void SetUp() override {
        null_service::invoked = false;
        derived_service::invoked = false;
    }
};

using ServiceLocatorDeathTest = ServiceLocator;

TEST(ServiceLocator, Functionalities) {
    ASSERT_FALSE(entt::locator<base_service>::has_value());
    ASSERT_FALSE(null_service::invoked);

    entt::locator<base_service>::value_or<null_service>().invoke();

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_TRUE(null_service::invoked);

    entt::locator<base_service>::reset();

    ASSERT_FALSE(entt::locator<base_service>::has_value());
    ASSERT_FALSE(derived_service::invoked);

    entt::locator<base_service>::emplace<derived_service>();
    entt::locator<base_service>::value().invoke();

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_TRUE(derived_service::invoked);

    derived_service::invoked = false;
    entt::locator<base_service>::allocate_emplace<derived_service>(std::allocator<derived_service>{}).invoke();

    ASSERT_TRUE(entt::locator<base_service>::has_value());
    ASSERT_TRUE(derived_service::invoked);
}

ENTT_DEBUG_TEST(ServiceLocatorDeathTest, UninitializedValue) {
    ASSERT_NO_FATAL_FAILURE(entt::locator<base_service>::value_or().invoke());

    entt::locator<base_service>::reset();

    ASSERT_DEATH(entt::locator<base_service>::value().invoke(), "");
}
