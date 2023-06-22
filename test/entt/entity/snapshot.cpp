#include <cstddef>
#include <map>
#include <memory>
#include <queue>
#include <tuple>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <entt/signal/sigh.hpp>

struct empty {};

struct shadow {
    entt::entity target{entt::null};

    static void listener(entt::entity &elem, entt::registry &registry, const entt::entity entt) {
        elem = registry.get<shadow>(entt).target;
    }
};

TEST(BasicSnapshot, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_snapshot<entt::registry>>);

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    entt::basic_snapshot other{std::move(snapshot)};

    ASSERT_NO_FATAL_FAILURE(snapshot = std::move(other));
}

TEST(BasicSnapshot, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&value) { data.emplace_back(std::forward<decltype(value)>(value)); };

    snapshot.get<entt::entity>(archive);

    ASSERT_EQ(data.size(), 2u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.in_use());

    entt::entity entities[3u];

    registry.create(std::begin(entities), std::end(entities));
    registry.destroy(entities[1u]);

    data.clear();
    snapshot.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.in_use());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[2u]), storage.data()[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), storage.data()[1u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), storage.data()[2u]);
}

TEST(BasicSnapshot, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<int>();

    entt::entity entities[3u];
    const int values[3u]{1, 2, 3};

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<int>(std::begin(entities), std::end(entities), std::begin(values));
    registry.destroy(entities[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&value) { data.emplace_back(std::forward<decltype(value)>(value)); };

    snapshot.get<int>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entities[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), values[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), entities[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[4u]), values[2u]);
}

TEST(BasicSnapshot, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<empty>();

    entt::entity entities[3u];

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<empty>(std::begin(entities), std::end(entities));
    registry.destroy(entities[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&value) { data.emplace_back(std::forward<decltype(value)>(value)); };

    snapshot.get<empty>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<empty>(archive);

    ASSERT_EQ(data.size(), 3u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entities[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[2u]), entities[2u]);
}

TEST(BasicSnapshot, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};

    entt::entity entities[3u];
    const int values[3u]{1, 2, 3};

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<int>(std::begin(entities), std::end(entities), std::begin(values));
    registry.destroy(entities[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&value) { data.emplace_back(std::forward<decltype(value)>(value)); };

    snapshot.get<int>(archive, std::begin(entities), std::end(entities), "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive, std::begin(entities), std::end(entities));

    ASSERT_EQ(data.size(), 6u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), static_cast<typename traits_type::entity_type>(std::distance(std::begin(entities), std::end(entities))));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entities[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), values[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), static_cast<entt::entity>(entt::null));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), entities[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[5u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[5u]), values[2u]);
}

TEST(BasicSnapshotLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_snapshot_loader<entt::registry>>);

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    entt::basic_snapshot_loader other{std::move(loader)};

    ASSERT_NO_FATAL_FAILURE(loader = std::move(other));
}

TEST(BasicSnapshotLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[3u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u), traits_type::construct(1u, 1u)};

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.in_use(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);
    data.emplace_back(entities[2u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));

    ASSERT_EQ(storage.size(), 3u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[0u], entities[0u]);
    ASSERT_EQ(storage[1u], entities[1u]);
    ASSERT_EQ(storage[2u], entities[2u]);

    ASSERT_EQ(registry.create(), entities[2u]);
}

TEST(BasicSnapshotLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(entities[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entities[0u]));
    ASSERT_TRUE(storage.contains(entities[1u]));
    ASSERT_EQ(storage.get(entities[0u]), values[0u]);
    ASSERT_EQ(storage.get(entities[1u]), values[1u]);
}

TEST(BasicSnapshotLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);

    loader.get<empty>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);

    loader.get<empty>(archive);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entities[0u]));
    ASSERT_TRUE(storage.contains(entities[1u]));
}

TEST(BasicSnapshotLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entities[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entities[0u]));
    ASSERT_TRUE(storage.contains(entities[1u]));
    ASSERT_EQ(storage.get(entities[0u]), values[0u]);
    ASSERT_EQ(storage.get(entities[1u]), values[1u]);
}

