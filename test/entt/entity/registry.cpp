#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include "../../common/aggregate.h"
#include "../../common/config.h"
#include "../../common/empty.h"
#include "../../common/entity.h"
#include "../../common/mixin.hpp"
#include "../../common/non_default_constructible.h"
#include "../../common/pointer_stable.h"

struct no_eto_type {
    static constexpr std::size_t page_size = 1024u;
};

bool operator==(const no_eto_type &lhs, const no_eto_type &rhs) {
    return &lhs == &rhs;
}

struct listener {
    template<typename Type>
    static void sort(entt::registry &registry) {
        registry.sort<Type>([](auto lhs, auto rhs) { return lhs < rhs; });
    }

    void incr(const entt::registry &, entt::entity entity) {
        last = entity;
        ++counter;
    }

    void decr(const entt::registry &, entt::entity entity) {
        last = entity;
        --counter;
    }

    entt::entity last{entt::null};
    int counter{0};
};

struct owner {
    void receive(const entt::registry &ref) {
        parent = &ref;
    }

    const entt::registry *parent{nullptr};
};

struct destruction_order {
    using ctx_check_type = int;

    destruction_order(const entt::registry &ref, bool &ctx)
        : registry{&ref},
          ctx_check{&ctx} {
        *ctx_check = (registry->ctx().find<ctx_check_type>() != nullptr);
    }

    destruction_order(const destruction_order &) = delete;
    destruction_order &operator=(const destruction_order &) = delete;

    ~destruction_order() {
        *ctx_check = *ctx_check && (registry->ctx().find<ctx_check_type>() != nullptr);
    }

private:
    const entt::registry *registry;
    bool *ctx_check{};
};

struct entity_traits {
    using value_type = test::entity;
    using entity_type = uint32_t;
    using version_type = uint16_t;
    static constexpr entity_type entity_mask = 0xFF;
    static constexpr entity_type version_mask = 0x00;
};

template<>
struct entt::entt_traits<test::entity>: entt::basic_entt_traits<entity_traits> {
    static constexpr auto page_size = ENTT_SPARSE_PAGE;
};

TEST(Registry, Functionalities) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = registry.get_allocator());

    ASSERT_EQ(registry.storage<entt::entity>().size(), 0u);
    ASSERT_EQ(registry.storage<entt::entity>().free_list(), 0u);
    ASSERT_NO_THROW(registry.storage<entt::entity>().reserve(4));
    ASSERT_EQ(registry.storage<entt::entity>().capacity(), 4u);
    ASSERT_TRUE(registry.storage<entt::entity>().empty());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_TRUE(registry.storage<int>().empty());
    ASSERT_TRUE(registry.storage<char>().empty());

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    ASSERT_TRUE(registry.all_of<>(e0));
    ASSERT_FALSE(registry.any_of<>(e1));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 1u);
    ASSERT_FALSE(registry.storage<int>().empty());
    ASSERT_FALSE(registry.storage<char>().empty());

    ASSERT_NE(e0, e1);

    ASSERT_FALSE((registry.all_of<int, const char>(e0)));
    ASSERT_TRUE((registry.all_of<const int, char>(e1)));
    ASSERT_FALSE((registry.any_of<int, const double>(e0)));
    ASSERT_TRUE((registry.any_of<const int, double>(e1)));

    ASSERT_EQ(registry.try_get<int>(e0), nullptr);
    ASSERT_NE(registry.try_get<int>(e1), nullptr);
    ASSERT_EQ(registry.try_get<char>(e0), nullptr);
    ASSERT_NE(registry.try_get<char>(e1), nullptr);
    ASSERT_EQ(registry.try_get<double>(e0), nullptr);
    ASSERT_EQ(registry.try_get<double>(e1), nullptr);

    ASSERT_EQ(registry.emplace<int>(e0, 4), 4);
    ASSERT_EQ(registry.emplace<char>(e0, 'c'), 'c');
    ASSERT_NO_THROW(registry.erase<int>(e1));
    ASSERT_NO_THROW(registry.erase<char>(e1));

    ASSERT_TRUE((registry.all_of<const int, char>(e0)));
    ASSERT_FALSE((registry.all_of<int, const char>(e1)));
    ASSERT_TRUE((registry.any_of<const int, double>(e0)));
    ASSERT_FALSE((registry.any_of<int, const double>(e1)));

    const auto e2 = registry.create();

    registry.emplace_or_replace<int>(e2, registry.get<int>(e0));
    registry.emplace_or_replace<char>(e2, registry.get<char>(e0));

    ASSERT_TRUE((registry.all_of<int, char>(e2)));
    ASSERT_EQ(registry.get<int>(e0), 4);
    ASSERT_EQ(registry.get<char>(e0), 'c');

    ASSERT_NE(registry.try_get<int>(e0), nullptr);
    ASSERT_NE(registry.try_get<char>(e0), nullptr);
    ASSERT_EQ(registry.try_get<double>(e0), nullptr);
    ASSERT_EQ(*registry.try_get<int>(e0), 4);
    ASSERT_EQ(*registry.try_get<char>(e0), 'c');

    ASSERT_EQ(std::get<0>(registry.get<int, char>(e0)), 4);
    ASSERT_EQ(*std::get<0>(registry.try_get<int, char, double>(e0)), 4);
    ASSERT_EQ(std::get<1>(static_cast<const entt::registry &>(registry).get<int, char>(e0)), 'c');
    ASSERT_EQ(*std::get<1>(static_cast<const entt::registry &>(registry).try_get<int, char, double>(e0)), 'c');

    ASSERT_EQ(registry.get<int>(e0), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e0), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e0), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e0), &registry.get<char>(e2));

    ASSERT_EQ(registry.patch<int>(e0, [](auto &instance) { instance = 2; }), 2);
    ASSERT_EQ(registry.replace<int>(e0, 3), 3);

    ASSERT_NO_THROW(registry.emplace_or_replace<int>(e0, 1));
    ASSERT_NO_THROW(registry.emplace_or_replace<int>(e1, 1));
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.storage<entt::entity>().size(), 3u);
    ASSERT_EQ(registry.storage<entt::entity>().free_list(), 3u);

    ASSERT_EQ(traits_type::to_version(e2), 0u);
    ASSERT_EQ(registry.current(e2), 0u);
    ASSERT_NO_THROW(registry.destroy(e2));
    ASSERT_EQ(traits_type::to_version(e2), 0u);
    ASSERT_EQ(registry.current(e2), 1u);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));

    ASSERT_EQ(registry.storage<entt::entity>().size(), 3u);
    ASSERT_EQ(registry.storage<entt::entity>().free_list(), 2u);

    ASSERT_NO_THROW(registry.clear());

    ASSERT_EQ(registry.storage<entt::entity>().size(), 3u);
    ASSERT_EQ(registry.storage<entt::entity>().free_list(), 0u);
    ASSERT_FALSE(registry.storage<entt::entity>().empty());

    const auto e3 = registry.create();

    ASSERT_EQ(registry.get_or_emplace<int>(e3, 3), 3);
    ASSERT_EQ(registry.get_or_emplace<char>(e3, 'c'), 'c');

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 1u);
    ASSERT_FALSE(registry.storage<int>().empty());
    ASSERT_FALSE(registry.storage<char>().empty());
    ASSERT_TRUE((registry.all_of<int, char>(e3)));
    ASSERT_EQ(registry.get<int>(e3), 3);
    ASSERT_EQ(registry.get<char>(e3), 'c');

    ASSERT_NO_THROW(registry.clear<int>());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 1u);
    ASSERT_TRUE(registry.storage<int>().empty());
    ASSERT_FALSE(registry.storage<char>().empty());

    ASSERT_NO_THROW(registry.clear());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_TRUE(registry.storage<int>().empty());
    ASSERT_TRUE(registry.storage<char>().empty());

    const auto e4 = registry.create();
    const auto e5 = registry.create();

    registry.emplace<int>(e4);

    ASSERT_EQ(registry.remove<int>(e4), 1u);
    ASSERT_EQ(registry.remove<int>(e5), 0u);

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_TRUE(registry.storage<int>().empty());
}

TEST(Registry, Constructors) {
    entt::registry registry{};
    entt::registry other{64u};

    ASSERT_TRUE(registry.storage<entt::entity>().empty());
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = registry.get_allocator());
    ASSERT_EQ(registry.storage().begin(), registry.storage().end());

    ASSERT_TRUE(other.storage<entt::entity>().empty());
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = other.get_allocator());
    ASSERT_EQ(other.storage().begin(), other.storage().end());
}

