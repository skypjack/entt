#include <array>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <entt/signal/sigh.hpp>
#include "../../common/config.h"
#include "../../common/empty.h"
#include "../../common/pointer_stable.h"

struct shadow {
    entt::entity target{entt::null};

    static void listener(entt::entity &elem, entt::registry &registry, const entt::entity entt) {
        elem = registry.get<shadow>(entt).target;
    }
};

TEST(BasicSnapshot, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot<entt::registry>>, "Default constructible type not allowed");
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot<entt::registry>>, "Copy constructible type not allowed");
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot<entt::registry>>, "Copy assignable type not allowed");
    static_assert(std::is_move_constructible_v<entt::basic_snapshot<entt::registry>>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<entt::basic_snapshot<entt::registry>>, "Move assignable type required");

    const entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    entt::basic_snapshot other{std::move(snapshot)};

    ASSERT_NO_THROW(snapshot = std::move(other));
}

TEST(BasicSnapshot, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<entt::entity>(archive);

    ASSERT_EQ(data.size(), 2u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.free_list());

    constexpr auto number_of_entities = 3u;
    std::array<entt::entity, number_of_entities> entity{};

    registry.create(entity.begin(), entity.end());
    registry.destroy(entity[1u]);

    data.clear();
    snapshot.get<entt::entity>(archive);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.free_list());

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
    const entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<int>();
    constexpr auto number_of_entities = 3u;

    std::array<entt::entity, number_of_entities> entity{};
    const std::array value{1, 2, 3};

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), value.begin());
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<int>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), value[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), entity[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[4u]), value[2u]);
}

TEST(BasicSnapshot, GetPointerStableType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<test::pointer_stable>();
    constexpr auto number_of_entities = 3u;

    std::array<entt::entity, number_of_entities> entity{};
    const std::array value{test::pointer_stable{1}, test::pointer_stable{2}, test::pointer_stable{3}};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::pointer_stable>(entity.begin(), entity.end(), value.begin());
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<test::pointer_stable>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<test::pointer_stable>(archive);

    ASSERT_EQ(data.size(), 6u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<test::pointer_stable>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<test::pointer_stable>(data[2u]), value[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), static_cast<entt::entity>(entt::null));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), entity[2u]);

    ASSERT_NE(entt::any_cast<test::pointer_stable>(&data[5u]), nullptr);
    ASSERT_EQ(entt::any_cast<test::pointer_stable>(data[5u]), value[2u]);
}

TEST(BasicSnapshot, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<test::empty>();
    constexpr auto number_of_entities = 3u;

    std::array<entt::entity, number_of_entities> entity{};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<test::empty>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<test::empty>(archive);

    ASSERT_EQ(data.size(), 3u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[2u]), entity[2u]);
}

TEST(BasicSnapshot, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const entt::basic_snapshot snapshot{registry};
    constexpr auto number_of_entities = 3u;

    std::array<entt::entity, number_of_entities> entity{};
    const std::array value{1, 2, 3};

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), value.begin());
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<int>(archive, entity.begin(), entity.end(), "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive, entity.begin(), entity.end());

    ASSERT_EQ(data.size(), 6u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(data.data()), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), static_cast<typename traits_type::entity_type>(std::distance(entity.begin(), entity.end())));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), value[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), static_cast<entt::entity>(entt::null));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), entity[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[5u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[5u]), value[2u]);
}

TEST(BasicSnapshotLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot_loader<entt::registry>>, "Default constructible type not allowed");
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot_loader<entt::registry>>, "Copy constructible type not allowed");
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot_loader<entt::registry>>, "Copy assignable type not allowed");
    static_assert(std::is_move_constructible_v<entt::basic_snapshot_loader<entt::registry>>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<entt::basic_snapshot_loader<entt::registry>>, "Move assignable type required");

    entt::registry registry;

    // helps stress the check in the constructor
    registry.emplace<int>(registry.create(), 0);
    registry.clear();

    entt::basic_snapshot_loader loader{registry};
    entt::basic_snapshot_loader other{std::move(loader)};

    ASSERT_NO_THROW(loader = std::move(other));
}

ENTT_DEBUG_TEST(BasicSnapshotLoaderDeathTest, Constructors) {
    entt::registry registry;
    registry.emplace<int>(registry.create());

    ASSERT_DEATH([[maybe_unused]] const entt::basic_snapshot_loader loader{registry}, "");
}

