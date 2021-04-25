#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct clazz_t {
    clazz_t(): value{} {}
    void incr() { ++value; }
    void decr() { --value; }
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
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaHandle, Functionalities) {
    using namespace entt::literals;

    clazz_t instance{};
    entt::meta_handle handle{};

    ASSERT_FALSE(handle);

    handle = entt::meta_handle{instance};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle->invoke("incr"_hs));
    ASSERT_EQ(instance.value, 1);

    auto any = entt::make_meta_any<clazz_t &>(instance);
    handle = entt::meta_handle{any};

    ASSERT_FALSE(std::as_const(handle)->invoke("decr"_hs));
    ASSERT_TRUE(handle->invoke("decr"_hs));
    ASSERT_EQ(instance.value, 0);
}