TEST(Registry, Move) {
    entt::registry registry{};
    const auto entity = registry.create();
    owner test{};

    registry.on_construct<int>().connect<&owner::receive>(test);
    registry.on_destroy<int>().connect<&owner::receive>(test);

    ASSERT_EQ(test.parent, nullptr);

    registry.emplace<int>(entity);

    ASSERT_EQ(test.parent, &registry);

    entt::registry other{std::move(registry)};
    other.erase<int>(entity);

    registry = {};
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &other);

    registry = std::move(other);
    registry.emplace<int>(entity);
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &registry);
}

TEST(Registry, Swap) {
    entt::registry registry{};
    const auto entity = registry.create();
    owner test{};

    registry.on_construct<int>().connect<&owner::receive>(test);
    registry.on_destroy<int>().connect<&owner::receive>(test);

    ASSERT_EQ(test.parent, nullptr);

    registry.emplace<int>(entity);

    ASSERT_EQ(test.parent, &registry);

    entt::registry other{};
    other.swap(registry);
    other.erase<int>(entity);

    registry = {};
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &other);

    registry.swap(other);
    registry.emplace<int>(entity);
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &registry);
}

TEST(Registry, StorageIterable) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entity = registry.create();

    auto &storage = registry.storage<int>("other"_hs);
    storage.emplace(entity);

    for(auto [id, pool]: registry.storage()) {
        testing::StaticAssertTypeEq<decltype(pool), entt::sparse_set &>();
        testing::StaticAssertTypeEq<decltype(id), entt::id_type>();

        ASSERT_TRUE(pool.contains(entity));
        ASSERT_EQ(&storage, &pool);
        ASSERT_EQ(id, "other"_hs);
    }
}

TEST(Registry, ConstStorageIterable) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entity = registry.create();

    auto &storage = registry.storage<int>("other"_hs);
    storage.emplace(entity);

    for(auto &&curr: std::as_const(registry).storage()) {
        testing::StaticAssertTypeEq<decltype(curr.second), const entt::sparse_set &>();
        testing::StaticAssertTypeEq<decltype(curr.first), entt::id_type>();

        ASSERT_TRUE(curr.second.contains(entity));
        ASSERT_EQ(&storage, &curr.second);
        ASSERT_EQ(curr.first, "other"_hs);
    }
}

TEST(Registry, RegistryStorageIterator) {
    entt::registry registry{};
    const auto entity = registry.create();
    registry.emplace<int>(entity);

    auto iterable = registry.storage();

    auto end{iterable.begin()};
    decltype(end) begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin--, iterable.end());

    ASSERT_EQ(begin + 1, iterable.end());
    ASSERT_EQ(end - 1, iterable.begin());

    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(--begin, iterable.begin());

    ASSERT_EQ(begin += 1, iterable.end());
    ASSERT_EQ(begin -= 1, iterable.begin());

    ASSERT_EQ(begin + (end - begin), iterable.end());
    ASSERT_EQ(begin - (begin - end), iterable.end());

    ASSERT_EQ(end - (end - begin), iterable.begin());
    ASSERT_EQ(end + (begin - end), iterable.begin());

    ASSERT_EQ(begin[0u].first, iterable.begin()->first);
    ASSERT_EQ(&begin[0u].second, &(*iterable.begin()).second);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, iterable.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, iterable.end());

    ASSERT_EQ(begin[0u].first, entt::type_id<int>().hash());
    ASSERT_TRUE(begin[0u].second.contains(entity));
}

TEST(Registry, RegistryConstStorageIterator) {
    entt::registry registry{};
    const auto entity = registry.create();
    registry.emplace<int>(entity);

    auto iterable = std::as_const(registry).storage();

    auto end{iterable.begin()};
    decltype(end) begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin--, iterable.end());

    ASSERT_EQ(begin + 1, iterable.end());
    ASSERT_EQ(end - 1, iterable.begin());

    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(--begin, iterable.begin());

    ASSERT_EQ(begin += 1, iterable.end());
    ASSERT_EQ(begin -= 1, iterable.begin());

    ASSERT_EQ(begin + (end - begin), iterable.end());
    ASSERT_EQ(begin - (begin - end), iterable.end());

    ASSERT_EQ(end - (end - begin), iterable.begin());
    ASSERT_EQ(end + (begin - end), iterable.begin());

    ASSERT_EQ(begin[0u].first, iterable.begin()->first);
    ASSERT_EQ(&begin[0u].second, &(*iterable.begin()).second);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, iterable.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, iterable.end());

    ASSERT_EQ(begin[0u].first, entt::type_id<int>().hash());
    ASSERT_TRUE(begin[0u].second.contains(entity));
}

TEST(Registry, RegistryStorageIteratorConversion) {
    entt::registry registry{};
    registry.storage<int>();

    auto proxy = registry.storage();
    [[maybe_unused]] auto cproxy = std::as_const(registry).storage();

    const typename decltype(proxy)::iterator it = proxy.begin();
    typename decltype(cproxy)::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::pair<entt::id_type, entt::sparse_set &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::pair<entt::id_type, const entt::sparse_set &>>();

    ASSERT_EQ(it->first, entt::type_id<int>().hash());
    ASSERT_EQ((*it).second.type(), entt::type_id<int>());
    ASSERT_EQ(it->first, cit->first);
    ASSERT_EQ((*it).second.type(), (*cit).second.type());

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(Registry, Storage) {
    using namespace entt::literals;

    entt::registry registry{};
    auto &storage = registry.storage<test::empty>("other"_hs);
    const auto entity = registry.create();

    testing::StaticAssertTypeEq<decltype(registry.storage<test::empty>()), entt::storage_type_t<test::empty> &>();
    testing::StaticAssertTypeEq<decltype(registry.storage<const test::empty>()), const entt::storage_type_t<test::empty> &>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).storage<test::empty>()), const entt::storage_type_t<test::empty> *>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).storage<const test::empty>()), const entt::storage_type_t<test::empty> *>();

    testing::StaticAssertTypeEq<decltype(registry.storage("other"_hs)), entt::storage_type_t<test::empty>::base_type *>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).storage("other"_hs)), const entt::storage_type_t<test::empty>::base_type *>();

    ASSERT_EQ(&registry.storage<test::empty>("other"_hs), &storage);
    ASSERT_EQ(&registry.storage<const test::empty>("other"_hs), &storage);
    ASSERT_NE(std::as_const(registry).storage<test::empty>(), &storage);
    ASSERT_NE(std::as_const(registry).storage<const test::empty>(), &storage);

    ASSERT_NE(registry.storage("other"_hs), nullptr);
    ASSERT_EQ(std::as_const(registry).storage("rehto"_hs), nullptr);

    ASSERT_FALSE(registry.any_of<test::empty>(entity));
    ASSERT_FALSE(storage.contains(entity));

    registry.emplace<test::empty>(entity);

    ASSERT_FALSE(storage.contains(entity));
    ASSERT_TRUE(registry.any_of<test::empty>(entity));
    ASSERT_EQ((entt::basic_view{registry.storage<test::empty>(), storage}.size_hint()), 0u);

    storage.emplace(entity);

    ASSERT_TRUE(storage.contains(entity));
    ASSERT_TRUE(registry.any_of<test::empty>(entity));
    ASSERT_EQ((entt::basic_view{registry.storage<test::empty>(), storage}.size_hint()), 1u);

    registry.destroy(entity);

    ASSERT_EQ(registry.create(entity), entity);

    ASSERT_FALSE(storage.contains(entity));
    ASSERT_FALSE(registry.any_of<test::empty>(entity));
}

ENTT_DEBUG_TEST(RegistryDeathTest, Storage) {
    using namespace entt::literals;

    entt::registry registry{};
    registry.storage<test::empty>("other"_hs);

    ASSERT_DEATH(registry.storage<int>("other"_hs), "");
    ASSERT_DEATH([[maybe_unused]] const auto *storage = std::as_const(registry).storage<int>("other"_hs), "");

    ASSERT_DEATH(registry.storage<entt::entity>("other"_hs), "");
    ASSERT_DEATH([[maybe_unused]] const auto *storage = std::as_const(registry).storage<entt::entity>("other"_hs), "");
}

