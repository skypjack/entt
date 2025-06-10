#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/storage.hpp>
#include "../../common/config.h"
#include "../../common/empty.h"
#include "../../common/linter.hpp"

template<typename Type>
struct StorageNoInstance: testing::Test {
    static_assert(entt::component_traits<Type>::page_size == 0u, "Non-empty type not allowed");

    using type = Type;

    static auto emplace_instance(entt::storage<type> &pool, const entt::entity entt) {
        if constexpr(std::is_void_v<type>) {
            return pool.emplace(entt);
        } else {
            return pool.emplace(entt, type{});
        }
    }

    template<typename It>
    static auto insert_instance(entt::storage<type> &pool, const It from, const It to) {
        if constexpr(std::is_void_v<type>) {
            return pool.insert(from, to);
        } else {
            const std::array<type, 2u> value{};
            return pool.insert(from, to, value.begin());
        }
    }

    static auto push_instance(entt::storage<type> &pool, const entt::entity entt) {
        if constexpr(std::is_void_v<type>) {
            return pool.push(entt, nullptr);
        } else {
            type instance{};
            return pool.push(entt, &instance);
        }
    }
};

template<typename Type>
using StorageNoInstanceDeathTest = StorageNoInstance<Type>;

using StorageNoInstanceTypes = ::testing::Types<test::empty, void>;

TYPED_TEST_SUITE(StorageNoInstance, StorageNoInstanceTypes, );
TYPED_TEST_SUITE(StorageNoInstanceDeathTest, StorageNoInstanceTypes, );

TYPED_TEST(StorageNoInstance, Constructors) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());

    pool = entt::storage<value_type>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
}

TYPED_TEST(StorageNoInstance, Move) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{3}, entt::entity{2}};

    pool.emplace(entity[0u]);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    entt::storage<value_type> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.index(entity[0u]), 0u);

    entt::storage<value_type> extended{std::move(other), std::allocator<value_type>{}};

    test::is_initialized(other);

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(extended.info(), entt::type_id<value_type>());
    ASSERT_EQ(extended.index(entity[0u]), 0u);

    pool = std::move(extended);
    test::is_initialized(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(pool.index(entity[0u]), 0u);

    other = entt::storage<value_type>{};
    other.emplace(entity[1u], 2);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.index(entity[0u]), 0u);
}

TYPED_TEST(StorageNoInstance, Swap) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    entt::storage<value_type> other;

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    pool.emplace(entt::entity{4});

    other.emplace(entt::entity{2});
    other.emplace(entt::entity{1});
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entt::entity{1}), 0u);
    ASSERT_EQ(other.index(entt::entity{4}), 0u);
}

TYPED_TEST(StorageNoInstance, Getters) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    pool.emplace(entity, 3);

    testing::StaticAssertTypeEq<decltype(pool.get({})), void>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), void>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<>>();

    ASSERT_NO_THROW(pool.get(entity));
    ASSERT_NO_THROW(std::as_const(pool).get(entity));

    ASSERT_EQ(pool.get_as_tuple(entity), std::make_tuple());
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entity), std::make_tuple());
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Getters) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    ASSERT_DEATH(pool.get(entity), "");
    ASSERT_DEATH(std::as_const(pool).get(entity), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entity), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entity), "");
}

TYPED_TEST(StorageNoInstance, Value) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    pool.emplace(entity);

    ASSERT_EQ(pool.value(entt::entity{4}), nullptr);
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Value) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    ASSERT_DEATH([[maybe_unused]] const void *value = pool.value(entt::entity{4}), "");
}

TYPED_TEST(StorageNoInstance, Emplace) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_THROW(pool.emplace(entity[0u]));
    ASSERT_NO_THROW(this->emplace_instance(pool, entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Emplace) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    pool.emplace(entity);

    ASSERT_DEATH(pool.emplace(entity), "");
    ASSERT_DEATH(this->emplace_instance(pool, entity), "");
}

TYPED_TEST(StorageNoInstance, TryEmplace) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_NE(this->push_instance(pool, entity[0u]), base.end());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(base.index(entity[0u]), 0u);

    base.erase(entity[0u]);

    ASSERT_NE(base.push(entity.begin(), entity.end()), base.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(base.index(entity[0u]), 0u);
    ASSERT_EQ(base.index(entity[1u]), 1u);

    base.erase(entity.begin(), entity.end());

    ASSERT_NE(base.push(entity.rbegin(), entity.rend()), base.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(base.index(entity[0u]), 1u);
    ASSERT_EQ(base.index(entity[1u]), 0u);
}

TYPED_TEST(StorageNoInstance, Patch) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    int counter = 0;
    auto callback = [&counter]() { ++counter; };

    pool.emplace(entity);

    ASSERT_EQ(counter, 0);

    pool.patch(entity);
    pool.patch(entity, callback);
    pool.patch(entity, callback, callback);

    ASSERT_EQ(counter, 3);
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Patch) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TYPED_TEST(StorageNoInstance, Insert) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    pool.insert(entity.begin(), entity.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.index(entity[1u]), 1u);

    pool.erase(entity.begin(), entity.end());
    this->insert_instance(pool, entity.rbegin(), entity.rend());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Insert) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    pool.insert(entity.begin(), entity.end());

    ASSERT_DEATH(pool.insert(entity.begin(), entity.end()), "");
    ASSERT_DEATH(this->insert_instance(pool, entity.begin(), entity.end()), "");
}

