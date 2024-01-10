#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

template<typename Type>
struct BasicHandle: testing::Test {
    using type = Type;
};

using BasicHandleTypes = ::testing::Types<entt::handle, entt::const_handle>;

TYPED_TEST_SUITE(BasicHandle, BasicHandleTypes, );

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

    const handle_type handle{registry, entity};

    ASSERT_FALSE(entt::null == handle.entity());
    ASSERT_EQ(entity, handle);
    ASSERT_TRUE(handle);

    ASSERT_EQ(handle, (entt::handle{registry, entity}));
    ASSERT_EQ(handle, (entt::const_handle{registry, entity}));

    testing::StaticAssertTypeEq<typename handle_type::registry_type *, decltype(handle.registry())>();
}

TYPED_TEST(BasicHandle, Invalidation) {
    using handle_type = typename TestFixture::type;

    handle_type handle;

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});

    entt::registry registry;
    const auto entity = registry.create();

    handle = {registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_NE(handle.entity(), entt::entity{entt::null});

    handle = {};

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});
}

TEST(BasicHandle, Destruction) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const auto entity = registry.create();
    entt::handle handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy(traits_type::to_version(entity));

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(registry.current(entity), typename entt::registry::version_type{});
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});

    handle = entt::handle{registry, registry.create()};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy();

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_NE(registry.current(entity), typename entt::registry::version_type{});
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});
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

TEST(BasicHandle, Component) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::handle_view<int, char, double> handle{registry, entity};

    ASSERT_EQ(3, handle.emplace<int>(3));
    ASSERT_EQ('c', handle.emplace_or_replace<char>('c'));
    ASSERT_EQ(.3, handle.emplace_or_replace<double>(.3));

    const auto &patched = handle.patch<int>([](auto &comp) { comp = 42; }); // NOLINT

    ASSERT_EQ(42, patched);
    ASSERT_EQ('a', handle.replace<char>('a'));
    ASSERT_TRUE((handle.all_of<int, char, double>()));
    ASSERT_EQ((std::make_tuple(42, 'a', .3)), (handle.get<int, char, double>()));

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

    ASSERT_EQ(42, handle.get_or_emplace<int>(42));
    ASSERT_EQ(42, handle.get_or_emplace<int>(1));
    ASSERT_EQ(42, handle.get<int>());

    ASSERT_EQ(42, *handle.try_get<int>());
    ASSERT_EQ(nullptr, handle.try_get<char>());
    ASSERT_EQ(nullptr, std::get<1>(handle.try_get<int, char, double>()));
}

TYPED_TEST(BasicHandle, FromEntity) {
    using handle_type = typename TestFixture::type;

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42); // NOLINT
    registry.emplace<char>(entity, 'c');

    const handle_type handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_EQ(entity, handle.entity());
    ASSERT_TRUE((handle.template all_of<int, char>()));
    ASSERT_EQ(handle.template get<int>(), 42);
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

TEST(BasicHandle, ImplicitConversions) {
    entt::registry registry;
    const entt::handle handle{registry, registry.create()};
    const entt::const_handle const_handle = handle;
    const entt::handle_view<int, char> handle_view = handle;
    const entt::const_handle_view<int> const_handle_view = handle_view;

    handle.emplace<int>(42); // NOLINT

    ASSERT_EQ(handle.get<int>(), const_handle.get<int>());
    ASSERT_EQ(const_handle.get<int>(), handle_view.get<int>());
    ASSERT_EQ(handle_view.get<int>(), const_handle_view.get<int>());
    ASSERT_EQ(const_handle_view.get<int>(), 42);
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
    ASSERT_EQ(handle.storage().begin()->second.type(), entt::type_id<int>());
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