TEST(Registry, StorageReset) {
    using namespace entt::literals;

    entt::registry registry{};
    registry.storage<int>();
    registry.storage<int>("other"_hs);

    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_NE(registry.storage("other"_hs), nullptr);

    ASSERT_EQ(registry.reset("other"_hs), 1u);

    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_EQ(registry.storage("other"_hs), nullptr);

    ASSERT_EQ(registry.reset("other"_hs), 0u);
    ASSERT_EQ(registry.reset(entt::type_id<int>().hash()), 1u);
    ASSERT_EQ(registry.reset(entt::type_id<int>().hash()), 0u);

    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_EQ(registry.storage("other"_hs), nullptr);
}

ENTT_DEBUG_TEST(RegistryDeathTest, StorageReset) {
    entt::registry registry{};
    const entt::entity entity = registry.create();

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_DEATH(registry.reset(entt::type_id<entt::entity>().hash()), "");
    ASSERT_TRUE(registry.valid(entity));
}

TEST(Registry, Identifiers) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    const auto pre = registry.create();

    ASSERT_EQ(traits_type::to_integral(pre), traits_type::to_entity(pre));

    registry.destroy(pre);
    const auto post = registry.create();

    ASSERT_NE(pre, post);
    ASSERT_EQ(traits_type::to_entity(pre), traits_type::to_entity(post));
    ASSERT_NE(traits_type::to_version(pre), traits_type::to_version(post));
    ASSERT_NE(traits_type::to_version(pre), registry.current(pre));
    ASSERT_EQ(traits_type::to_version(post), registry.current(post));

    const auto invalid = traits_type::combine(traits_type::to_entity(post) + 1u, {});

    ASSERT_EQ(traits_type::to_version(invalid), typename traits_type::version_type{});
    ASSERT_EQ(registry.current(invalid), traits_type::to_version(entt::tombstone));
}

TEST(Registry, MoreOnIdentifiers) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    const std::array entity{registry.create(), registry.create()};

    registry.destroy(entity[0]);

    ASSERT_FALSE(registry.valid(entity[0]));
    ASSERT_TRUE(registry.valid(entity[1]));

    ASSERT_EQ(registry.current(entt::null), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(registry.current(entity[0]), traits_type::to_version(entity[0]) + 1u);
    ASSERT_EQ(registry.current(entity[1]), traits_type::to_version(entity[1]));

    registry.destroy(entity[1]);

    ASSERT_FALSE(registry.valid(entity[1]));
    ASSERT_EQ(registry.current(entity[1]), traits_type::to_version(entity[1]) + 1u);
}

TEST(Registry, VersionOverflow) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    const auto entity = registry.create();

    registry.destroy(entity);

    ASSERT_NE(registry.current(entity), traits_type::to_version(entity));
    ASSERT_NE(registry.current(entity), typename traits_type::version_type{});

    registry.destroy(registry.create(), traits_type::to_version(entt::tombstone) - 1u);
    registry.destroy(registry.create());

    ASSERT_EQ(registry.current(entity), traits_type::to_version(entity));
    ASSERT_EQ(registry.current(entity), typename traits_type::version_type{});
}

TEST(Registry, NullEntity) {
    entt::registry registry{};
    const entt::entity entity = entt::null;

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_NE(registry.create(entity), entity);
}

TEST(Registry, TombstoneVersion) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    const entt::entity entity = entt::tombstone;

    ASSERT_FALSE(registry.valid(entity));

    const auto other = registry.create();
    const auto vers = traits_type::to_version(entity);
    const auto required = traits_type::construct(traits_type::to_entity(other), vers);

    ASSERT_NE(registry.destroy(other, vers), vers);
    ASSERT_NE(registry.create(required), required);
}

TEST(Registry, CreateManyEntitiesAtOnce) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    const auto entt = registry.create();
    registry.destroy(registry.create());
    registry.destroy(entt);
    registry.destroy(registry.create());

    registry.create(entity.begin(), entity.end());

    ASSERT_TRUE(registry.valid(entity[0]));
    ASSERT_TRUE(registry.valid(entity[1]));
    ASSERT_TRUE(registry.valid(entity[2]));

    ASSERT_EQ(traits_type::to_entity(entity[0]), 0u);
    ASSERT_EQ(traits_type::to_version(entity[0]), 2u);

    ASSERT_EQ(traits_type::to_entity(entity[1]), 1u);
    ASSERT_EQ(traits_type::to_version(entity[1]), 1u);

    ASSERT_EQ(traits_type::to_entity(entity[2]), 2u);
    ASSERT_EQ(traits_type::to_version(entity[2]), 0u);
}

TEST(Registry, CreateManyEntitiesAtOnceWithListener) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};
    listener listener;

    registry.on_construct<int>().connect<&listener::incr>(listener);
    registry.create(entity.begin(), entity.end());
    registry.insert(entity.begin(), entity.end(), 1);
    registry.insert(entity.begin(), entity.end(), 'c');

    ASSERT_EQ(registry.get<int>(entity[0]), 1);
    ASSERT_EQ(registry.get<char>(entity[1]), 'c');
    ASSERT_EQ(listener.counter, 3);

    registry.on_construct<int>().disconnect<&listener::incr>(listener);
    registry.on_construct<test::empty>().connect<&listener::incr>(listener);
    registry.create(entity.begin(), entity.end());
    registry.insert(entity.begin(), entity.end(), 'a');
    registry.insert<test::empty>(entity.begin(), entity.end());

    ASSERT_TRUE(registry.all_of<test::empty>(entity[0]));
    ASSERT_EQ(registry.get<char>(entity[2]), 'a');
    ASSERT_EQ(listener.counter, 6);
}

TEST(Registry, CreateWithHint) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    auto e3 = registry.create(entt::entity{3});
    auto e2 = registry.create(entt::entity{3});

    ASSERT_EQ(e2, entt::entity{0});
    ASSERT_FALSE(registry.valid(entt::entity{1}));
    ASSERT_FALSE(registry.valid(entt::entity{2}));
    ASSERT_EQ(e3, entt::entity{3});

    registry.destroy(e2);

    ASSERT_EQ(traits_type::to_version(e2), 0u);
    ASSERT_EQ(registry.current(e2), 1u);

    e2 = registry.create();
    auto e1 = registry.create(entt::entity{2});

    ASSERT_EQ(traits_type::to_entity(e2), 0u);
    ASSERT_EQ(traits_type::to_version(e2), 1u);

    ASSERT_EQ(traits_type::to_entity(e1), 2u);
    ASSERT_EQ(traits_type::to_version(e1), 0u);

    registry.destroy(e1);
    registry.destroy(e2);
    auto e0 = registry.create(entt::entity{0});

    ASSERT_EQ(e0, entt::entity{0});
    ASSERT_EQ(traits_type::to_version(e0), 0u);
}

TEST(Registry, CreateClearCycle) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    entt::entity pre = entt::null;
    entt::entity post = entt::null;

    const std::size_t first_iteration = 10u;
    const std::size_t second_iteration = 7u;
    const std::size_t third_iteration = 5u;

    for(std::size_t i{}; i < first_iteration; ++i) {
        const auto entity = registry.create();
        registry.emplace<double>(entity);
    }

    registry.clear();

    for(std::size_t i{}; i < second_iteration; ++i) {
        const auto entity = registry.create();
        registry.emplace<int>(entity);

        if(i == 3u) {
            pre = entity;
        }
    }

    registry.clear();

    for(std::size_t i{}; i < third_iteration; ++i) {
        const auto entity = registry.create();

        if(i == 3u) {
            post = entity;
        }
    }

    ASSERT_FALSE(registry.valid(pre));
    ASSERT_TRUE(registry.valid(post));
    ASSERT_NE(traits_type::to_version(pre), traits_type::to_version(post));
    ASSERT_EQ(traits_type::to_version(pre) + 1, traits_type::to_version(post));
    ASSERT_EQ(registry.current(pre), registry.current(post));
}