TEST(BasicSnapshotLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u), traits_type::construct(1u, 1u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.free_list(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_EQ(storage.size(), 3u);
    ASSERT_EQ(storage.free_list(), 2u);

    ASSERT_EQ(storage[0u], entity[0u]);
    ASSERT_EQ(storage[1u], entity[1u]);
    ASSERT_EQ(storage[2u], entity[2u]);

    ASSERT_EQ(registry.create(), entity[2u]);
}

TEST(BasicSnapshotLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const std::array value{1, 3};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    data.emplace_back(entity[1u]);
    data.emplace_back(value[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
    ASSERT_EQ(storage.get(entity[0u]), value[0u]);
    ASSERT_EQ(storage.get(entity[1u]), value[1u]);
}

TEST(BasicSnapshotLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<test::empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);

    loader.get<test::empty>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<test::empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    loader.get<test::empty>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
}

TEST(BasicSnapshotLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const std::array value{1, 3};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entity[1u]);
    data.emplace_back(value[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
    ASSERT_EQ(storage.get(entity[0u]), value[0u]);
    ASSERT_EQ(storage.get(entity[1u]), value[1u]);
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
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 3;

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    loader.orphans();

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
}

TEST(BasicContinuousLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_continuous_loader<entt::registry>>, "Default constructible type not allowed");
    static_assert(!std::is_copy_constructible_v<entt::basic_continuous_loader<entt::registry>>, "Copy constructible type not allowed");
    static_assert(!std::is_copy_assignable_v<entt::basic_continuous_loader<entt::registry>>, "Copy assignable type not allowed");
    static_assert(std::is_move_constructible_v<entt::basic_continuous_loader<entt::registry>>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<entt::basic_continuous_loader<entt::registry>>, "Move assignable type required");

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    entt::basic_continuous_loader other{std::move(loader)};

    ASSERT_NO_THROW(loader = std::move(other));
}

TEST(BasicContinuousLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(1u, 0u), traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_EQ(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.free_list(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_NE(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_EQ(storage.free_list(), 2u);

    ASSERT_EQ(storage[0u], loader.map(entity[0u]));
    ASSERT_EQ(storage[1u], loader.map(entity[1u]));

    ASSERT_EQ(registry.create(), entity[2u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_TRUE(loader.contains(entity[2u]));

    ASSERT_NE(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[2u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.free_list(), 4u);

    ASSERT_EQ(storage[0u], loader.map(entity[0u]));
    ASSERT_EQ(storage[1u], loader.map(entity[1u]));
    ASSERT_EQ(storage[3u], loader.map(entity[2u]));

    registry.destroy(loader.map(entity[1u]));

    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entity[1u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));
    ASSERT_EQ(storage[3u], loader.map(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);
    data.emplace_back(entity[0u]);

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_EQ(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.free_list(), 2u);

    ASSERT_EQ(storage[1u], loader.map(entity[1u]));
}

TEST(BasicContinuousLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const std::array value{1, 3};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    data.emplace_back(entity[1u]);
    data.emplace_back(value[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[0u])), value[0u]);
    ASSERT_EQ(storage.get(loader.map(entity[1u])), value[1u]);
}

TEST(BasicContinuousLoader, GetTypeExtended) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<shadow>();

    std::vector<entt::any> data{};
    const std::array entity{traits_type::construct(0u, 1u), traits_type::construct(1u, 1u)};
    const shadow value{entity[0u]};

    auto archive = [&loader, &data, pos = 0u](auto &elem) mutable {
        elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]);

        if constexpr(std::is_same_v<std::remove_reference_t<decltype(elem)>, shadow>) {
            elem.target = loader.map(elem.target);
        }
    };

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[1u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<shadow>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 1u);
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[1u])).target, loader.map(entity[0u]));
}

TEST(BasicContinuousLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<test::empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);

    loader.get<test::empty>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<test::empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    loader.get<test::empty>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
}

TEST(BasicContinuousLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const std::array value{1, 3};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(value[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entity[1u]);
    data.emplace_back(value[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[0u])), value[0u]);
    ASSERT_EQ(storage.get(loader.map(entity[1u])), value[1u]);
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

TEST(BasicContinuousLoader, Orphans) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const std::array entity{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 3;

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    loader.orphans();

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));
}
