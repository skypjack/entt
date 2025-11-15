#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/locator/locator.hpp>
#include "../../../common/boxed_type.h"
#include "lib.h"

TEST(Lib, Locator) {
    entt::locator<test::boxed_int>::emplace().value = 4;

    ASSERT_EQ(entt::locator<test::boxed_int>::value().value, 4);

    set_up(entt::locator<test::boxed_int>::handle());
    use_service(3);

    ASSERT_EQ(entt::locator<test::boxed_int>::value().value, 3);

    // service updates do not propagate across boundaries
    entt::locator<test::boxed_int>::emplace().value = 4;
    use_service(3);

    ASSERT_EQ(entt::locator<test::boxed_int>::value().value, 4);
}