TEST(Registry, CreateDestroyReleaseCornerCase) {
    entt::registry registry{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.destroy(e0);
    registry.storage<entt::entity>().erase(e1);

    ASSERT_EQ(registry.storage<entt::entity>().free_list(), 0u);

    ASSERT_EQ(registry.current(e0), 1u);
    ASSERT_EQ(registry.current(e1), 1u);
}

ENTT_DEBUG_TEST(RegistryDeathTest, CreateTooManyEntities) {
    entt::basic_registry<test::entity> registry{};
    std::vector<test::entity> entity(entt::entt_traits<test::entity>::to_entity(entt::null));
    registry.create(entity.begin(), entity.end());

    ASSERT_DEATH([[maybe_unused]] const auto entt = registry.create(), "");
}

TEST(Registry, DestroyVersion) {
    entt::registry registry{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_EQ(registry.current(e0), 0u);
    ASSERT_EQ(registry.current(e1), 0u);

    registry.destroy(e0);
    registry.destroy(e1, 3);

    ASSERT_EQ(registry.current(e0), 1u);
    ASSERT_EQ(registry.current(e1), 3u);
}

ENTT_DEBUG_TEST(RegistryDeathTest, DestroyVersion) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.destroy(entity);

    ASSERT_DEATH(registry.destroy(entity), "");
    ASSERT_DEATH(registry.destroy(entity, 3), "");
}

TEST(Registry, DestroyRange) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, char>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<char>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<char>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));
    ASSERT_TRUE(registry.valid(entity[2u]));

    registry.destroy(icview.begin(), icview.end());

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_TRUE(registry.valid(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 0u);

    registry.destroy(iview.begin(), iview.end());

    ASSERT_FALSE(registry.valid(entity[2u]));
    ASSERT_NO_THROW(registry.destroy(iview.rbegin(), iview.rend()));
    ASSERT_EQ(iview.size(), 0u);
    ASSERT_EQ(icview.size_hint(), 0u);

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 0u);

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end());

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));
    ASSERT_TRUE(registry.valid(entity[2u]));
    ASSERT_EQ(registry.storage<int>().size(), 3u);

    registry.destroy(entity.begin(), entity.end());

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));
    ASSERT_EQ(registry.storage<int>().size(), 0u);

    entt::sparse_set managed{};

    registry.create(entity.begin(), entity.end());
    managed.push(entity.begin(), entity.end());
    registry.insert<int>(managed.begin(), managed.end());

    ASSERT_TRUE(registry.valid(managed[0u]));
    ASSERT_TRUE(registry.valid(managed[1u]));
    ASSERT_TRUE(registry.valid(managed[2u]));
    ASSERT_EQ(registry.storage<int>().size(), 3u);

    registry.destroy(managed.begin(), managed.end());

    ASSERT_FALSE(registry.valid(managed[0u]));
    ASSERT_FALSE(registry.valid(managed[1u]));
    ASSERT_FALSE(registry.valid(managed[2u]));
    ASSERT_EQ(registry.storage<int>().size(), 0u);
}

TEST(Registry, StableDestroy) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, test::pointer_stable>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<test::pointer_stable>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<test::pointer_stable>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));
    ASSERT_TRUE(registry.valid(entity[2u]));

    registry.destroy(icview.begin(), icview.end());

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_TRUE(registry.valid(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 0u);

    registry.destroy(iview.begin(), iview.end());

    ASSERT_FALSE(registry.valid(entity[2u]));
    ASSERT_EQ(iview.size(), 0u);
    ASSERT_EQ(icview.size_hint(), 0u);

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 0u);
}

TEST(Registry, Emplace) {
    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<int>(entity));

    const auto &ref = registry.emplace<int>(entity, 4);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), ref);
    ASSERT_EQ(ref, 4);
}

TEST(Registry, EmplaceTypesFromStandardTemplateLibrary) {
    // see #37 - the test shouldn't crash, that's all
    entt::registry registry{};
    const auto entity = registry.create();
    registry.emplace<std::unordered_set<int>>(entity).insert(1);
    registry.destroy(entity);
}

TEST(Registry, EmplaceWithComponents) {
    // it should compile, that's all
    entt::registry registry{};
    const auto value = 0;
    registry.emplace<int>(registry.create(), value);
}

TEST(Registry, EmplaceMoveOnlyComponent) {
    entt::registry registry{};
    // the purpose is to ensure that move only types are always accepted
    registry.emplace<std::unique_ptr<int>>(registry.create());
}

TEST(Registry, EmplaceNonDefaultConstructibleComponent) {
    entt::registry registry{};
    // the purpose is to ensure that non default constructible type are always accepted
    registry.emplace<test::non_default_constructible>(registry.create(), 1);
}

TEST(Registry, StableEmplace) {
    entt::registry registry{};
    registry.on_construct<int>().connect<&listener::sort<int>>();
    registry.emplace<int>(registry.create(), 0);

    ASSERT_EQ(registry.emplace<int>(registry.create(), 1), 1);
}

ENTT_DEBUG_TEST(RegistryDeathTest, Emplace) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.destroy(entity);

    ASSERT_DEATH(registry.emplace<int>(entity), "");
}

TEST(Registry, Insert) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<char>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<char>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_FALSE(registry.all_of<float>(entity[0u]));
    ASSERT_FALSE(registry.all_of<float>(entity[1u]));
    ASSERT_FALSE(registry.all_of<float>(entity[2u]));

    const auto icview = registry.view<int, char>();
    registry.insert(icview.begin(), icview.end(), 3.f);

    ASSERT_EQ(registry.get<float>(entity[0u]), 3.f);
    ASSERT_EQ(registry.get<float>(entity[1u]), 3.f);
    ASSERT_FALSE(registry.all_of<float>(entity[2u]));

    registry.clear<float>();
    std::array value{0.f, 1.f, 2.f};

    const auto iview = registry.view<int>();
    registry.insert<float>(iview.rbegin(), iview.rend(), value.begin());

    ASSERT_EQ(registry.get<float>(entity[0u]), 0.f);
    ASSERT_EQ(registry.get<float>(entity[1u]), 1.f);
    ASSERT_EQ(registry.get<float>(entity[2u]), 2.f);
}

ENTT_DEBUG_TEST(RegistryDeathTest, Insert) {
    entt::registry registry{};
    const std::array entity{registry.create()};
    const std::array value{0};

    registry.destroy(entity[0]);

    ASSERT_DEATH(registry.insert<int>(entity.begin(), entity.end(), value[0]), "");
    ASSERT_DEATH(registry.insert<int>(entity.begin(), entity.end(), value.begin()), "");
}

TEST(Registry, EmplaceOrReplace) {
    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<int>(entity));

    const auto &ref = registry.emplace_or_replace<int>(entity, 4);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), ref);
    ASSERT_EQ(ref, 4);

    registry.emplace_or_replace<int>(entity, 0);

    ASSERT_EQ(ref, 0);
}

TEST(Registry, EmplaceOrReplaceAggregate) {
    entt::registry registry{};
    const auto entity = registry.create();
    auto &instance = registry.emplace_or_replace<test::aggregate>(entity, 1);

    ASSERT_EQ(instance.value, 1);
}

ENTT_DEBUG_TEST(RegistryDeathTest, EmplaceOrReplace) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.destroy(entity);

    ASSERT_DEATH(registry.emplace_or_replace<int>(entity), "");
}

TEST(Registry, Patch) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<int>(entity, 3);

    ASSERT_EQ(registry.get<int>(entity), 3);

    registry.patch<int>(entity);

    ASSERT_EQ(registry.get<int>(entity), 3);

    registry.patch<int>(entity, [](auto &value) { value = 1; });

    ASSERT_EQ(registry.get<int>(entity), 1);
}

TEST(Registry, Replace) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<int>(entity, 3);

    ASSERT_EQ(registry.get<int>(entity), 3);

    registry.replace<int>(entity);

    ASSERT_EQ(registry.get<int>(entity), 0);

    registry.replace<int>(entity, 1);

    ASSERT_EQ(registry.get<int>(entity), 1);
}

TEST(Registry, ReplaceAggregate) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<test::aggregate>(entity, 0);
    auto &instance = registry.replace<test::aggregate>(entity, 1);

    ASSERT_EQ(instance.value, 1);
}

TEST(Registry, Remove) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, char>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<char>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<char>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.any_of<int>(entity[0u]));
    ASSERT_TRUE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    registry.remove<int, char>(entity[0u]);

    ASSERT_EQ((registry.remove<int, char>(icview.begin(), icview.end())), 2u);
    ASSERT_EQ((registry.remove<int, char>(icview.begin(), icview.end())), 0u);

    ASSERT_FALSE(registry.any_of<int>(entity[0u]));
    ASSERT_FALSE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    ASSERT_EQ((registry.remove<int>(iview.begin(), iview.end())), 1u);

    ASSERT_EQ(registry.remove<int>(entity[0u]), 0u);
    ASSERT_EQ(registry.remove<int>(entity[1u]), 0u);

    ASSERT_FALSE(registry.any_of<int>(entity[2u]));
    ASSERT_EQ(registry.remove<int>(iview.begin(), iview.end()), 0u);

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    registry.insert<int>(entity.begin() + 1u, entity.end() - 1u);
    registry.insert<char>(entity.begin() + 1u, entity.end() - 1u);

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 1u);

    registry.remove<int, char>(iview.begin(), iview.end());
    registry.remove<int, char>(iview.begin(), iview.end());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);

    registry.insert<int>(entity.begin(), entity.end());
    registry.insert<char>(entity.begin(), entity.end());

    ASSERT_EQ(registry.storage<int>().size(), 3u);
    ASSERT_EQ(registry.storage<char>().size(), 3u);

    registry.remove<int, char>(entity.begin(), entity.end());
    registry.remove<int, char>(entity.begin(), entity.end());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);

    ASSERT_FALSE(registry.orphan(entity[0u]));
    ASSERT_TRUE(registry.orphan(entity[1u]));
    ASSERT_TRUE(registry.orphan(entity[2u]));
}

