#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>

struct clazz {};
struct local_only {};

struct MetaContext: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        // global context

        entt::meta<clazz>()
            .type("foo"_hs);

        // local context

        entt::meta<local_only>(context)
            .type("quux"_hs);

        entt::meta<clazz>(context)
            .type("bar"_hs);
    }

    void TearDown() override {
        entt::meta_reset(context);
        entt::meta_reset();
    }

    entt::meta_ctx context{};
};

TEST_F(MetaContext, Resolve) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve<clazz>());
    ASSERT_TRUE(entt::resolve<clazz>(context));

    ASSERT_TRUE(entt::resolve<local_only>());
    ASSERT_TRUE(entt::resolve<local_only>(context));

    ASSERT_TRUE(entt::resolve(entt::type_id<clazz>()));
    ASSERT_TRUE(entt::resolve(context, entt::type_id<clazz>()));

    ASSERT_FALSE(entt::resolve(entt::type_id<local_only>()));
    ASSERT_TRUE(entt::resolve(context, entt::type_id<local_only>()));

    ASSERT_TRUE(entt::resolve("foo"_hs));
    ASSERT_FALSE(entt::resolve(context, "foo"_hs));

    ASSERT_FALSE(entt::resolve("bar"_hs));
    ASSERT_TRUE(entt::resolve(context, "bar"_hs));

    ASSERT_FALSE(entt::resolve("quux"_hs));
    ASSERT_TRUE(entt::resolve(context, "quux"_hs));

    ASSERT_EQ((std::distance(entt::resolve().cbegin(), entt::resolve().cend())), 1);
    ASSERT_EQ((std::distance(entt::resolve(context).cbegin(), entt::resolve(context).cend())), 2);
}