TEST(BasicSnapshotLoader, GetTypeWithListener) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    entt::entity check{entt::null};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const auto entity{traits_type::construct(1u, 1u)};
    const shadow value{entity};

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_EQ(check, static_cast<entt::entity>(entt::null));

    registry.on_construct<shadow>().connect<&shadow::listener>(check);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity);
    data.emplace_back(value);

    loader.get<shadow>(archive);

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_EQ(check, entity);
}

TEST(BasicSnapshotLoader, Orphans) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 42;

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));

    loader.orphans();

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
}

TEST(BasicContinuousLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_continuous_loader<entt::registry>>);

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    entt::basic_continuous_loader other{std::move(loader)};

    ASSERT_NO_FATAL_FAILURE(loader = std::move(other));
}

TEST(BasicContinuousLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[3u]{traits_type::construct(1u, 0u), traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));
    ASSERT_FALSE(loader.contains(entities[2u]));

    ASSERT_EQ(loader.map(entities[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entities[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.in_use(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);
    data.emplace_back(entities[2u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));
    ASSERT_FALSE(loader.contains(entities[2u]));

    ASSERT_NE(loader.map(entities[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entities[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[0u], loader.map(entities[0u]));
    ASSERT_EQ(storage[1u], loader.map(entities[1u]));

    ASSERT_EQ(registry.create(), entities[2u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);
    data.emplace_back(entities[2u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));
    ASSERT_TRUE(loader.contains(entities[2u]));

    ASSERT_NE(loader.map(entities[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entities[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[2u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.in_use(), 4u);

    ASSERT_EQ(storage[0u], loader.map(entities[0u]));
    ASSERT_EQ(storage[1u], loader.map(entities[1u]));
    ASSERT_EQ(storage[3u], loader.map(entities[2u]));

    registry.destroy(loader.map(entities[1u]));

    ASSERT_TRUE(loader.contains(entities[1u]));
    ASSERT_NE(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entities[1u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entities[1u]));
    ASSERT_NE(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));
    ASSERT_EQ(storage[3u], loader.map(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entities[1u]);
    data.emplace_back(entities[2u]);
    data.emplace_back(entities[0u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));
    ASSERT_FALSE(loader.contains(entities[2u]));

    ASSERT_EQ(loader.map(entities[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entities[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entities[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[1u], loader.map(entities[1u]));
}

TEST(BasicContinuousLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(entities[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entities[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entities[1u])));
    ASSERT_EQ(storage.get(loader.map(entities[0u])), values[0u]);
    ASSERT_EQ(storage.get(loader.map(entities[1u])), values[1u]);
}

TEST(BasicContinuousLoader, GetTypeExtended) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<shadow>();

    std::vector<entt::any> data{};
    const entt::entity entities[2u]{traits_type::construct(0u, 1u), traits_type::construct(1u, 1u)};
    const shadow value{entities[0u]};

    auto archive = [&loader, &data, pos = 0u](auto &value) mutable {
        value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]);

        if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>, shadow>) {
            value.target = loader.map(value.target);
        }
    };

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[1u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<shadow>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    ASSERT_EQ(storage.size(), 1u);
    ASSERT_TRUE(storage.contains(loader.map(entities[1u])));
    ASSERT_EQ(storage.get(loader.map(entities[1u])).target, loader.map(entities[0u]));
}

TEST(BasicContinuousLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);

    loader.get<empty>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);

    loader.get<empty>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entities[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entities[1u])));
}

TEST(BasicContinuousLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_FALSE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entities[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entities[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entities[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entities[1u])));
    ASSERT_EQ(storage.get(loader.map(entities[0u])), values[0u]);
    ASSERT_EQ(storage.get(loader.map(entities[1u])), values[1u]);
}

TEST(BasicContinuousLoader, GetTypeWithListener) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    entt::entity check{entt::null};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const auto entity{traits_type::construct(1u, 1u)};
    const shadow value{entity};

    ASSERT_FALSE(registry.valid(loader.map(entity)));
    ASSERT_EQ(check, static_cast<entt::entity>(entt::null));

    registry.on_construct<shadow>().connect<&shadow::listener>(check);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity);
    data.emplace_back(value);

    loader.get<shadow>(archive);

    ASSERT_TRUE(registry.valid(loader.map(entity)));
    ASSERT_EQ(check, entity);
}

TEST(BasicContinuousLoader, Shrink) {
    entt::registry registry;
    entt::basic_continuous_loader loader{registry};

    ASSERT_NO_FATAL_FAILURE(loader.shrink());
}

TEST(BasicContinuousLoader, Orphans) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const entt::entity entities[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 42;

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entities[0u]);
    data.emplace_back(entities[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entities[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entities[1u])));

    loader.orphans();

    ASSERT_TRUE(loader.contains(entities[0u]));
    ASSERT_TRUE(loader.contains(entities[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entities[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entities[1u])));
}

template<typename Storage>
struct output_archive {
    output_archive(Storage &instance)
        : storage{instance} {}

    template<typename Value>
    void operator()(const Value &value) {
        std::get<std::queue<Value>>(storage).push(value);
    }

    void operator()(const std::unique_ptr<int> &instance) {
        (*this)(*instance);
    }

private:
    Storage &storage;
};

template<typename Storage>
struct input_archive {
    input_archive(Storage &instance)
        : storage{instance} {}

    template<typename Value>
    void operator()(Value &value) {
        auto assign = [this](auto &val) {
            auto &queue = std::get<std::queue<std::decay_t<decltype(val)>>>(storage);
            val = queue.front();
            queue.pop();
        };

        assign(value);
    }

    void operator()(std::unique_ptr<int> &instance) {
        instance = std::make_unique<int>();
        (*this)(*instance);
    }

private:
    Storage &storage;
};

struct a_component {};

struct another_component {
    int key{};
    int value{};
};

struct what_a_component {
    entt::entity bar{};
    std::vector<entt::entity> quux{};
};

struct map_component {
    std::map<entt::entity, int> keys{};
    std::map<int, entt::entity> values{};
    std::map<entt::entity, entt::entity> both{};
};

TEST(Snapshot, Dump) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);
    registry.emplace<char>(e0, 'c');
    registry.emplace<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 3);

    const auto e3 = registry.create();
    registry.emplace<a_component>(e3);
    registry.emplace<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using archive_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<a_component>,
        std::queue<another_component>>;

    archive_type storage;
    output_archive<archive_type> output{storage};
    input_archive<archive_type> input{storage};

    entt::snapshot{registry}
        .entities(output)
        .component<int>(output)
        .component<char>(output)
        .component<double>(output)
        .component<a_component>(output)
        .component<another_component>(output);

    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}
        .entities(input)
        .component<int>(input)
        .component<char>(input)
        .component<double>(input)
        .component<a_component>(input)
        .component<another_component>(input)
        .orphans();

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e2));
    ASSERT_FALSE(registry.orphan(e3));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<double>(e0), .1);
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');
    ASSERT_TRUE(registry.all_of<a_component>(e3));

    ASSERT_TRUE(registry.storage<another_component>().empty());
}

