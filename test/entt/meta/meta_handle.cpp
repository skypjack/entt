#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

struct clazz_t {
    clazz_t()
        : value{} {}

    void incr() {
        ++value;
    }

    void decr() {
        --value;
    }

    int value;
};

struct MetaHandle: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .func<&clazz_t::incr>("incr"_hs)
            .func<&clazz_t::decr>("decr"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaHandle, Functionalities) {
    using namespace entt::literals;

    clazz_t instance{};
    entt::meta_handle handle{};
    entt::meta_handle chandle{};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(chandle);

    handle = entt::meta_handle{instance};
    chandle = entt::meta_handle{std::as_const(instance)};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(chandle);

    ASSERT_TRUE(handle->invoke("incr"_hs));
    ASSERT_FALSE(chandle->invoke("incr"_hs));
    ASSERT_FALSE(std::as_const(handle)->invoke("incr"_hs));
    ASSERT_EQ(instance.value, 1);

    auto any = entt::forward_as_meta(instance);
    handle = entt::meta_handle{any};
    chandle = entt::meta_handle{std::as_const(any)};

    ASSERT_TRUE(handle->invoke("decr"_hs));
    ASSERT_FALSE(chandle->invoke("decr"_hs));
    ASSERT_FALSE(std::as_const(handle)->invoke("decr"_hs));
    ASSERT_EQ(instance.value, 0);
}