TEST(Registry, StableRemove) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, test::pointer_stable>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<test::pointer_stable>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<test::pointer_stable>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.any_of<int>(entity[0u]));
    ASSERT_TRUE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    registry.remove<int, test::pointer_stable>(entity[0u]);

    ASSERT_EQ((registry.remove<int, test::pointer_stable>(icview.begin(), icview.end())), 2u);
    ASSERT_EQ((registry.remove<int, test::pointer_stable>(icview.begin(), icview.end())), 0u);

    ASSERT_FALSE(registry.any_of<int>(entity[0u]));
    ASSERT_FALSE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    ASSERT_EQ((registry.remove<int>(iview.begin(), iview.end())), 1u);

    ASSERT_EQ(registry.remove<int>(entity[0u]), 0u);
    ASSERT_EQ(registry.remove<int>(entity[1u]), 0u);

    ASSERT_FALSE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);
}

TEST(Registry, Erase) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, char>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<char>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<char>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.any_of<int>(entity[0u]));
    ASSERT_TRUE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    registry.erase<int, char>(entity[0u]);
    registry.erase<int, char>(icview.begin(), icview.end());

    ASSERT_FALSE(registry.any_of<int>(entity[0u]));
    ASSERT_FALSE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    registry.erase<int>(iview.begin(), iview.end());

    ASSERT_FALSE(registry.any_of<int>(entity[2u]));
    ASSERT_NO_THROW(registry.erase<int>(iview.rbegin(), iview.rend()));

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    registry.insert<int>(entity.begin() + 1u, entity.end() - 1u);
    registry.insert<char>(entity.begin() + 1u, entity.end() - 1u);

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<char>().size(), 1u);

    registry.erase<int, char>(iview.begin(), iview.end());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);

    registry.insert<int>(entity.begin(), entity.end());
    registry.insert<char>(entity.begin(), entity.end());

    ASSERT_EQ(registry.storage<int>().size(), 3u);
    ASSERT_EQ(registry.storage<char>().size(), 3u);

    registry.erase<int, char>(entity.begin(), entity.end());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<char>().size(), 0u);

    ASSERT_FALSE(registry.orphan(entity[0u]));
    ASSERT_TRUE(registry.orphan(entity[1u]));
    ASSERT_TRUE(registry.orphan(entity[2u]));
}

ENTT_DEBUG_TEST(RegistryDeathTest, Erase) {
    entt::registry registry{};
    const std::array entity{registry.create()};

    ASSERT_FALSE((registry.any_of<int>(entity[0u])));
    ASSERT_DEATH((registry.erase<int>(entity.begin(), entity.end())), "");
    ASSERT_DEATH(registry.erase<int>(entity[0u]), "");
}

TEST(Registry, StableErase) {
    entt::registry registry{};
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, test::pointer_stable>();
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<test::pointer_stable>(entity[0u]);
    registry.emplace<double>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<test::pointer_stable>(entity[1u]);

    registry.emplace<int>(entity[2u]);

    ASSERT_TRUE(registry.any_of<int>(entity[0u]));
    ASSERT_TRUE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    registry.erase<int, test::pointer_stable>(entity[0u]);
    registry.erase<int, test::pointer_stable>(icview.begin(), icview.end());
    registry.erase<int, test::pointer_stable>(icview.begin(), icview.end());

    ASSERT_FALSE(registry.any_of<int>(entity[0u]));
    ASSERT_FALSE(registry.all_of<int>(entity[1u]));
    ASSERT_TRUE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 1u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);

    registry.erase<int>(iview.begin(), iview.end());

    ASSERT_FALSE(registry.any_of<int>(entity[2u]));

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);
    ASSERT_EQ(registry.storage<double>().size(), 1u);
}

TEST(Registry, EraseIf) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.storage<int>("other"_hs).emplace(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(registry.storage<int>().contains(entity));
    ASSERT_TRUE(registry.storage<int>("other"_hs).contains(entity));
    ASSERT_TRUE(registry.storage<char>().contains(entity));

    registry.erase_if(entity, [](auto &&...) { return false; });

    ASSERT_TRUE(registry.storage<int>().contains(entity));
    ASSERT_TRUE(registry.storage<int>("other"_hs).contains(entity));
    ASSERT_TRUE(registry.storage<char>().contains(entity));

    registry.erase_if(entity, [](entt::id_type id, auto &&...) { return id == "other"_hs; });

    ASSERT_TRUE(registry.storage<int>().contains(entity));
    ASSERT_FALSE(registry.storage<int>("other"_hs).contains(entity));
    ASSERT_TRUE(registry.storage<char>().contains(entity));

    registry.erase_if(entity, [](auto, const auto &storage) { return storage.type() == entt::type_id<char>(); });

    ASSERT_TRUE(registry.storage<int>().contains(entity));
    ASSERT_FALSE(registry.storage<int>("other"_hs).contains(entity));
    ASSERT_FALSE(registry.storage<char>().contains(entity));
}

TEST(Registry, Compact) {
    entt::registry registry{};
    std::array<entt::entity, 2u> entity{};

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u]);
    registry.emplace<test::pointer_stable>(entity[0u]);

    registry.emplace<int>(entity[1u]);
    registry.emplace<test::pointer_stable>(entity[1u]);

    ASSERT_EQ(registry.storage<int>().size(), 2u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);

    registry.destroy(entity.begin(), entity.end());

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);

    registry.compact<int>();

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 2u);

    registry.compact();

    ASSERT_EQ(registry.storage<int>().size(), 0u);
    ASSERT_EQ(registry.storage<test::pointer_stable>().size(), 0u);
}

TEST(Registry, AllAnyOf) {
    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<int>(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
    ASSERT_FALSE((registry.any_of<int, char>(entity)));

    registry.emplace<int>(entity);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
    ASSERT_TRUE((registry.any_of<int, char>(entity)));

    registry.emplace<char>(entity);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));
    ASSERT_TRUE((registry.any_of<int, char>(entity)));
}

TEST(Registry, Get) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<int>(entity, 1);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(std::as_const(registry).get<int>(entity), 1);
    ASSERT_EQ((std::as_const(registry).get<int, char>(entity)), std::make_tuple(1, 'c'));

    registry.get<int>(entity) = 3;
    std::get<1>(registry.get<int, char>(entity)) = 'a';

    ASSERT_EQ(registry.get<int>(entity), 3);
    ASSERT_EQ((registry.get<int, char>(entity)), std::make_tuple(3, 'a'));
}

TEST(Registry, GetOrEmplace) {
    entt::registry registry{};
    const auto entity = registry.create();
    const auto value = registry.get_or_emplace<int>(entity, 3);

    // get_or_emplace must work for empty types
    // NOLINTNEXTLINE(readability-redundant-casting)
    static_cast<void>(registry.get_or_emplace<test::empty>(entity));

    ASSERT_TRUE((registry.all_of<int, test::empty>(entity)));
    ASSERT_EQ(registry.get<int>(entity), value);
    ASSERT_EQ(registry.get<int>(entity), 3);
}

ENTT_DEBUG_TEST(RegistryDeathTest, GetOrEmplace) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.destroy(entity);

    ASSERT_DEATH([[maybe_unused]] const auto value = registry.get_or_emplace<int>(entity), "");
}

TEST(Registry, TryGet) {
    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ(registry.try_get<int>(entity), nullptr);
    ASSERT_EQ(std::as_const(registry).try_get<int>(entity), nullptr);

    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);

    const int &elem = registry.emplace<int>(entity);

    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);

    ASSERT_EQ(registry.try_get<int>(entity), &elem);
    ASSERT_EQ(std::as_const(registry).try_get<int>(entity), &elem);
}