TEST(Snapshot, Partial) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);
    registry.emplace<char>(e0, 'c');
    registry.emplace<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 3);

    const auto e3 = registry.create();
    registry.emplace<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using archive_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>>;

    archive_type storage;
    output_archive<archive_type> output{storage};
    input_archive<archive_type> input{storage};

    entt::snapshot{registry}
        .entities(output)
        .component<char>(output)
        .component<int>(output);

    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}
        .entities(input)
        .component<char>(input)
        .component<int>(input);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_FALSE(registry.all_of<double>(e0));
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');

    entt::snapshot{registry}
        .entities(output);

    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}
        .entities(input)
        .orphans();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
}

TEST(Snapshot, Iterator) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    for(auto i = 0; i < 50; ++i) {
        const auto entity = registry.create();
        registry.emplace<a_component>(entity);

        if(i % 2) {
            registry.emplace<another_component>(entity, i, i);
            registry.emplace<std::unique_ptr<int>>(entity, std::make_unique<int>(i));
        }
    }

    using archive_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<another_component>,
        std::queue<int>>;

    archive_type storage;
    output_archive<archive_type> output{storage};
    input_archive<archive_type> input{storage};

    const auto view = registry.view<a_component>();
    const auto size = view.size();

    entt::snapshot{registry}
        .component<another_component>(output, view.begin(), view.end())
        .component<std::unique_ptr<int>>(output, view.begin(), view.end());

    registry.clear();

    entt::snapshot_loader{registry}
        .component<another_component>(input)
        .component<std::unique_ptr<int>>(input);

    ASSERT_EQ(registry.view<another_component>().size(), size / 2u);

    registry.view<another_component>().each([](const auto entity, const auto &) {
        ASSERT_NE(entt::to_integral(entity) % 2u, 0u);
    });
}

