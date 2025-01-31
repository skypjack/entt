#include <algorithm>
#include <array>
#include <memory>
#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/runtime_view.hpp>
#include <entt/entity/storage.hpp>
#include "../../common/linter.hpp"
#include "../../common/pointer_stable.h"

template<typename Type>
struct RuntimeView: testing::Test {
    using type = Type;
};

using RuntimeViewTypes = ::testing::Types<entt::runtime_view, entt::const_runtime_view>;

TYPED_TEST_SUITE(RuntimeView, RuntimeViewTypes, );

TYPED_TEST(RuntimeView, Functionalities) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    ASSERT_FALSE(view);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_FALSE(view.contains(entity[1u]));

    view.iterate(std::get<0>(storage)).iterate(std::get<1>(storage));

    ASSERT_TRUE(view);
    ASSERT_EQ(view.size_hint(), 0u);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);

    ASSERT_NE(view.size_hint(), 0u);

    std::get<1>(storage).emplace(entity[1u]);

    ASSERT_EQ(view.size_hint(), 1u);

    auto it = view.begin();

    ASSERT_EQ(*it, entity[1u]);
    ASSERT_EQ(++it, (view.end()));

    ASSERT_NO_THROW((view.begin()++));
    ASSERT_NO_THROW((++view.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size_hint(), 1u);

    std::get<1>(storage).get(entity[0u]) = '1';
    std::get<1>(storage).get(entity[1u]) = '2';
    std::get<0>(storage).get(entity[1u]) = 3;

    for(auto entt: view) {
        ASSERT_EQ(std::get<0>(storage).get(entt), 3);
        ASSERT_EQ(std::get<1>(storage).get(entt), '2');
    }

    view.clear();

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
}

TYPED_TEST(RuntimeView, Constructors) {
    using runtime_view_type = typename TestFixture::type;

    entt::storage<int> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};

    ASSERT_FALSE(view);

    storage.emplace(entity);

    view = runtime_view_type{std::allocator<int>{}};
    view.iterate(storage);

    ASSERT_TRUE(view);
    ASSERT_TRUE(view.contains(entity));

    runtime_view_type temp{view, view.get_allocator()};
    const runtime_view_type other{std::move(temp), view.get_allocator()};

    test::is_initialized(temp);

    ASSERT_FALSE(temp);
    ASSERT_TRUE(other);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Copy) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};

    ASSERT_FALSE(view);

    std::get<0>(storage).emplace(entity);
    std::get<1>(storage).emplace(entity);

    view.iterate(std::get<0>(storage));

    runtime_view_type other{view};

    ASSERT_TRUE(view);
    ASSERT_TRUE(other);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));

    other.iterate(std::get<0>(storage)).exclude(std::get<1>(storage));

    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));

    other = view;

    ASSERT_TRUE(view);
    ASSERT_TRUE(other);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Move) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};

    ASSERT_FALSE(view);

    std::get<0>(storage).emplace(entity);
    std::get<1>(storage).emplace(entity);

    view.iterate(std::get<0>(storage));

    runtime_view_type other{std::move(view)};

    test::is_initialized(view);

    ASSERT_FALSE(view);
    ASSERT_TRUE(other);

    ASSERT_TRUE(other.contains(entity));

    view = other;
    other.iterate(std::get<0>(storage)).exclude(std::get<1>(storage));

    ASSERT_TRUE(view);
    ASSERT_TRUE(other);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));

    other = std::move(view);
    test::is_initialized(view);

    ASSERT_FALSE(view);
    ASSERT_TRUE(other);

    ASSERT_TRUE(other.contains(entity));
}

