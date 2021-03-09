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
    static void StaticSetUp() {
        using namespace entt::literals;

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .dtor<&clazz_t::destroy_decr>();

        clazz_t::counter = 0;
    }

    void SetUp() override {
        StaticSetUp();
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

    ASSERT_EQ(clazz_t::counter, 1);

    any.reset();

    ASSERT_EQ(clazz_t::counter, 0);
}

TEST_F(MetaDtor, ReRegistration) {
    MetaDtor::StaticSetUp();

    auto *node = entt::internal::meta_info<clazz_t>::resolve();

    ASSERT_NE(node->dtor, nullptr);

    entt::meta<clazz_t>().dtor<&clazz_t::destroy_incr>();
    entt::resolve<clazz_t>().construct().reset();

    ASSERT_EQ(clazz_t::counter, 2);
}