TEST(Snapshot, Continuous) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    std::vector<entt::entity> entities;
    entt::entity entity;

    using archive_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<another_component>,
        std::queue<what_a_component>,
        std::queue<map_component>,
        std::queue<int>,
        std::queue<double>>;

    archive_type storage;
    output_archive<archive_type> output{storage};
    input_archive<archive_type> input{storage};

    for(int i = 0; i < 10; ++i) {
        static_cast<void>(src.create());
    }

    src.clear();

    for(int i = 0; i < 5; ++i) {
        entity = src.create();
        entities.push_back(entity);

        src.emplace<a_component>(entity);
        src.emplace<another_component>(entity, i, i);
        src.emplace<std::unique_ptr<int>>(entity, std::make_unique<int>(i));

        if(i % 2) {
            src.emplace<what_a_component>(entity, entity);
        } else {
            src.emplace<map_component>(entity);
        }
    }

    src.view<what_a_component>().each([&entities](auto, auto &what_a_component) {
        what_a_component.quux.insert(what_a_component.quux.begin(), entities.begin(), entities.end());
    });

    src.view<map_component>().each([&entities](auto, auto &map_component) {
        for(std::size_t i = 0; i < entities.size(); ++i) {
            map_component.keys.insert({entities[i], int(i)});
            map_component.values.insert({int(i), entities[i]});
            map_component.both.insert({entities[entities.size() - i - 1], entities[i]});
        }
    });

    entity = dst.create();
    dst.emplace<a_component>(entity);
    dst.emplace<another_component>(entity, -1, -1);
    dst.emplace<std::unique_ptr<int>>(entity, std::make_unique<int>(-1));

    entt::snapshot{src}
        .entities(output)
        .component<a_component>(output)
        .component<another_component>(output)
        .component<what_a_component>(output)
        .component<map_component>(output)
        .component<std::unique_ptr<int>>(output);

    loader
        .entities(input)
        .component<a_component>(input)
        .component<another_component>(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .component<std::unique_ptr<int>>(input)
        .orphans();

    decltype(dst.size()) a_component_cnt{};
    decltype(dst.size()) another_component_cnt{};
    decltype(dst.size()) what_a_component_cnt{};
    decltype(dst.size()) map_component_cnt{};
    decltype(dst.size()) unique_ptr_cnt{};

    dst.each([&dst, &a_component_cnt](auto entt) {
        ASSERT_TRUE(dst.all_of<a_component>(entt));
        ++a_component_cnt;
    });

    dst.view<another_component>().each([&another_component_cnt](auto, const auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : component.key);
        ++another_component_cnt;
    });

    dst.view<what_a_component>().each([&dst, &what_a_component_cnt](auto entt, const auto &component) {
        ASSERT_EQ(entt, component.bar);

        for(auto child: component.quux) {
            ASSERT_TRUE(dst.valid(child));
        }

        ++what_a_component_cnt;
    });

    dst.view<map_component>().each([&dst, &map_component_cnt](const auto &component) {
        for(auto child: component.keys) {
            ASSERT_TRUE(dst.valid(child.first));
        }

        for(auto child: component.values) {
            ASSERT_TRUE(dst.valid(child.second));
        }

        for(auto child: component.both) {
            ASSERT_TRUE(dst.valid(child.first));
            ASSERT_TRUE(dst.valid(child.second));
        }

        ++map_component_cnt;
    });

    dst.view<std::unique_ptr<int>>().each([&dst, &unique_ptr_cnt](auto, const auto &component) {
        ++unique_ptr_cnt;
        ASSERT_EQ(*component, static_cast<int>(dst.storage<std::unique_ptr<int>>().size() - unique_ptr_cnt - 1u));
    });

    src.view<another_component>().each([](auto, auto &component) {
        component.value = 2 * component.key;
    });

    auto size = dst.size();

    entt::snapshot{src}
        .entities(output)
        .component<a_component>(output)
        .component<what_a_component>(output)
        .component<map_component>(output)
        .component<another_component>(output);

    loader
        .entities(input)
        .component<a_component>(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .component<another_component>(input)
        .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);
    ASSERT_EQ(dst.storage<another_component>().size(), another_component_cnt);
    ASSERT_EQ(dst.storage<what_a_component>().size(), what_a_component_cnt);
    ASSERT_EQ(dst.storage<map_component>().size(), map_component_cnt);
    ASSERT_EQ(dst.storage<std::unique_ptr<int>>().size(), unique_ptr_cnt);

    dst.view<another_component>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    entt::snapshot{src}
        .entities(output)
        .component<a_component>(output)
        .component<what_a_component>(output)
        .component<map_component>(output)
        .component<another_component>(output);

    loader
        .entities(input)
        .component<a_component>(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .component<another_component>(input)
        .orphans();

    dst.view<what_a_component>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    entities.clear();
    for(auto entt: src.view<a_component>()) {
        entities.push_back(entt);
    }

    src.destroy(entity);
    loader.shrink();

    entt::snapshot{src}
        .entities(output)
        .component<a_component>(output)
        .component<another_component>(output)
        .component<what_a_component>(output)
        .component<map_component>(output);

    loader
        .entities(input)
        .component<a_component>(input)
        .component<another_component>(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .orphans()
        .shrink();

    dst.view<what_a_component>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.contains(entity));

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.clear<a_component>();
    a_component_cnt = src.storage<a_component>().size();

    entt::snapshot{src}
        .entities(output)
        .component<a_component>(output)
        .component<what_a_component>(output)
        .component<map_component>(output)
        .component<another_component>(output);

    loader
        .entities(input)
        .component<a_component>(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .component<another_component>(input)
        .orphans();

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);

    src.clear<a_component>();
    a_component_cnt = {};

    entt::snapshot{src}
        .entities(output)
        .component<what_a_component>(output)
        .component<map_component>(output)
        .component<a_component>(output)
        .component<another_component>(output);

    loader
        .entities(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both)
        .component<a_component>(input)
        .component<another_component>(input)
        .orphans();

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);
}

