#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct clazz_t {
    clazz_t() = default;

    operator int() const {
        return value;
    }

    bool to_bool() const {
        return (value != 0);
    }

    int value{};
};

double conv_to_double(const clazz_t &instance) {
    return instance.value * 2.;
}

struct MetaConv: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .conv<int>()
            .conv<&clazz_t::to_bool>()
            .conv<conv_to_double>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaConv, Functionalities) {
    auto any = entt::resolve<clazz_t>().construct();
    any.cast<clazz_t &>().value = 42;

    const auto as_int = std::as_const(any).allow_cast<int>();
    const auto as_bool = std::as_const(any).allow_cast<bool>();
    const auto as_double = std::as_const(any).allow_cast<double>();

    ASSERT_FALSE(any.allow_cast<char>());

    ASSERT_TRUE(as_int);
    ASSERT_TRUE(as_bool);
    ASSERT_TRUE(as_double);

    ASSERT_EQ(as_int.cast<int>(), any.cast<clazz_t &>().operator int());
    ASSERT_EQ(as_bool.cast<bool>(), any.cast<clazz_t &>().to_bool());
    ASSERT_EQ(as_double.cast<double>(), conv_to_double(any.cast<clazz_t &>()));
}

TEST_F(MetaConv, ReRegistration) {
    SetUp();

    auto *node = entt::internal::meta_node<clazz_t>::resolve();

    ASSERT_NE(node->conv, nullptr);
    ASSERT_NE(node->conv->next, nullptr);
    ASSERT_NE(node->conv->next->next, nullptr);
    ASSERT_EQ(node->conv->next->next->next, nullptr);
}
