#include <iterator>
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

struct derived: base {
};

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

    entt::meta_factory<int> factory{};

    ASSERT_EQ(entt::resolve<int>().id(), entt::id_type{});

    factory.type("foo"_hs);

    ASSERT_EQ(entt::resolve<int>().id(), "foo"_hs);

    factory.type("bar"_hs);

    ASSERT_EQ(entt::resolve<int>().id(), "bar"_hs);
}

ENTT_DEBUG_TEST_F(MetaFactoryDeathTest, Type) {
    using namespace entt::literals;

    entt::meta_factory<int> factory{};
    entt::meta_factory<double> other{};

    factory.type("foo"_hs);

    ASSERT_DEATH(other.type("foo"_hs);, "");
}

TEST_F(MetaFactory, Base) {
    entt::meta_factory<derived> factory{};
    decltype(std::declval<entt::meta_type>().base()) range{};

    ASSERT_NE(entt::resolve(entt::type_id<derived>()), entt::meta_type{});
    ASSERT_EQ(entt::resolve(entt::type_id<base>()), entt::meta_type{});

    range = entt::resolve<derived>().base();

    ASSERT_EQ(range.begin(), range.end());

    factory.base<base>();
    range = entt::resolve<derived>().base();

    ASSERT_EQ(entt::resolve(entt::type_id<base>()), entt::meta_type{});
    ASSERT_NE(range.begin(), range.end());
    ASSERT_EQ(std::distance(range.begin(), range.end()), 1);
    ASSERT_EQ(range.begin()->first, entt::type_id<base>().hash());
    ASSERT_EQ(range.begin()->second.info(), entt::type_id<base>());
}
