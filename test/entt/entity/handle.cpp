#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include "../../common/config.h"

template<typename Type>
struct BasicHandle: testing::Test {
    using type = Type;
};

template<typename Type>
using BasicHandleDeathTest = BasicHandle<Type>;

using BasicHandleTypes = ::testing::Types<entt::handle, entt::const_handle>;

TYPED_TEST_SUITE(BasicHandle, BasicHandleTypes, );
TYPED_TEST_SUITE(BasicHandleDeathTest, BasicHandleTypes, );

TYPED_TEST(BasicHandle, Assumptions) {
    using handle_type = typename TestFixture::type;
    static_assert(std::is_trivially_copyable_v<handle_type>, "Trivially copyable type required");
    static_assert((std::is_trivially_assignable_v<handle_type, handle_type>), "Trivially assignable type required");
    static_assert(std::is_trivially_destructible_v<handle_type>, "Trivially destructible type required");
}

TYPED_TEST(BasicHandle, DeductionGuide) {
    using handle_type = typename TestFixture::type;
    testing::StaticAssertTypeEq<decltype(entt::basic_handle{std::declval<typename handle_type::registry_type &>(), {}}), handle_type>();
}

TYPED_TEST(BasicHandle, Construction) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();

    handle_type handle{};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());

    ASSERT_TRUE(handle == entt::null);
    ASSERT_EQ(handle.registry(), nullptr);

    ASSERT_NE(handle, (entt::handle{registry, entity}));
    ASSERT_NE(handle, (entt::const_handle{registry, entity}));

    handle = handle_type{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle.valid());

    ASSERT_FALSE(handle == entt::null);
    ASSERT_EQ(handle.registry(), &registry);

    ASSERT_EQ(handle, (entt::handle{registry, entity}));
    ASSERT_EQ(handle, (entt::const_handle{registry, entity}));

    handle = {};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());

    ASSERT_TRUE(handle == entt::null);
    ASSERT_EQ(handle.registry(), nullptr);

    ASSERT_NE(handle, (entt::handle{registry, entity}));
    ASSERT_NE(handle, (entt::const_handle{registry, entity}));
}

TYPED_TEST(BasicHandle, Storage) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();
    const handle_type handle{registry, entity};

    testing::StaticAssertTypeEq<decltype(*handle.storage().begin()), std::pair<entt::id_type, entt::constness_as_t<entt::sparse_set, typename handle_type::registry_type> &>>();

    ASSERT_EQ(handle.storage().begin(), handle.storage().end());

    registry.storage<double>();
    registry.emplace<int>(entity);

    ASSERT_NE(handle.storage().begin(), handle.storage().end());
    ASSERT_EQ(++handle.storage().begin(), handle.storage().end());
    ASSERT_EQ(handle.storage().begin()->second.info(), entt::type_id<int>());
}

ENTT_DEBUG_TYPED_TEST(BasicHandleDeathTest, Storage) {
    using handle_type = typename TestFixture::type;
    const handle_type handle{};

    ASSERT_DEATH([[maybe_unused]] auto iterable = handle.storage(), "");
}

TYPED_TEST(BasicHandle, HandleStorageIterator) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<double>(entity);
    // required to test the find-first initialization step
    registry.storage<entt::entity>().erase(entity);

    const handle_type handle{registry, entity};
    auto iterable = handle.storage();

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_FALSE(handle);

    auto end{iterable.begin()};
    decltype(end) begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(++begin, iterable.end());
}

TYPED_TEST(BasicHandle, Entity) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();

    handle_type handle{};

    ASSERT_TRUE(handle == entt::null);
    ASSERT_NE(handle.entity(), entity);
    ASSERT_NE(handle, entity);

    handle = handle_type{registry, entity};

    ASSERT_FALSE(handle == entt::null);
    ASSERT_EQ(handle.entity(), entity);
    ASSERT_EQ(handle, entity);
}