TEST(Registry, Clear) {
    entt::registry registry{};
    const std::array entity{registry.create(), registry.create()};

    registry.insert<int>(entity.begin(), entity.end());
    registry.insert<char>(entity.begin(), entity.end());

    ASSERT_TRUE((registry.all_of<int, char>(entity[0])));
    ASSERT_TRUE((registry.all_of<int, char>(entity[1])));

    registry.clear<int>();

    ASSERT_TRUE(registry.valid(entity[0]));
    ASSERT_TRUE(registry.valid(entity[1]));

    ASSERT_FALSE(registry.all_of<int>(entity[0]));
    ASSERT_FALSE(registry.all_of<int>(entity[1]));

    ASSERT_TRUE(registry.all_of<char>(entity[0]));
    ASSERT_TRUE(registry.all_of<char>(entity[1]));

    registry.clear();

    ASSERT_FALSE(registry.valid(entity[0]));
    ASSERT_FALSE(registry.valid(entity[1]));

    ASSERT_TRUE(registry.storage<int>().empty());
    ASSERT_TRUE(registry.storage<char>().empty());
}

TEST(Registry, Orphan) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());
    registry.emplace<int>(entity[0u]);
    registry.emplace<int>(entity[2u]);

    for(auto [entt]: registry.storage<entt::entity>().each()) {
        ASSERT_TRUE(entt != entity[1u] || registry.orphan(entt));
    }

    registry.erase<int>(entity[0u]);
    registry.erase<int>(entity[2u]);

    for(auto [entt]: registry.storage<entt::entity>().each()) {
        ASSERT_TRUE(registry.orphan(entt));
    }
}

TEST(Registry, Signals) {
    entt::registry registry{};
    std::array<entt::entity, 2u> entity{};
    listener listener;

    registry.on_construct<test::empty>().connect<&listener::incr>(listener);
    registry.on_destroy<test::empty>().connect<&listener::decr>(listener);
    registry.on_construct<int>().connect<&listener::incr>(listener);
    registry.on_destroy<int>().connect<&listener::decr>(listener);

    registry.create(entity.begin(), entity.end());
    registry.insert<test::empty>(entity.begin(), entity.end());

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[1u]);

    registry.insert<int>(entity.rbegin(), entity.rend());

    ASSERT_EQ(listener.counter, 4);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.erase<test::empty, int>(entity[0u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.on_destroy<test::empty>().disconnect<&listener::decr>(listener);
    registry.on_destroy<int>().disconnect<&listener::decr>(listener);

    registry.erase<test::empty, int>(entity[1u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.on_construct<test::empty>().disconnect<&listener::incr>(listener);
    registry.on_construct<int>().disconnect<&listener::incr>(listener);

    registry.emplace<test::empty>(entity[1u]);
    registry.emplace<int>(entity[1u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.on_construct<int>().connect<&listener::incr>(listener);
    registry.on_destroy<int>().connect<&listener::decr>(listener);

    registry.emplace<int>(entity[0u]);
    registry.erase<int>(entity[1u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[1u]);

    registry.on_construct<test::empty>().connect<&listener::incr>(listener);
    registry.on_destroy<test::empty>().connect<&listener::decr>(listener);

    registry.erase<test::empty>(entity[1u]);
    registry.emplace<test::empty>(entity[0u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.clear<test::empty, int>();

    ASSERT_EQ(listener.counter, 0);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.insert<test::empty>(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end());
    registry.destroy(entity[1u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[1u]);

    registry.erase<int, test::empty>(entity[0u]);
    registry.emplace_or_replace<int>(entity[0u]);
    registry.emplace_or_replace<test::empty>(entity[0u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.on_destroy<test::empty>().disconnect<&listener::decr>(listener);
    registry.on_destroy<int>().disconnect<&listener::decr>(listener);

    registry.emplace_or_replace<test::empty>(entity[0u]);
    registry.emplace_or_replace<int>(entity[0u]);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.on_update<test::empty>().connect<&listener::incr>(listener);
    registry.on_update<int>().connect<&listener::incr>(listener);

    registry.emplace_or_replace<test::empty>(entity[0u]);
    registry.emplace_or_replace<int>(entity[0u]);

    ASSERT_EQ(listener.counter, 4);
    ASSERT_EQ(listener.last, entity[0u]);

    registry.replace<test::empty>(entity[0u]);
    registry.replace<int>(entity[0u]);

    ASSERT_EQ(listener.counter, 6);
    ASSERT_EQ(listener.last, entity[0u]);
}

TEST(Registry, SignalsOnRuntimePool) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entity = registry.create();
    listener listener;

    registry.on_construct<int>("custom"_hs).connect<&listener::incr>(listener);
    registry.on_update<int>("custom"_hs).connect<&listener::incr>(listener);
    registry.on_destroy<int>("custom"_hs).connect<&listener::incr>(listener);

    ASSERT_EQ(listener.counter, 0);

    registry.emplace<int>(entity);
    registry.patch<int>(entity);
    registry.erase<int>(entity);

    ASSERT_EQ(listener.counter, 0);

    registry.storage<int>("custom"_hs).emplace(entity);
    registry.storage<int>("custom"_hs).patch(entity);
    registry.storage<int>("custom"_hs).erase(entity);

    ASSERT_EQ(listener.counter, 3);
}

TEST(Registry, SignalsOnEntity) {
    entt::registry registry{};
    listener listener;

    registry.on_construct<entt::entity>().connect<&listener::incr>(listener);

    entt::entity entity = registry.create();
    entt::entity other = registry.create();

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, other);

    registry.destroy(other);
    registry.destroy(entity);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, other);

    registry.on_construct<entt::entity>().disconnect(&listener);

    other = registry.create();
    entity = registry.create();

    ASSERT_EQ(listener.counter, 2);
    ASSERT_NE(listener.last, entity);
    ASSERT_NE(listener.last, other);

    registry.on_update<entt::entity>().connect<&listener::decr>(listener);
    registry.patch<entt::entity>(entity);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, entity);

    registry.on_update<entt::entity>().disconnect(&listener);
    registry.patch<entt::entity>(other);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_NE(listener.last, other);

    registry.on_destroy<entt::entity>().connect<&listener::decr>(listener);
    registry.destroy(entity);

    ASSERT_EQ(listener.counter, 0);
    ASSERT_EQ(listener.last, entity);

    registry.on_destroy<entt::entity>().disconnect(&listener);
    registry.destroy(other);

    ASSERT_EQ(listener.counter, 0);
    ASSERT_NE(listener.last, other);
}

TEST(Registry, SignalWhenDestroying) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.on_destroy<double>().connect<&entt::registry::remove<char>>();
    registry.emplace<double>(entity);
    registry.emplace<int>(entity);

    ASSERT_NE(registry.storage(entt::type_id<double>().hash()), nullptr);
    ASSERT_NE(registry.storage(entt::type_id<int>().hash()), nullptr);
    ASSERT_EQ(registry.storage(entt::type_id<char>().hash()), nullptr);
    ASSERT_TRUE(registry.valid(entity));

    registry.destroy(entity);

    ASSERT_NE(registry.storage(entt::type_id<char>().hash()), nullptr);
    ASSERT_FALSE(registry.valid(entity));
}

TEST(Registry, SelfSignal) {
    entt::registry registry{};
    const auto entity = registry.create();

    // required because of an issue of VS2019
    constexpr auto emplace_or_replace = &entt::registry::emplace_or_replace<double>;
    constexpr auto remove = &entt::registry::remove<double>;

    registry.on_construct<int>().connect<emplace_or_replace>();
    registry.on_destroy<int>().connect<remove>();
    registry.emplace<double>(entity, .3);

    ASSERT_FALSE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);

    registry.emplace<int>(entity);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);

    registry.erase<int>(entity);

    ASSERT_FALSE((registry.any_of<int, double>(entity)));

    registry.on_construct<int>().disconnect<emplace_or_replace>();
    registry.on_destroy<int>().disconnect<remove>();
    registry.emplace<int>(entity);

    ASSERT_TRUE((registry.any_of<int, double>(entity)));
    ASSERT_FALSE(registry.all_of<double>(entity));
}

TEST(Registry, View) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    auto iview = std::as_const(registry).view<int>();
    auto cview = std::as_const(registry).view<char>();

    ASSERT_FALSE(iview);
    ASSERT_FALSE(cview);

    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<char>(), nullptr);

    auto mview = registry.view<int, char>();
    auto fview = registry.view<int>(entt::exclude<char>);

    ASSERT_TRUE(mview);
    ASSERT_TRUE(fview);

    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_NE(std::as_const(registry).storage<char>(), nullptr);

    iview = std::as_const(registry).view<int>();
    cview = std::as_const(registry).view<char>();

    ASSERT_TRUE(iview);
    ASSERT_TRUE(cview);

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], 'c');

    registry.emplace<int>(entity[1u], 0);

    registry.emplace<int>(entity[2u], 0);
    registry.emplace<char>(entity[2u], 'c');

    ASSERT_EQ(iview.size(), 3u);
    ASSERT_EQ(cview.size(), 2u);

    ASSERT_EQ(mview.size_hint(), 3u);
    ASSERT_EQ(fview.size_hint(), 3u);

    mview.refresh();

    ASSERT_EQ(mview.size_hint(), 2u);
    ASSERT_EQ(fview.size_hint(), 3u);

    ASSERT_NE(mview.begin(), mview.end());
    ASSERT_NE(fview.begin(), fview.end());

    ASSERT_EQ(std::distance(mview.begin(), mview.end()), 2);
    ASSERT_EQ(std::distance(fview.begin(), fview.end()), 1);

    mview.each([&entity, first = true](auto entt, auto &&...) mutable {
        ASSERT_EQ(entt, first ? entity[2u] : entity[0u]);
        first = false;
    });

    fview.each([&entity](auto entt, auto &&...) {
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(Registry, ExcludeOnlyView) {
    entt::registry registry{};
    std::array<entt::entity, 4u> entity{};

    auto view = registry.view<entt::entity>(entt::exclude<int>);

    registry.create(entity.begin(), entity.end());

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<int>(entity[2u], 0);
    registry.emplace<int>(entity[3u], 0);

    registry.destroy(entity[3u]);

    ASSERT_EQ(view.size_hint(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(*view.begin(), entity[1u]);

    for(auto [entt]: view.each()) {
        ASSERT_EQ(entt, entity[1u]);
    }

    view.each([&entity](auto entt) {
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(Registry, CleanViewAfterRemoveAndClear) {
    entt::registry registry{};
    auto view = registry.view<int, char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.erase<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.clear<int>();

    ASSERT_EQ(view.size_hint(), 0u);

    registry.emplace<int>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.clear();

    ASSERT_EQ(view.size_hint(), 0u);
}

TEST(Registry, NonOwningGroupInitOnFirstUse) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    auto group = registry.group(entt::get<int, char>);
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE((registry.owned<int, char>()));
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, NonOwningGroupInitOnEmplace) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};
    auto group = registry.group(entt::get<int, char>);

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE((registry.owned<int, char>()));
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInitOnFirstUse) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    auto group = registry.group<int, char>();
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_TRUE(registry.owned<char>());
    ASSERT_FALSE(registry.owned<double>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInitOnEmplace) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};
    auto group = registry.group<int, char>();

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_TRUE(registry.owned<char>());
    ASSERT_FALSE(registry.owned<double>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, PartialOwningGroupInitOnFirstUse) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    auto group = registry.group<int>(entt::get<char>);
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE((registry.owned<int, char>()));
    ASSERT_TRUE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, PartialOwningGroupInitOnEmplace) {
    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};
    auto group = registry.group<int>(entt::get<char>);

    registry.create(entity.begin(), entity.end());
    registry.insert<int>(entity.begin(), entity.end(), 0);
    registry.emplace<char>(entity[0u], 'c');
    registry.emplace<char>(entity[2u], 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE((registry.owned<int, char>()));
    ASSERT_TRUE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, CleanNonOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry{};
    auto group = registry.group(entt::get<int, char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.erase<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

TEST(Registry, CleanFullOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry{};
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.erase<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

TEST(Registry, CleanPartialOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry{};
    auto group = registry.group<int>(entt::get<char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.erase<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

ENTT_DEBUG_TEST(RegistryDeathTest, NestedGroups) {
    entt::registry registry{};
    registry.group<int, double>(entt::get<char>);

    ASSERT_DEATH(registry.group<int>(entt::get<char>), "");
    ASSERT_DEATH(registry.group<int>(entt::get<char, double>), "");
    ASSERT_DEATH(registry.group<int>(entt::get<char>, entt::exclude<double>), "");
    ASSERT_DEATH((registry.group<int, double>()), "");
}

ENTT_DEBUG_TEST(RegistryDeathTest, ConflictingGroups) {
    entt::registry registry{};

    registry.group<char>(entt::get<int>, entt::exclude<double>);
    ASSERT_DEATH(registry.group<char>(entt::get<float>, entt::exclude<double>), "");
}

TEST(Registry, GroupIfExists) {
    entt::registry registry{};
    const auto entity = registry.create();
    auto group = std::as_const(registry).group_if_exists<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_FALSE(group);

    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<char>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<double>(), nullptr);

    registry.emplace<int>(entity);
    group = std::as_const(registry).group_if_exists<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_FALSE(group);

    registry.emplace<char>(entity);
    group = std::as_const(registry).group_if_exists<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_FALSE(group);

    registry.emplace<double>(entity);
    group = std::as_const(registry).group_if_exists<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_FALSE(group);

    registry.group<int>(entt::get<char>, entt::exclude<double>);
    group = std::as_const(registry).group_if_exists<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_TRUE(group);
}

TEST(Registry, SortSingle) {
    entt::registry registry{};

    int val = 0;

    registry.emplace<int>(registry.create(), val++);
    registry.emplace<int>(registry.create(), val++);
    registry.emplace<int>(registry.create(), val++);

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --val);
    }

    registry.sort<int>(std::less{});

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), val++);
    }
}

TEST(Registry, SortMulti) {
    entt::registry registry{};

    unsigned int uval = 0u;
    int ival = 0;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<unsigned int>(entity, uval++);
        registry.emplace<int>(entity, ival++);
    }

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), --uval);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less{});
    registry.sort<int, unsigned int>();

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), uval++);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), ival++);
    }
}

