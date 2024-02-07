#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct clazz_t {
    clazz_t(int &cnt)
        : counter{&cnt} {
        ++(*counter);
    }

    static void destroy_decr(clazz_t &instance) {
        --(*instance.counter);
    }

    void destroy_incr() const {
        ++(*counter);
    }

    int *counter;
};

struct MetaDtor: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .ctor<int &>()
            .dtor<clazz_t::destroy_decr>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaDtor, Functionalities) {
    int counter{};

    auto any = entt::resolve<clazz_t>().construct(entt::forward_as_meta(counter));
    auto cref = std::as_const(any).as_ref();
    auto ref = any.as_ref();

    ASSERT_TRUE(any);
    ASSERT_TRUE(cref);
    ASSERT_TRUE(ref);

    ASSERT_EQ(counter, 1);

    cref.reset();
    ref.reset();

    ASSERT_TRUE(any);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(counter, 1);

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(counter, 0);
}

TEST_F(MetaDtor, AsRefConstruction) {
    int counter{};

    clazz_t instance{counter};
    auto any = entt::forward_as_meta(instance);
    auto cany = entt::forward_as_meta(std::as_const(instance));
    auto cref = cany.as_ref();
    auto ref = any.as_ref();

    ASSERT_TRUE(any);
    ASSERT_TRUE(cany);
    ASSERT_TRUE(cref);
    ASSERT_TRUE(ref);

    ASSERT_EQ(counter, 1);

    any.reset();
    cany.reset();
    cref.reset();
    ref.reset();

    ASSERT_FALSE(any);
    ASSERT_FALSE(cany);
    ASSERT_FALSE(cref);
    ASSERT_FALSE(ref);

    ASSERT_EQ(counter, 1);
}

TEST_F(MetaDtor, ReRegistration) {
    SetUp();

    int counter{};
    auto &&node = entt::internal::resolve<clazz_t>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));

    ASSERT_NE(node.dtor.dtor, nullptr);

    entt::meta<clazz_t>().dtor<&clazz_t::destroy_incr>();
    entt::resolve<clazz_t>().construct(entt::forward_as_meta(counter)).reset();

    ASSERT_EQ(counter, 2);
}
