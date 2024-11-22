#include <iterator>
#include <string>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"

struct base {
};

struct clazz: base {
    clazz(int val)
        : value{val} {}

    explicit operator int() const noexcept {
        return value;
    }

private:
    int value{};
};

std::string clazz_to_string(const clazz &instance) {
    return std::to_string(static_cast<int>(instance));
}

clazz string_to_clazz(const std::string &value) {
    return clazz{std::stoi(value)};
}

struct MetaFactory: ::testing::Test {
    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaFactoryDeathTest = MetaFactory;

TEST_F(MetaFactory, Constructors) {
    entt::meta_ctx ctx{};

    ASSERT_EQ(entt::resolve(entt::type_id<int>()), entt::meta_type{});
    ASSERT_EQ(entt::resolve(ctx, entt::type_id<int>()), entt::meta_type{});

    entt::meta_factory<int> factory{};

    ASSERT_NE(entt::resolve(entt::type_id<int>()), entt::meta_type{});
    ASSERT_EQ(entt::resolve(ctx, entt::type_id<int>()), entt::meta_type{});

    // this is because of entt::meta, which should be deprecated nowadays
    ASSERT_FALSE(entt::resolve(entt::type_id<int>()).is_integral());

    factory = entt::meta_factory<int>{ctx};

    ASSERT_NE(entt::resolve(entt::type_id<int>()), entt::meta_type{});
    ASSERT_NE(entt::resolve(ctx, entt::type_id<int>()), entt::meta_type{});

    // this is because of entt::meta, which should be deprecated nowadays
    ASSERT_FALSE(entt::resolve(ctx, entt::type_id<int>()).is_integral());
}

TEST_F(MetaFactory, Type) {
    using namespace entt::literals;

    auto factory = entt::meta<int>();

    ASSERT_EQ(entt::resolve("foo"_hs), entt::meta_type{});

    factory.type("foo"_hs);

    ASSERT_NE(entt::resolve("foo"_hs), entt::meta_type{});
    ASSERT_EQ(entt::resolve<int>().id(), "foo"_hs);

    factory.type("bar"_hs);

    ASSERT_EQ(entt::resolve("foo"_hs), entt::meta_type{});
    ASSERT_NE(entt::resolve("bar"_hs), entt::meta_type{});
    ASSERT_EQ(entt::resolve<int>().id(), "bar"_hs);
}

ENTT_DEBUG_TEST_F(MetaFactoryDeathTest, Type) {
    using namespace entt::literals;

    auto factory = entt::meta<int>();
    auto other = entt::meta<double>();

    factory.type("foo"_hs);

    ASSERT_DEATH(other.type("foo"_hs);, "");
}

TEST_F(MetaFactory, Base) {
    auto factory = entt::meta<clazz>();
    decltype(std::declval<entt::meta_type>().base()) range{};

    ASSERT_NE(entt::resolve(entt::type_id<clazz>()), entt::meta_type{});
    ASSERT_EQ(entt::resolve(entt::type_id<base>()), entt::meta_type{});

    range = entt::resolve<clazz>().base();

    ASSERT_EQ(range.begin(), range.end());

    factory.base<base>();
    range = entt::resolve<clazz>().base();

    ASSERT_EQ(entt::resolve(entt::type_id<base>()), entt::meta_type{});
    ASSERT_NE(range.begin(), range.end());
    ASSERT_EQ(std::distance(range.begin(), range.end()), 1);
    ASSERT_EQ(range.begin()->first, entt::type_id<base>().hash());
    ASSERT_EQ(range.begin()->second.info(), entt::type_id<base>());
}

TEST_F(MetaFactory, Conv) {
    clazz instance{3};
    auto factory = entt::meta<clazz>();
    const entt::meta_any any = entt::forward_as_meta(instance);

    ASSERT_FALSE(any.allow_cast<int>());
    ASSERT_FALSE(any.allow_cast<std::string>());

    factory.conv<int>().conv<&clazz_to_string>();

    ASSERT_TRUE(any.allow_cast<int>());
    ASSERT_TRUE(any.allow_cast<std::string>());
    ASSERT_EQ(any.allow_cast<int>().cast<int>(), static_cast<int>(instance));
    ASSERT_EQ(any.allow_cast<std::string>().cast<std::string>(), clazz_to_string(instance));
}

TEST_F(MetaFactory, Ctor) {
    const int values[]{1, 3};
    auto factory = entt::meta<clazz>();

    ASSERT_FALSE(entt::resolve<clazz>().construct(values[0u]));
    ASSERT_FALSE(entt::resolve<clazz>().construct(std::to_string(values[1u])));

    factory.ctor<int>().ctor<&string_to_clazz>();

    const auto instance = entt::resolve<clazz>().construct(values[0u]);
    const auto other = entt::resolve<clazz>().construct(std::to_string(values[1u]));

    ASSERT_TRUE(instance);
    ASSERT_TRUE(other);
    ASSERT_TRUE(instance.allow_cast<clazz>());
    ASSERT_TRUE(other.allow_cast<clazz>());
    ASSERT_EQ(static_cast<int>(instance.cast<const clazz &>()), values[0u]);
    ASSERT_EQ(static_cast<int>(other.cast<const clazz &>()), values[1u]);
}