TEST(Snapshot, SyncDataMembers) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    using archive_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<what_a_component>,
        std::queue<map_component>>;

    archive_type storage;
    output_archive<archive_type> output{storage};
    input_archive<archive_type> input{storage};

    static_cast<void>(src.create());
    static_cast<void>(src.create());

    src.clear();

    auto parent = src.create();
    auto child = src.create();

    src.emplace<what_a_component>(parent, entt::null);
    src.emplace<what_a_component>(child, parent).quux.push_back(child);

    src.emplace<map_component>(
        child,
        decltype(map_component::keys){{{child, 10}}},
        decltype(map_component::values){{{10, child}}},
        decltype(map_component::both){{{child, child}}});

    entt::snapshot{src}
        .entities(output)
        .component<what_a_component>(output)
        .component<map_component>(output);

    loader
        .entities(input)
        .component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
        .component<map_component>(input, &map_component::keys, &map_component::values, &map_component::both);

    ASSERT_FALSE(dst.valid(parent));
    ASSERT_FALSE(dst.valid(child));

    ASSERT_TRUE(dst.all_of<what_a_component>(loader.map(parent)));
    ASSERT_TRUE(dst.all_of<what_a_component>(loader.map(child)));

    ASSERT_EQ(dst.get<what_a_component>(loader.map(parent)).bar, static_cast<entt::entity>(entt::null));

    const auto &component = dst.get<what_a_component>(loader.map(child));

    ASSERT_EQ(component.bar, loader.map(parent));
    ASSERT_EQ(component.quux[0], loader.map(child));

    const auto &elem = dst.get<map_component>(loader.map(child));
    ASSERT_EQ(elem.keys.at(loader.map(child)), 10);
    ASSERT_EQ(elem.values.at(10), loader.map(child));
    ASSERT_EQ(elem.both.at(loader.map(child)), loader.map(child));
}
