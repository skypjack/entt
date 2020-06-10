#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/resolve.hpp>

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<char>().type("char"_hs);
        entt::meta<double>().type("double"_hs);
    }
};

TEST_F(Meta, Resolve) {
    ASSERT_EQ(entt::resolve<double>(), entt::resolve_id("double"_hs));
    ASSERT_EQ(entt::resolve<double>(), entt::resolve_type(entt::type_info<double>::id()));
    // it could be "char"_hs rather than entt::hashed_string::value("char") if it weren't for a bug in VS2017
    ASSERT_EQ(entt::resolve_if([](auto type) { return type.id() == entt::hashed_string::value("char"); }), entt::resolve<char>());

    bool found = false;

    entt::resolve([&found](auto type) {
        found = found || type == entt::resolve<double>();
    });

    ASSERT_TRUE(found);
}
