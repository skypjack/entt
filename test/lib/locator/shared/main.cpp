#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/locator/locator.hpp>
#include "types.h"

ENTT_API void set_up(const entt::locator<service>::node_type &);
ENTT_API void use_service(int);

TEST(Lib, Locator) {
    entt::locator<service>::emplace().value = 42;

    ASSERT_EQ(entt::locator<service>::value().value, 42);

    set_up(entt::locator<service>::handle());
    use_service(3);

    ASSERT_EQ(entt::locator<service>::value().value, 3);

    // service updates do not propagate across boundaries
    entt::locator<service>::emplace().value = 42;
    use_service(3);

    ASSERT_EQ(entt::locator<service>::value().value, 42);
}
