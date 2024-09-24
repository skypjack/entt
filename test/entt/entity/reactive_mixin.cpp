#include <memory>
#include <gtest/gtest.h>
#include <entt/entity/component.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/storage.hpp>
template<typename Type>
struct ReactiveMixin: testing::Test {
    using type = Type;
};

template<typename Type>
using ReactiveMixinDeathTest = ReactiveMixin<Type>;

using ReactiveMixinTypes = ::testing::Types<void, bool>;

TYPED_TEST_SUITE(ReactiveMixin, ReactiveMixinTypes, );
TYPED_TEST_SUITE(ReactiveMixinDeathTest, ReactiveMixinTypes, );

TYPED_TEST(ReactiveMixin, Constructors) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::reactive_mixin<entt::storage<value_type>> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());

    pool = entt::reactive_mixin<entt::storage<value_type>>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
}

