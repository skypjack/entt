#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/storage.hpp>
#include "../common/config.h"

struct empty_type {};

template<typename Type>
struct StorageNoInstance: testing::Test {
    using type = Type;
};

template<typename Type>
using StorageNoInstanceDeathTest = StorageNoInstance<Type>;

using StorageNoInstanceTypes = ::testing::Types<empty_type, void>;

TYPED_TEST_SUITE(StorageNoInstance, StorageNoInstanceTypes, );
TYPED_TEST_SUITE(StorageNoInstanceDeathTest, StorageNoInstanceTypes, );

TYPED_TEST(StorageNoInstance, Constructors) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());

    pool = entt::storage<value_type>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
}

TYPED_TEST(StorageNoInstance, Getters) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    pool.emplace(entt::entity{41}, 3);

    testing::StaticAssertTypeEq<decltype(pool.get({})), void>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), void>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<>>();

    ASSERT_NO_FATAL_FAILURE(pool.get(entt::entity{41}));
    ASSERT_NO_FATAL_FAILURE(std::as_const(pool).get(entt::entity{41}));

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

TYPED_TEST(StorageNoInstance, Emplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_FATAL_FAILURE(pool.emplace(entity[0u]));

    if constexpr(!std::is_void_v<value_type>) {
        ASSERT_NO_FATAL_FAILURE(pool.emplace(entity[1u], value_type{}));
    }
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Emplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity{42};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_FATAL_FAILURE(pool.emplace(entity));
    ASSERT_DEATH(pool.emplace(entity), "");

    if constexpr(!std::is_void_v<value_type>) {
        ASSERT_DEATH(pool.emplace(entity, value_type{}), "");
    }
}

TYPED_TEST(StorageNoInstance, TryEmplace) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    if constexpr(std::is_void_v<value_type>) {
        ASSERT_NE(base.push(entity[0u], nullptr), base.end());
    } else {
        value_type instance{};

        ASSERT_NE(base.push(entity[0u], &instance), base.end());
    }

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

    entt::entity entity{42};

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

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.index(entity[1u]), 1u);

    if constexpr(!std::is_void_v<value_type>) {
        const value_type values[2u]{};

        pool.erase(std::begin(entity), std::end(entity));
        pool.insert(std::rbegin(entity), std::rend(entity), std::begin(values));

        ASSERT_EQ(pool.size(), 2u);
        ASSERT_EQ(pool.index(entity[0u]), 1u);
        ASSERT_EQ(pool.index(entity[1u]), 0u);
    }
}

ENTT_DEBUG_TYPED_TEST(StorageNoInstanceDeathTest, Insert) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    ASSERT_NO_FATAL_FAILURE(pool.insert(std::begin(entity), std::end(entity)));
    ASSERT_DEATH(pool.insert(std::begin(entity), std::end(entity)), "");

    if constexpr(!std::is_void_v<value_type>) {
        const value_type values[2u]{};

        ASSERT_DEATH(pool.insert(std::begin(entity), std::end(entity), std::begin(values)), "");
    }
}

TYPED_TEST(StorageNoInstance, Iterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;

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
    entt::sparse_set &base = pool;

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

TYPED_TEST(StorageNoInstance, ReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;

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
    entt::sparse_set &base = pool;

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

TYPED_TEST(StorageNoInstance, IterableIteratorConversion) {
    using value_type = typename TestFixture::type;
    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3});

    typename entt::storage<value_type>::iterable::iterator it = pool.each().begin();
    typename entt::storage<value_type>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}
