#include <utility>
#include <gtest/gtest.h>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct clazz_t {
    clazz_t() {
        ++counter;
    }

    static void destroy_decr(clazz_t &) {
        --counter;
    }

    static void destroy_incr(clazz_t &) {
        ++counter;
    }

    inline static int counter = 0;
};

struct MetaDtor: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .dtor<&clazz_t::destroy_decr>();

        clazz_t::counter = 0;
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaDtor, Functionalities) {
    ASSERT_EQ(clazz_t::counter, 0);

    auto any = entt::resolve<clazz_t>().construct();
    auto cref = std::as_const(any).as_ref();
    auto ref = any.as_ref();

    ASSERT_TRUE(any);
    ASSERT_TRUE(cref);
    ASSERT_TRUE(ref);

    ASSERT_EQ(clazz_t::counter, 1);

    cref.reset();
    ref.reset();

    ASSERT_TRUE(any);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(clazz_t::counter, 1);

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(clazz_t::counter, 0);
}

TEST_F(MetaDtor, AsRefConstruction) {
    ASSERT_EQ(clazz_t::counter, 0);

    clazz_t instance{};
    entt::meta_any any{std::ref(instance)};
    entt::meta_any cany{std::cref(instance)};
    auto cref = cany.as_ref();
    auto ref = any.as_ref();

    ASSERT_TRUE(any);
    ASSERT_TRUE(cany);
    ASSERT_TRUE(cref);
    ASSERT_TRUE(ref);

    ASSERT_EQ(clazz_t::counter, 1);

    any.reset();
    cany.reset();
    cref.reset();
    ref.reset();

    ASSERT_FALSE(any);
    ASSERT_FALSE(cany);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(clazz_t::counter, 1);
}

TEST_F(MetaDtor, ReRegistration) {
    SetUp();

    auto *node = entt::internal::meta_info<clazz_t>::resolve();

    ASSERT_NE(node->dtor, nullptr);

    entt::meta<clazz_t>().dtor<&clazz_t::destroy_incr>();
    entt::resolve<clazz_t>().construct().reset();

    ASSERT_EQ(clazz_t::counter, 2);
}
