#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

ENTT_API void share(const entt::locator<entt::meta_ctx>::node_type &);
ENTT_API void set_up();
ENTT_API void tear_down();
ENTT_API entt::meta_any wrap_int(int);

TEST(Lib, Meta) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve("boxed_int"_hs));
    ASSERT_FALSE(entt::resolve("empty"_hs));

    share(entt::locator<entt::meta_ctx>::handle());
    set_up();

    ASSERT_TRUE(entt::resolve("boxed_int"_hs));
    ASSERT_TRUE(entt::resolve("empty"_hs));

    ASSERT_EQ(entt::resolve<test::boxed_int>(), entt::resolve("boxed_int"_hs));
    ASSERT_EQ(entt::resolve<test::empty>(), entt::resolve("empty"_hs));

    auto boxed_int = entt::resolve("boxed_int"_hs).construct(4.);
    auto empty = entt::resolve("empty"_hs).construct();

    ASSERT_TRUE(boxed_int);
    ASSERT_TRUE(empty);

    ASSERT_EQ(boxed_int.type().data("value"_hs).type(), entt::resolve<int>());
    ASSERT_NE(boxed_int.get("value"_hs).try_cast<int>(), nullptr);
    ASSERT_EQ(boxed_int.get("value"_hs).cast<int>(), 4);

    boxed_int.reset();
    empty.reset();

    ASSERT_EQ(wrap_int(4).type(), entt::resolve<int>());
    ASSERT_EQ(wrap_int(4).cast<int>(), 4);

    tear_down();

    ASSERT_FALSE(entt::resolve("boxed_int"_hs));
    ASSERT_FALSE(entt::resolve("empty"_hs));
}
