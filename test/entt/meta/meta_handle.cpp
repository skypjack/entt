#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

struct clazz {
    void incr() {
        ++value;
    }

    void decr() {
        --value;
    }

    int value{};
};

struct MetaHandle: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .func<&clazz::incr>("incr"_hs)
            .func<&clazz::decr>("decr"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaHandle, Handle) {
    using namespace entt::literals;

    clazz instance{};
    entt::meta_handle handle{};
    entt::meta_handle chandle{};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(chandle);

    ASSERT_EQ(handle, chandle);
    ASSERT_EQ(handle, entt::meta_handle{});
    ASSERT_FALSE(handle != handle);
    ASSERT_TRUE(handle == handle);

    handle = entt::meta_handle{instance};
    chandle = entt::meta_handle{std::as_const(instance)};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(chandle);

    ASSERT_EQ(handle, chandle);
    ASSERT_NE(handle, entt::meta_handle{});
    ASSERT_FALSE(handle != handle);
    ASSERT_TRUE(handle == handle);

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