TEST(BasicHandle, Destruction) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const auto entity = registry.create();
    entt::handle handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.registry(), &registry);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy(traits_type::to_version(entity));

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.registry(), &registry);
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});
    ASSERT_EQ(registry.current(entity), traits_type::to_version(entity));

    handle = entt::handle{registry, registry.create()};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.registry(), &registry);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy();

    ASSERT_FALSE(handle);
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_NE(registry.current(entity), traits_type::to_version(entity));
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Destruction) {
    entt::handle handle{};

    ASSERT_DEATH(handle.destroy(0u), "");
    ASSERT_DEATH(handle.destroy(), "");
}

TEST(BasicHandle, Emplace) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_FALSE(registry.all_of<int>(entity));

    ASSERT_EQ(handle.emplace<int>(3), 3);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), 3);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Emplace) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.emplace<int>(3), "");
}

TEST(BasicHandle, EmplaceOrReplace) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_FALSE(registry.all_of<int>(entity));

    ASSERT_EQ(handle.emplace_or_replace<int>(3), 3);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), 3);

    ASSERT_EQ(handle.emplace_or_replace<int>(1), 1);

    ASSERT_EQ(registry.get<int>(entity), 1);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, EmplaceOrReplace) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.emplace_or_replace<int>(3), "");
}

TEST(BasicHandle, Patch) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    registry.emplace<int>(entity, 3);

    ASSERT_TRUE(handle.all_of<int>());
    ASSERT_EQ(handle.patch<int>([](auto &comp) { comp = 1; }), 1);

    ASSERT_EQ(registry.get<int>(entity), 1);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Patch) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.patch<int>([](auto &comp) { comp = 1; }), "");
}

TEST(BasicHandle, Replace) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    registry.emplace<int>(entity, 3);

    ASSERT_TRUE(handle.all_of<int>());
    ASSERT_EQ(handle.replace<int>(1), 1);

    ASSERT_EQ(registry.get<int>(entity), 1);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Replace) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.replace<int>(3), "");
}

TEST(BasicHandle, Remove) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_FALSE(handle.all_of<int>());
    ASSERT_EQ(handle.remove<int>(), 0u);

    registry.emplace<int>(entity, 3);

    ASSERT_TRUE(handle.all_of<int>());
    ASSERT_EQ(handle.remove<int>(), 1u);

    ASSERT_FALSE(handle.all_of<int>());
    ASSERT_EQ(handle.remove<int>(), 0u);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Remove) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.remove<int>(), "");
}

TEST(BasicHandle, Erase) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    registry.emplace<int>(entity, 3);

    ASSERT_TRUE(handle.all_of<int>());

    handle.erase<int>();

    ASSERT_FALSE(handle.all_of<int>());
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, Erase) {
    const entt::handle handle{};

    ASSERT_DEATH(handle.erase<int>(), "");
}

TYPED_TEST(BasicHandle, AllAnyOf) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_FALSE((handle.all_of<int, char>()));
    ASSERT_FALSE((handle.any_of<int, char>()));

    registry.emplace<char>(entity);

    ASSERT_FALSE((handle.all_of<int, char>()));
    ASSERT_TRUE((handle.any_of<int, char>()));

    registry.emplace<int>(entity);

    ASSERT_TRUE((handle.all_of<int, char>()));
    ASSERT_TRUE((handle.any_of<int, char>()));
}

ENTT_DEBUG_TYPED_TEST(BasicHandleDeathTest, AllAnyOf) {
    using handle_type = typename TestFixture::type;
    const handle_type handle{};

    ASSERT_DEATH([[maybe_unused]] const auto all_of = handle.template all_of<int>(), "");
    ASSERT_DEATH([[maybe_unused]] const auto any_of = handle.template any_of<int>(), "");
}

TYPED_TEST(BasicHandle, Get) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    registry.emplace<int>(entity, 3);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(handle.get<int>(), 3);
    ASSERT_EQ((handle.get<int, const char>()), (std::make_tuple(3, 'c')));

    std::get<0>(handle.get<int, char>()) = 1;
    std::get<1>(handle.get<int, char>()) = '\0';

    ASSERT_EQ(registry.get<int>(entity), 1);
    ASSERT_EQ(registry.get<char>(entity), '\0');
}

ENTT_DEBUG_TYPED_TEST(BasicHandleDeathTest, Get) {
    using handle_type = typename TestFixture::type;
    const handle_type handle{};

    ASSERT_DEATH([[maybe_unused]] const auto &elem = handle.template get<int>(), "");
}

