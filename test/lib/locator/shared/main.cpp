#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/locator/locator.hpp>
#include "../../../common/boxed_type.h"

ENTT_API void set_up(const entt::locator<test::boxed_int>::node_type &);
ENTT_API void use_service(int);

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
