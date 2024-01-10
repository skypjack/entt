#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>
#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/storage.hpp>
#include "../common/config.h"
#include "../common/empty.h"

template<typename Type>
struct StorageNoInstance: testing::Test {
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
            const type values[2u]{}; // NOLINT
            return pool.insert(from, to, std::begin(values));
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
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());

    pool = entt::storage<value_type>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
}

TYPED_TEST(StorageNoInstance, Move) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3});

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    entt::storage<value_type> other{std::move(pool)};

    ASSERT_TRUE(pool.empty()); // NOLINT
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<value_type>()); // NOLINT
    ASSERT_EQ(other.type(), entt::type_id<value_type>());

    ASSERT_EQ(other.index(entt::entity{3}), 0u);

    entt::storage<value_type> extended{std::move(other), std::allocator<value_type>{}};

    ASSERT_TRUE(other.empty()); // NOLINT
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(other.type(), entt::type_id<value_type>()); // NOLINT
    ASSERT_EQ(extended.type(), entt::type_id<value_type>());

    ASSERT_EQ(extended.index(entt::entity{3}), 0u);

    pool = std::move(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());    // NOLINT
    ASSERT_TRUE(extended.empty()); // NOLINT

    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
    ASSERT_EQ(other.type(), entt::type_id<value_type>());    // NOLINT
    ASSERT_EQ(extended.type(), entt::type_id<value_type>()); // NOLINT

    ASSERT_EQ(pool.index(entt::entity{3}), 0u);

    other = entt::storage<value_type>{};
    other.emplace(entt::entity{42}, 42); // NOLINT
    other = std::move(pool);

    ASSERT_TRUE(pool.empty()); // NOLINT
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<value_type>()); // NOLINT
    ASSERT_EQ(other.type(), entt::type_id<value_type>());

    ASSERT_EQ(other.index(entt::entity{3}), 0u);
}

TYPED_TEST(StorageNoInstance, Swap) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;
    entt::storage<value_type> other;

    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
    ASSERT_EQ(other.type(), entt::type_id<value_type>());

    pool.emplace(entt::entity{42}); // NOLINT

    other.emplace(entt::entity{9}); // NOLINT
    other.emplace(entt::entity{3});
    other.erase(entt::entity{9}); // NOLINT

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
    ASSERT_EQ(other.type(), entt::type_id<value_type>());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entt::entity{3}), 0u);
    ASSERT_EQ(other.index(entt::entity{42}), 0u);
}

TYPED_TEST(StorageNoInstance, Getters) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    pool.emplace(entt::entity{41}, 3); // NOLINT

    testing::StaticAssertTypeEq<decltype(pool.get({})), void>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), void>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<>>();

    ASSERT_NO_THROW(pool.get(entt::entity{41}));
    ASSERT_NO_THROW(std::as_const(pool).get(entt::entity{41}));

    ASSERT_EQ(pool.get_as_tuple(entt::entity{41}), std::make_tuple());
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entt::entity{41}), std::make_tuple());
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Getters) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    ASSERT_DEATH(pool.get(entt::entity{41}), "");
    ASSERT_DEATH(std::as_const(pool).get(entt::entity{41}), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entt::entity{41}), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entt::entity{41}), "");
}

TYPED_TEST(StorageNoInstance, Value) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    pool.emplace(entt::entity{42}); // NOLINT

    ASSERT_EQ(pool.value(entt::entity{42}), nullptr);
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Value) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    ASSERT_DEATH([[maybe_unused]] const void *value = pool.value(entt::entity{42}), "");
}

TYPED_TEST(StorageNoInstance, Emplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}}; // NOLINT

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_THROW(pool.emplace(entity[0u]));
    ASSERT_NO_THROW(this->emplace_instance(pool, entity[1u]));
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Emplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity{42};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    pool.emplace(entity);

    ASSERT_DEATH(pool.emplace(entity), "");
    ASSERT_DEATH(this->emplace_instance(pool, entity), "");
}