TEST(BasicHandle, GetOrEmplace) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_FALSE(registry.all_of<int>(entity));

    ASSERT_EQ(handle.get_or_emplace<int>(3), 3);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), 3);

    ASSERT_EQ(handle.get_or_emplace<int>(1), 3);
}

ENTT_DEBUG_TEST(BasicHandleDeathTest, GetOrEmplace) {
    const entt::handle handle{};

    ASSERT_DEATH([[maybe_unused]] auto &&elem = handle.template get_or_emplace<int>(3), "");
}

TYPED_TEST(BasicHandle, TryGet) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_EQ((handle.try_get<int, const char>()), (std::make_tuple(nullptr, nullptr)));

    registry.emplace<int>(entity, 3);

    ASSERT_NE(handle.try_get<int>(), nullptr);
    ASSERT_EQ(handle.try_get<char>(), nullptr);

    ASSERT_EQ((*std::get<0>(handle.try_get<int, const char>())), 3);
    ASSERT_EQ((std::get<1>(handle.try_get<int, const char>())), nullptr);

    *std::get<0>(handle.try_get<int, const char>()) = 1;

    ASSERT_EQ(registry.get<int>(entity), 1);
}

ENTT_DEBUG_TYPED_TEST(BasicHandleDeathTest, TryGet) {
    using handle_type = typename TestFixture::type;
    const handle_type handle{};

    ASSERT_DEATH([[maybe_unused]] const auto *elem = handle.template try_get<int>(), "");
}

TYPED_TEST(BasicHandle, Orphan) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle handle{registry, entity};

    ASSERT_TRUE(handle.orphan());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_FALSE(handle.orphan());

    registry.erase<char>(entity);

    ASSERT_FALSE(handle.orphan());

    registry.erase<int>(entity);

    ASSERT_TRUE(handle.orphan());
}

ENTT_DEBUG_TYPED_TEST(BasicHandleDeathTest, Orphan) {
    using handle_type = typename TestFixture::type;
    const handle_type handle{};

    ASSERT_DEATH([[maybe_unused]] const auto result = handle.orphan(), "");
}

/*
TEST(BasicHandle, Component) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle_view<int, char, double> handle{registry, entity};

    ASSERT_EQ(3, handle.emplace<int>(3));
    ASSERT_EQ('c', handle.emplace_or_replace<char>('c'));
    ASSERT_EQ(.3, handle.emplace_or_replace<double>(.3));

    const auto &patched = handle.patch<int>([](auto &comp) { comp = 2; });

    ASSERT_EQ(2, patched);
    ASSERT_EQ('a', handle.replace<char>('a'));
    ASSERT_TRUE((handle.all_of<int, char, double>()));
    ASSERT_EQ((std::make_tuple(2, 'a', .3)), (handle.get<int, char, double>()));

    handle.erase<char, double>();

    ASSERT_TRUE(registry.storage<char>().empty());
    ASSERT_TRUE(registry.storage<double>().empty());
    ASSERT_EQ(0u, (handle.remove<char, double>()));

    for(auto [id, pool]: handle.storage()) {
        ASSERT_EQ(id, entt::type_id<int>().hash());
        ASSERT_TRUE(pool.contains(handle.entity()));
    }

    ASSERT_TRUE((handle.any_of<int, char, double>()));
    ASSERT_FALSE((handle.all_of<int, char, double>()));
    ASSERT_FALSE(handle.orphan());

    ASSERT_EQ(1u, (handle.remove<int>()));
    ASSERT_TRUE(registry.storage<int>().empty());
    ASSERT_TRUE(handle.orphan());

    ASSERT_EQ(2, handle.get_or_emplace<int>(2));
    ASSERT_EQ(2, handle.get_or_emplace<int>(1));
    ASSERT_EQ(2, handle.get<int>());

    ASSERT_EQ(2, *handle.try_get<int>());
    ASSERT_EQ(nullptr, handle.try_get<char>());
    ASSERT_EQ(nullptr, std::get<1>(handle.try_get<int, char, double>()));
}
*/