TYPED_TEST(StorageNoInstance, Iterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});

    auto iterable = pool.each();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.begin());
    ASSERT_EQ(end.base(), base.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{3});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity == entt::entity{1} || entity == entt::entity{3});
    }
}

TYPED_TEST(StorageNoInstance, ConstIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::const_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});

    auto iterable = std::as_const(pool).each();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.begin());
    ASSERT_EQ(end.base(), base.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{3});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity == entt::entity{1} || entity == entt::entity{3});
    }
}

TYPED_TEST(StorageNoInstance, IterableIteratorConversion) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3});

    const typename entt::storage<value_type>::iterable::iterator it = pool.each().begin();
    typename entt::storage<value_type>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TYPED_TEST(StorageNoInstance, IterableAlgorithmCompatibility) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{3};

    pool.emplace(entity);

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TYPED_TEST(StorageNoInstance, ReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});

    auto iterable = pool.reach();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.rbegin());
    ASSERT_EQ(end.base(), base.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity == entt::entity{1} || entity == entt::entity{3});
    }
}

TYPED_TEST(StorageNoInstance, ConstReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});

    auto iterable = std::as_const(pool).reach();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.rbegin());
    ASSERT_EQ(end.base(), base.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity == entt::entity{1} || entity == entt::entity{3});
    }
}

TYPED_TEST(StorageNoInstance, ReverseIterableIteratorConversion) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3});

    const typename entt::storage<value_type>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<value_type>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TYPED_TEST(StorageNoInstance, ReverseIterableAlgorithmCompatibility) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{3};

    pool.emplace(entity);

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TYPED_TEST(StorageNoInstance, SortOrdered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{16}, entt::entity{8}, entt::entity{4}, entt::entity{2}, entt::entity{1}};

    pool.insert(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), pool.begin(), pool.end()));
}

TYPED_TEST(StorageNoInstance, SortReverse) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};

    pool.insert(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(entity.begin(), entity.end(), pool.begin(), pool.end()));
}

TYPED_TEST(StorageNoInstance, SortUnordered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{4}, entt::entity{2}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    pool.insert(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[0u]);
    ASSERT_EQ(pool.data()[3u], entity[1u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TYPED_TEST(StorageNoInstance, SortN) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{2}, entt::entity{4}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    pool.insert(entity.begin(), entity.end());
    pool.sort_n(0u, std::less{});

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), pool.begin(), pool.end()));

    pool.sort_n(2u, std::less{});

    ASSERT_EQ(pool.data()[0u], entity[1u]);
    ASSERT_EQ(pool.data()[1u], entity[0u]);
    ASSERT_EQ(pool.data()[2u], entity[2u]);

    const auto length = 5u;
    pool.sort_n(length, std::less{});

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[1u]);
    ASSERT_EQ(pool.data()[3u], entity[0u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TYPED_TEST(StorageNoInstance, SortAsDisjoint) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    const entt::storage<value_type> rhs;
    const std::array entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};

    lhs.insert(entity.begin(), entity.end());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));
}

TYPED_TEST(StorageNoInstance, SortAsOverlap) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};
    const std::array rhs_entity{entt::entity{2}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end());
    rhs.insert(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_EQ(lhs.data()[0u], lhs_entity[0u]);
    ASSERT_EQ(lhs.data()[1u], lhs_entity[2u]);
    ASSERT_EQ(lhs.data()[2u], lhs_entity[1u]);
}

TYPED_TEST(StorageNoInstance, SortAsOrdered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{32}, entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end());
    rhs.insert(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));
}

TYPED_TEST(StorageNoInstance, SortAsReverse) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{16}, entt::entity{8}, entt::entity{4}, entt::entity{2}, entt::entity{1}, entt::entity{32}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end());
    rhs.insert(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_EQ(rhs.data()[0u], rhs_entity[5u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[0u]);
}

TYPED_TEST(StorageNoInstance, SortAsUnordered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{4}, entt::entity{2}, entt::entity{32}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end());
    rhs.insert(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_EQ(rhs.data()[0u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[0u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[5u]);
}
