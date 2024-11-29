#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct clazz {
    clazz() = default;

    operator int() const {
        return value;
    }

    [[nodiscard]] bool to_bool() const {
        return (value != 0);
    }

    int value{};
};

double conv_to_double(const clazz &instance) {
    return instance.value * 2.;
}

struct MetaConv: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .conv<int>()
            .conv<&clazz::to_bool>()
            .conv<conv_to_double>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaConv, Conv) {
    auto any = entt::resolve<clazz>().construct();
    any.cast<clazz &>().value = 2;

    const auto as_int = std::as_const(any).allow_cast<int>();
    const auto as_bool = std::as_const(any).allow_cast<bool>();
    const auto as_double = std::as_const(any).allow_cast<double>();

    ASSERT_FALSE(any.allow_cast<char>());

    ASSERT_TRUE(as_int);
    ASSERT_TRUE(as_bool);
    ASSERT_TRUE(as_double);

    ASSERT_EQ(as_int.cast<int>(), any.cast<clazz &>().operator int());
    ASSERT_EQ(as_bool.cast<bool>(), any.cast<clazz &>().to_bool());
    ASSERT_EQ(as_double.cast<double>(), conv_to_double(any.cast<clazz &>()));
}

TEST_F(MetaConv, ReRegistration) {
    SetUp();

    auto &&node = entt::internal::resolve<clazz>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->conv.empty());
    ASSERT_EQ(node.details->conv.size(), 3u);
}