TEST(BasicHandle, ImplicitConversion) {
    entt::registry registry;
    const entt::handle handle{registry, registry.create()};
    const entt::const_handle const_handle = handle;
    const entt::handle_view<int, char> handle_view = handle;
    const entt::const_handle_view<int> const_handle_view = handle_view;

    handle.emplace<int>(2);

    ASSERT_EQ(handle.get<int>(), const_handle.get<int>());
    ASSERT_EQ(const_handle.get<int>(), handle_view.get<int>());
    ASSERT_EQ(handle_view.get<int>(), const_handle_view.get<int>());
    ASSERT_EQ(const_handle_view.get<int>(), 2);
}

TYPED_TEST(BasicHandle, Comparison) {
    using handle_type = typename TestFixture::type;

    handle_type handle{};

    ASSERT_EQ(handle, entt::handle{});
    ASSERT_TRUE(handle == entt::handle{});
    ASSERT_FALSE(handle != entt::handle{});

    ASSERT_EQ(handle, entt::const_handle{});
    ASSERT_TRUE(handle == entt::const_handle{});
    ASSERT_FALSE(handle != entt::const_handle{});

    entt::registry registry;
    const auto entity = registry.create();
    handle = handle_type{registry, entity};

    ASSERT_NE(handle, entt::handle{});
    ASSERT_FALSE(handle == entt::handle{});
    ASSERT_TRUE(handle != entt::handle{});

    ASSERT_NE(handle, entt::const_handle{});
    ASSERT_FALSE(handle == entt::const_handle{});
    ASSERT_TRUE(handle != entt::const_handle{});

    handle = {};

    ASSERT_EQ(handle, entt::handle{});
    ASSERT_TRUE(handle == entt::handle{});
    ASSERT_FALSE(handle != entt::handle{});

    ASSERT_EQ(handle, entt::const_handle{});
    ASSERT_TRUE(handle == entt::const_handle{});
    ASSERT_FALSE(handle != entt::const_handle{});

    entt::registry diff;
    handle = {registry, entity};
    const handle_type other = {diff, diff.create()};

    ASSERT_NE(handle, other);
    ASSERT_FALSE(other == handle);
    ASSERT_TRUE(other != handle);
    ASSERT_EQ(handle.entity(), other.entity());
    ASSERT_NE(handle.registry(), other.registry());
}

TYPED_TEST(BasicHandle, Null) {
    using handle_type = typename TestFixture::type;

    handle_type handle{};

    ASSERT_TRUE(handle == entt::null);
    ASSERT_TRUE(entt::null == handle);

    ASSERT_FALSE(handle != entt::null);
    ASSERT_FALSE(entt::null != handle);

    entt::registry registry;
    const auto entity = registry.create();

    handle = handle_type{registry, entity};

    ASSERT_FALSE(handle == entt::null);
    ASSERT_FALSE(entt::null == handle);

    ASSERT_TRUE(handle != entt::null);
    ASSERT_TRUE(entt::null != handle);

    if constexpr(!std::is_const_v<typename handle_type::registry_type>) {
        handle.destroy();

        ASSERT_TRUE(handle == entt::null);
        ASSERT_TRUE(entt::null == handle);

        ASSERT_FALSE(handle != entt::null);
        ASSERT_FALSE(entt::null != handle);
    }
}

TYPED_TEST(BasicHandle, FromEntity) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 2);
    registry.emplace<char>(entity, 'c');

    const handle_type handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_EQ(entity, handle.entity());
    ASSERT_TRUE((handle.template all_of<int, char>()));
    ASSERT_EQ(handle.template get<int>(), 2);
    ASSERT_EQ(handle.template get<char>(), 'c');
}

TEST(BasicHandle, Lifetime) {
    entt::registry registry;
    const auto entity = registry.create();
    auto handle = std::make_unique<entt::handle>(registry, entity);
    handle->emplace<int>();

    ASSERT_FALSE(registry.storage<int>().empty());
    ASSERT_NE(registry.storage<entt::entity>().free_list(), 0u);

    for(auto [entt]: registry.storage<entt::entity>().each()) {
        ASSERT_EQ(handle->entity(), entt);
    }

    handle.reset();

    ASSERT_FALSE(registry.storage<int>().empty());
    ASSERT_NE(registry.storage<entt::entity>().free_list(), 0u);
}
