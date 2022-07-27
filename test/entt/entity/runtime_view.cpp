#include <algorithm>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value;
};

template<typename Type>
struct RuntimeView: testing::Test {
    using type = Type;
};

using RuntimeViewTypes = ::testing::Types<entt::runtime_view, entt::const_runtime_view>;

TYPED_TEST_SUITE(RuntimeView, RuntimeViewTypes, );

TYPED_TEST(RuntimeView, Functionalities) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_FALSE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    // forces the creation of the pools
    static_cast<void>(registry.storage<int>());
    static_cast<void>(registry.storage<char>());

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    ASSERT_EQ(view.size_hint(), 0u);

    registry.emplace<char>(e0);
    registry.emplace<int>(e1);

    ASSERT_NE(view.size_hint(), 0u);

    registry.emplace<char>(e1);

    ASSERT_EQ(view.size_hint(), 1u);

    auto it = view.begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (view.end()));

    ASSERT_NO_FATAL_FAILURE((view.begin()++));
    ASSERT_NO_FATAL_FAILURE((++view.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size_hint(), 1u);

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(registry.get<int>(entity), 42);
        ASSERT_EQ(registry.get<char>(entity), '2');
    }

    view.clear();

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
}

TYPED_TEST(RuntimeView, Constructors) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    registry.emplace<int>(entity);

    view = runtime_view_type{std::allocator<int>{}};
    view.iterate(registry.storage<int>());

    ASSERT_TRUE(view.contains(entity));

    runtime_view_type temp{view, view.get_allocator()};
    runtime_view_type other{std::move(temp), view.get_allocator()};

    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(temp.contains(entity));
    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Copy) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    view.iterate(registry.storage<int>());

    ASSERT_TRUE(view.contains(entity));

    runtime_view_type other{view};

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));

    other.iterate(registry.storage<int>()).exclude(registry.storage<char>());

    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));

    other = view;

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Move) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    view.iterate(registry.storage<int>());

    ASSERT_TRUE(view.contains(entity));

    runtime_view_type other{std::move(view)};

    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));

    view = other;
    other.iterate(registry.storage<int>()).exclude(registry.storage<char>());

    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));

    other = std::move(view);

    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Swap) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};
    runtime_view_type other{};

    const auto entity = registry.create();

    registry.emplace<int>(entity);
    view.iterate(registry.storage<int>());

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_EQ(other.size_hint(), 0u);
    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(other.begin(), other.end());

    view.swap(other);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(other.size_hint(), 1u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_NE(other.begin(), other.end());
}

TYPED_TEST(RuntimeView, Iterator) {
    using runtime_view_type = typename TestFixture::type;
    using iterator = typename runtime_view_type::iterator;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();

    registry.emplace<int>(entity);
    view.iterate(registry.storage<int>());

    iterator end{view.begin()};
    iterator begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, view.begin());
    ASSERT_EQ(begin--, view.end());

    ASSERT_EQ(++begin, view.end());
    ASSERT_EQ(--begin, view.begin());

    ASSERT_EQ(*begin, entity);
    ASSERT_EQ(*begin.operator->(), entity);
}

TYPED_TEST(RuntimeView, Contains) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<int>(other);

    registry.destroy(entity);

    view.iterate(registry.storage<int>());

    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));
}

TYPED_TEST(RuntimeView, Empty) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<double>(entity);
    registry.emplace<float>(other);

    view.iterate(registry.storage<int>());

    ASSERT_FALSE(view.contains(entity));
    ASSERT_FALSE(view.contains(other));
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), entity)), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), other)), view.end());
}

TYPED_TEST(RuntimeView, Each) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<char>(other);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    view.each([entity](const auto entt) {
        ASSERT_EQ(entt, entity);
    });
}

TYPED_TEST(RuntimeView, EachWithHoles) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<char>(e0, '0');
    registry.emplace<char>(e1, '1');

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e2, 2);

    view.iterate(registry.storage<int>()).iterate(registry.storage<char>());

    view.each([e0](auto entt) {
        ASSERT_EQ(e0, entt);
    });
}

TYPED_TEST(RuntimeView, ExcludedComponents) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    view.iterate(registry.storage<int>())
        .exclude(registry.storage<char>());

    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    view.each([e0](auto entt) {
        ASSERT_EQ(e0, entt);
    });
}

TYPED_TEST(RuntimeView, StableType) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<int>(e0);
    registry.emplace<int>(e1);
    registry.emplace<int>(e2);

    registry.emplace<stable_type>(e0);
    registry.emplace<stable_type>(e1);

    registry.remove<stable_type>(e1);

    view.iterate(registry.storage<int>()).iterate(registry.storage<stable_type>());

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    ASSERT_EQ(*view.begin(), e0);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([e0](const auto entt) {
        ASSERT_EQ(e0, entt);
    });

    for(auto entt: view) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        ASSERT_EQ(e0, entt);
    }

    registry.compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TYPED_TEST(RuntimeView, StableTypeWithExcludedComponent) {
    using runtime_view_type = typename TestFixture::type;

    entt::registry registry;
    runtime_view_type view{};

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<stable_type>(entity, 0);
    registry.emplace<stable_type>(other, 42);
    registry.emplace<int>(entity);

    view.iterate(registry.storage<stable_type>()).exclude(registry.storage<int>());

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    registry.destroy(entity);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    for(auto entt: view) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    }

    view.each([other](const auto entt) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    });
}
