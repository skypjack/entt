
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

struct count_tracker final {
    inline static void on_construct(entt::registry &, entt::entity) {
        ++created;
    }

    inline static void on_update(entt::registry &, entt::entity) {
        ++updated;
    }

    inline static void on_destroy(entt::registry &, entt::entity) {
        ++destroyed;
    }

    inline static std::size_t created = 0;
    inline static std::size_t updated = 0;
    inline static std::size_t destroyed = 0;

    inline static std::size_t alive() {
        return created - destroyed;
    }
};

template<typename Type>
struct StorageSignals: testing::Test {
    using type = Type;
};

using StorageSignalsTypes = ::testing::Types<count_tracker>;

TYPED_TEST_SUITE(StorageSignals, StorageSignalsTypes, );

TYPED_TEST(StorageSignals, AutoSignals) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    auto const id = registry.create();

    registry.emplace<count_tracker>(id);

    ASSERT_EQ(count_tracker::created, 1);
    ASSERT_EQ(count_tracker::updated, 0);
    ASSERT_EQ(count_tracker::destroyed, 0);
    ASSERT_EQ(count_tracker::alive(), 1);

    registry.replace<count_tracker>(id);

    ASSERT_EQ(count_tracker::created, 1);
    ASSERT_EQ(count_tracker::updated, 1);
    ASSERT_EQ(count_tracker::destroyed, 0);
    ASSERT_EQ(count_tracker::alive(), 1);

    registry.remove<count_tracker>(id);

    ASSERT_EQ(count_tracker::created, 1);
    ASSERT_EQ(count_tracker::updated, 1);
    ASSERT_EQ(count_tracker::destroyed, 1);
    ASSERT_EQ(count_tracker::alive(), 0);
}