TYPED_TEST(StorageNoInstance, TryEmplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}}; // NOLINT

    ASSERT_NE(this->push_instance(pool, entity[0u]), base.end());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(base.index(entity[0u]), 0u);

    base.erase(entity[0u]);

    ASSERT_NE(base.push(std::begin(entity), std::end(entity)), base.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(base.index(entity[0u]), 0u);
    ASSERT_EQ(base.index(entity[1u]), 1u);

    base.erase(std::begin(entity), std::end(entity));

    ASSERT_NE(base.push(std::rbegin(entity), std::rend(entity)), base.end());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(base.index(entity[0u]), 1u);
    ASSERT_EQ(base.index(entity[1u]), 0u);
}

TYPED_TEST(StorageNoInstance, Patch) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity{42};

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

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.index(entity[1u]), 1u);

    pool.erase(std::begin(entity), std::end(entity));
    this->insert_instance(pool, std::rbegin(entity), std::rend(entity));

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Insert) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_DEATH(pool.insert(std::begin(entity), std::end(entity)), "");
    ASSERT_DEATH(this->insert_instance(pool, std::begin(entity), std::end(entity)), "");
}

TYPED_TEST(StorageNoInstance, Iterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
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

    pool.emplace(entt::entity{3});

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TYPED_TEST(StorageNoInstance, ReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
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

    pool.emplace(entt::entity{3});

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TYPED_TEST(StorageNoInstance, SortOrdered) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    entt::entity entity[5u]{entt::entity{42}, entt::entity{12}, entt::entity{9}, entt::entity{7}, entt::entity{3}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), pool.begin(), pool.end()));
}

TYPED_TEST(StorageNoInstance, SortReverse) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    entt::entity entity[5u]{entt::entity{3}, entt::entity{7}, entt::entity{9}, entt::entity{12}, entt::entity{42}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(std::begin(entity), std::end(entity), pool.begin(), pool.end()));
}

TYPED_TEST(StorageNoInstance, SortUnordered) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    entt::entity entity[5u]{entt::entity{9}, entt::entity{7}, entt::entity{3}, entt::entity{12}, entt::entity{42}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));
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

    entt::entity entity[5u]{entt::entity{7}, entt::entity{9}, entt::entity{3}, entt::entity{12}, entt::entity{42}}; // NOLINT

    pool.insert(std::begin(entity), std::end(entity));
    pool.sort_n(0u, std::less{});

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), pool.begin(), pool.end()));

    pool.sort_n(2u, std::less{});

    ASSERT_EQ(pool.data()[0u], entity[1u]);
    ASSERT_EQ(pool.data()[1u], entity[0u]);
    ASSERT_EQ(pool.data()[2u], entity[2u]);

    pool.sort_n(5u, std::less{}); // NOLINT

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

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}}; // NOLINT

    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
}

TYPED_TEST(StorageNoInstance, SortAsOverlap) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}}; // NOLINT
    entt::entity rhs_entity[1u]{entt::entity{12}};                                    // NOLINT

    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity));
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_EQ(lhs.data()[0u], lhs_entity[0u]);
    ASSERT_EQ(lhs.data()[1u], lhs_entity[2u]);
    ASSERT_EQ(lhs.data()[2u], lhs_entity[1u]);
}

TYPED_TEST(StorageNoInstance, SortAsOrdered) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};                  // NOLINT
    entt::entity rhs_entity[6u]{entt::entity{6}, entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}}; // NOLINT

    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity));
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));
}

TYPED_TEST(StorageNoInstance, SortAsReverse) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};                  // NOLINT
    entt::entity rhs_entity[6u]{entt::entity{5}, entt::entity{4}, entt::entity{3}, entt::entity{2}, entt::entity{1}, entt::entity{6}}; // NOLINT

    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity));
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

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

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};                  // NOLINT
    entt::entity rhs_entity[6u]{entt::entity{3}, entt::entity{2}, entt::entity{6}, entt::entity{1}, entt::entity{4}, entt::entity{5}}; // NOLINT

    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity));
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_EQ(rhs.data()[0u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[0u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[5u]);
}