TEST(Registry, SortEmpty) {
    entt::registry registry{};

    registry.emplace<test::empty>(registry.create());
    registry.emplace<test::empty>(registry.create());
    registry.emplace<test::empty>(registry.create());

    ASSERT_LT(registry.storage<test::empty>().data()[0], registry.storage<test::empty>().data()[1]);
    ASSERT_LT(registry.storage<test::empty>().data()[1], registry.storage<test::empty>().data()[2]);

    registry.sort<test::empty>(std::less{});

    ASSERT_GT(registry.storage<test::empty>().data()[0], registry.storage<test::empty>().data()[1]);
    ASSERT_GT(registry.storage<test::empty>().data()[1], registry.storage<test::empty>().data()[2]);
}

TEST(Registry, Context) {
    entt::registry registry{};
    auto &ctx = registry.ctx();
    const auto &cctx = std::as_const(registry).ctx();

    ASSERT_FALSE(ctx.contains<char>());
    ASSERT_FALSE(cctx.contains<const int>());
    ASSERT_EQ(ctx.find<char>(), nullptr);
    ASSERT_EQ(cctx.find<const int>(), nullptr);

    ctx.emplace<char>();
    ctx.emplace<int>();

    ASSERT_TRUE(ctx.contains<char>());
    ASSERT_TRUE(cctx.contains<int>());
    ASSERT_NE(ctx.find<const char>(), nullptr);
    ASSERT_NE(cctx.find<const int>(), nullptr);

    ASSERT_FALSE(ctx.erase<double>());
    ASSERT_TRUE(ctx.erase<int>());

    ASSERT_TRUE(ctx.contains<const char>());
    ASSERT_FALSE(cctx.contains<const int>());
    ASSERT_NE(ctx.find<char>(), nullptr);
    ASSERT_EQ(cctx.find<int>(), nullptr);

    ASSERT_FALSE(ctx.erase<int>());
    ASSERT_TRUE(ctx.erase<char>());

    ctx.emplace<char>('c');
    ctx.emplace<int>(1);

    ASSERT_EQ(ctx.emplace<char>('a'), 'c');
    ASSERT_EQ(ctx.find<const char>(), cctx.find<char>());
    ASSERT_EQ(ctx.get<char>(), cctx.get<const char>());
    ASSERT_EQ(ctx.get<char>(), 'c');

    ASSERT_EQ(ctx.emplace<const int>(0), 1);
    ASSERT_EQ(ctx.find<const int>(), cctx.find<int>());
    ASSERT_EQ(ctx.get<int>(), cctx.get<const int>());
    ASSERT_EQ(ctx.get<int>(), 1);

    ASSERT_EQ(ctx.find<double>(), nullptr);
    ASSERT_EQ(cctx.find<double>(), nullptr);

    ASSERT_EQ(ctx.insert_or_assign<char>('a'), 'a');
    ASSERT_EQ(ctx.find<const char>(), cctx.find<char>());
    ASSERT_EQ(ctx.get<char>(), cctx.get<const char>());
    ASSERT_EQ(ctx.get<const char>(), 'a');

    ASSERT_EQ(ctx.insert_or_assign<const int>(0), 0);
    ASSERT_EQ(ctx.find<const int>(), cctx.find<int>());
    ASSERT_EQ(ctx.get<int>(), cctx.get<const int>());
    ASSERT_EQ(ctx.get<int>(), 0);
}