TYPED_TEST(RuntimeView, Swap) {
    using runtime_view_type = typename TestFixture::type;

    entt::storage<int> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};
    runtime_view_type other{};

    ASSERT_FALSE(view);
    ASSERT_FALSE(other);

    storage.emplace(entity);
    view.iterate(storage);

    ASSERT_TRUE(view);
    ASSERT_FALSE(other);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_EQ(other.size_hint(), 0u);
    ASSERT_TRUE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(other.begin(), other.end());

    view.swap(other);

    ASSERT_FALSE(view);
    ASSERT_TRUE(other);

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

    entt::storage<int> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};

    storage.emplace(entity);
    view.iterate(storage);

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

    entt::storage<int> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    storage.emplace(entity[0u]);
    storage.emplace(entity[1u]);

    storage.erase(entity[0u]);

    view.iterate(storage);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));
}

TYPED_TEST(RuntimeView, Empty) {
    using runtime_view_type = typename TestFixture::type;

    entt::storage<int> storage{};
    const entt::entity entity{0};
    runtime_view_type view{};

    view.iterate(storage);

    ASSERT_FALSE(view.contains(entity));
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ((std::find(view.begin(), view.end(), entity)), view.end());

    storage.emplace(entity);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE((std::find(view.begin(), view.end(), entity)), view.end());
}

TYPED_TEST(RuntimeView, Each) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    view.iterate(std::get<0>(storage)).iterate(std::get<1>(storage));

    view.each([&](const auto entt) {
        ASSERT_EQ(entt, entity[0u]);
    });
}

TYPED_TEST(RuntimeView, EachWithHoles) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    std::get<1>(storage).emplace(entity[0u], '0');
    std::get<1>(storage).emplace(entity[1u], '1');

    std::get<0>(storage).emplace(entity[0u], 0);
    std::get<0>(storage).emplace(entity[2u], 2);

    view.iterate(std::get<0>(storage)).iterate(std::get<1>(storage));

    view.each([&](auto entt) {
        ASSERT_EQ(entt, entity[0u]);
    });
}

TYPED_TEST(RuntimeView, ExcludedComponents) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    std::get<0>(storage).emplace(entity[0u]);

    std::get<0>(storage).emplace(entity[1u]);
    std::get<1>(storage).emplace(entity[1u]);

    view.iterate(std::get<0>(storage)).exclude(std::get<1>(storage));

    ASSERT_TRUE(view.contains(entity[0u]));
    ASSERT_FALSE(view.contains(entity[1u]));

    view.each([&](auto entt) {
        ASSERT_EQ(entt, entity[0u]);
    });
}

TYPED_TEST(RuntimeView, StableType) {
    using runtime_view_type = typename TestFixture::type;

    std::tuple<entt::storage<int>, entt::storage<test::pointer_stable>> storage{};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);
    std::get<0>(storage).emplace(entity[2u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<1>(storage).remove(entity[1u]);

    view.iterate(std::get<0>(storage)).iterate(std::get<1>(storage));

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_TRUE(view.contains(entity[0u]));
    ASSERT_FALSE(view.contains(entity[1u]));

    ASSERT_EQ(*view.begin(), entity[0u]);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([&](const auto entt) {
        ASSERT_EQ(entt, entity[0u]);
    });

    for(auto entt: view) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        ASSERT_EQ(entt, entity[0u]);
    }

    std::get<1>(storage).compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TYPED_TEST(RuntimeView, StableTypeWithExcludedComponent) {
    using runtime_view_type = typename TestFixture::type;

    constexpr entt::entity tombstone = entt::tombstone;
    std::tuple<entt::storage<int>, entt::storage<test::pointer_stable>> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    runtime_view_type view{};

    std::get<1>(storage).emplace(entity[0u], 0);
    std::get<1>(storage).emplace(entity[1u], 1);
    std::get<0>(storage).emplace(entity[0u]);

    view.iterate(std::get<1>(storage)).exclude(std::get<0>(storage));

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    std::get<0>(storage).erase(entity[0u]);
    std::get<1>(storage).erase(entity[0u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    for(auto entt: view) {
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, entity[1u]);
    }

    view.each([&](const auto entt) {
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, entity[1u]);
    });
}
