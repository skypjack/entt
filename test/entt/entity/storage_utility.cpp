#include <gtest/gtest.h>
#include <entt/entity/storage.hpp>

template<typename Type>
struct StorageUtility: testing::Test {
    using type = Type;
};

using StorageUtilityTypes = ::testing::Types<int, char, double, void>;

TYPED_TEST_SUITE(StorageUtility, StorageUtilityTypes, );

TYPED_TEST(StorageUtility, StorageType) {
    using value_type = typename TestFixture::type;

    // just a bunch of static asserts to avoid regressions
#ifdef ENTT_NO_MIXIN
    testing::StaticAssertTypeEq<entt::storage_type_t<value_type, entt::entity>, entt::basic_storage<value_type, entt::entity>>();
    testing::StaticAssertTypeEq<entt::storage_type_t<value_type>, entt::storage<value_type>>();
#else
    testing::StaticAssertTypeEq<entt::storage_type_t<value_type, entt::entity>, entt::sigh_mixin<entt::basic_storage<value_type, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_type_t<value_type>, entt::sigh_mixin<entt::storage<value_type>>>();
#endif // ENTT_NO_MIXIN
}

TYPED_TEST(StorageUtility, StorageFor) {
    using value_type = typename TestFixture::type;

    // just a bunch of static asserts to avoid regressions
#ifdef ENTT_NO_MIXIN
    testing::StaticAssertTypeEq<entt::storage_for_t<const value_type, entt::entity>, const entt::basic_storage<value_type, entt::entity>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<value_type, entt::entity>, entt::basic_storage<value_type, entt::entity>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<const value_type>, const entt::storage<value_type>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<value_type>, entt::storage<value_type>>();
#else
    testing::StaticAssertTypeEq<entt::storage_for_t<const value_type, entt::entity>, const entt::sigh_mixin<entt::basic_storage<value_type, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<value_type, entt::entity>, entt::sigh_mixin<entt::basic_storage<value_type, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<const value_type>, const entt::sigh_mixin<entt::storage<value_type>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<value_type>, entt::sigh_mixin<entt::storage<value_type>>>();
#endif
}