TEST(Registry, ContextHint) {
    using namespace entt::literals;

    entt::registry registry{};
    auto &ctx = registry.ctx();
    const auto &cctx = std::as_const(registry).ctx();

    ctx.emplace<int>(1);
    ctx.emplace_as<int>("other"_hs, 3);

    ASSERT_TRUE(ctx.contains<int>());
    ASSERT_TRUE(cctx.contains<const int>("other"_hs));
    ASSERT_FALSE(ctx.contains<char>("other"_hs));

    ASSERT_NE(cctx.find<const int>(), nullptr);
    ASSERT_NE(ctx.find<int>("other"_hs), nullptr);
    ASSERT_EQ(cctx.find<const char>("other"_hs), nullptr);

    ASSERT_EQ(ctx.get<int>(), 1);
    ASSERT_EQ(cctx.get<const int>("other"_hs), 3);

    ctx.insert_or_assign(3);
    ctx.insert_or_assign("other"_hs, 1);

    ASSERT_EQ(ctx.get<const int>(), 3);
    ASSERT_EQ(cctx.get<int>("other"_hs), 1);

    ASSERT_FALSE(ctx.erase<char>("other"_hs));
    ASSERT_TRUE(ctx.erase<int>());

    ASSERT_TRUE(cctx.contains<int>("other"_hs));
    ASSERT_EQ(ctx.get<int>("other"_hs), 1);

    ASSERT_TRUE(ctx.erase<int>("other"_hs));

    ASSERT_FALSE(cctx.contains<int>("other"_hs));
    ASSERT_EQ(ctx.find<int>("other"_hs), nullptr);
}

TEST(Registry, ContextAsRef) {
    entt::registry registry{};
    int value{3};

    registry.ctx().emplace<int &>(value);

    ASSERT_NE(registry.ctx().find<int>(), nullptr);
    ASSERT_NE(registry.ctx().find<const int>(), nullptr);
    ASSERT_NE(std::as_const(registry).ctx().find<const int>(), nullptr);
    ASSERT_EQ(registry.ctx().get<const int>(), value);
    ASSERT_EQ(registry.ctx().get<int>(), value);

    registry.ctx().get<int>() = 2;

    ASSERT_EQ(value, 2);
    ASSERT_EQ(registry.ctx().get<int>(), value);

    value = 1;

    ASSERT_EQ(std::as_const(registry).ctx().get<const int>(), value);
}

TEST(Registry, ContextAsConstRef) {
    entt::registry registry{};
    int value{3};

    registry.ctx().emplace<const int &>(value);

    ASSERT_EQ(registry.ctx().find<int>(), nullptr);
    ASSERT_NE(registry.ctx().find<const int>(), nullptr);
    ASSERT_NE(std::as_const(registry).ctx().find<const int>(), nullptr);
    ASSERT_EQ(registry.ctx().get<const int>(), value);

    value = 1;

    ASSERT_EQ(std::as_const(registry).ctx().get<const int>(), value);
}

TEST(Registry, ContextPoolMemberDestructionOrder) {
    auto registry = std::make_unique<entt::registry>();
    const auto entity = registry->create();
    bool ctx_check = false;

    registry->ctx().emplace<typename destruction_order::ctx_check_type>();
    registry->emplace<destruction_order>(entity, *registry, ctx_check);
    registry.reset();

    ASSERT_TRUE(ctx_check);
}

TEST(Registry, Constness) {
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().emplace<int>({})), int &>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().emplace<test::empty>({})), void>();

    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().get<>({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().get<int>({})), int &>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().get<int, const char>({})), std::tuple<int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().try_get<>({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().try_get<int>({})), int *>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().try_get<int, const char>({})), std::tuple<int *, const char *>>();

    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().ctx().get<int>()), int &>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().ctx().get<const char>()), const char &>();

    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().ctx().find<int>()), int *>();
    testing::StaticAssertTypeEq<decltype(std::declval<entt::registry>().ctx().find<const char>()), const char *>();

    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().get<>({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().get<int>({})), const int &>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().get<int, const char>({})), std::tuple<const int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().try_get<>({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().try_get<int>({})), const int *>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().try_get<int, const char>({})), std::tuple<const int *, const char *>>();

    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().ctx().get<int>()), const int &>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().ctx().get<const char>()), const char &>();

    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().ctx().find<int>()), const int *>();
    testing::StaticAssertTypeEq<decltype(std::declval<const entt::registry>().ctx().find<const char>()), const char *>();
}

TEST(Registry, AssignEntities) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry{};
    std::array<entt::entity, 3u> entity{};
    registry.create(entity.begin(), entity.end());
    registry.destroy(entity[1]);
    registry.destroy(entity[2]);

    entt::registry other{};
    auto &src = registry.storage<entt::entity>();
    auto &dst = other.storage<entt::entity>();

    dst.push(src.rbegin(), src.rend());
    dst.free_list(src.free_list());

    ASSERT_EQ(registry.storage<entt::entity>().size(), other.storage<entt::entity>().size());
    ASSERT_TRUE(other.valid(entity[0]));
    ASSERT_FALSE(other.valid(entity[1]));
    ASSERT_FALSE(other.valid(entity[2]));
    ASSERT_EQ(registry.create(), other.create());
    ASSERT_EQ(traits_type::to_entity(other.create()), traits_type::to_integral(entity[1]));
}

TEST(Registry, ScramblingPoolsIsAllowed) {
    entt::registry registry{};
    registry.on_destroy<int>().connect<&listener::sort<int>>();

    for(std::size_t i{}; i < 2u; ++i) {
        const auto entity = registry.create();
        registry.emplace<int>(entity, static_cast<int>(i));
    }

    registry.destroy(registry.view<int>().back());

    // thanks to @andranik3949 for pointing out this missing test
    registry.view<const int>().each([](const auto entity, const auto &value) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entity)), value);
    });
}

TEST(Registry, AssureMixinLoop) {
    entt::registry registry{};
    const auto entity = registry.create();

    ASSERT_EQ(std::as_const(registry).storage<test::assure_loop>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);

    registry.emplace<test::assure_loop>(entity);

    ASSERT_NE(std::as_const(registry).storage<test::assure_loop>(), nullptr);
    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);

    ASSERT_TRUE(registry.all_of<test::assure_loop>(entity));
    ASSERT_FALSE(registry.all_of<int>(entity));
}

TEST(Registry, VoidType) {
    using namespace entt::literals;

    entt::registry registry{};
    const auto entity = registry.create();
    auto &storage = registry.storage<void>("custom"_hs);
    storage.emplace(entity);

    ASSERT_TRUE(registry.storage<void>().empty());
    ASSERT_FALSE(registry.storage<void>("custom"_hs).empty());
    ASSERT_TRUE(registry.storage<void>("custom"_hs).contains(entity));

    ASSERT_EQ(registry.storage<void>().type(), entt::type_id<void>());
    ASSERT_EQ(registry.storage<void>("custom"_hs).type(), entt::type_id<void>());
}

TEST(Registry, NoEtoType) {
    entt::registry registry{};
    const auto entity = registry.create();

    registry.emplace<no_eto_type>(entity);
    registry.emplace<int>(entity, 1);

    ASSERT_NE(registry.storage<no_eto_type>().raw(), nullptr);
    ASSERT_NE(registry.try_get<no_eto_type>(entity), nullptr);
    ASSERT_EQ(registry.view<no_eto_type>().get(entity), std::as_const(registry).view<const no_eto_type>().get(entity));

    auto view = registry.view<no_eto_type, int>();
    auto cview = std::as_const(registry).view<const no_eto_type, const int>();

    ASSERT_EQ((std::get<0>(view.get<no_eto_type, int>(entity))), (std::get<0>(cview.get<const no_eto_type, const int>(entity))));
}
