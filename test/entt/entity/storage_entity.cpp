#include <algorithm>
#include <array>
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
#include "../../common/config.h"
#include "../../common/linter.hpp"

TEST(StorageEntity, Constructors) {
    entt::storage<entt::entity> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<void>());

    pool = entt::storage<entt::entity>{std::allocator<entt::entity>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<void>());
}

TEST(StorageEntity, Move) {
    entt::storage<entt::entity> pool;
    const std::array entity{entt::entity{3}, entt::entity{2}};

    pool.generate(entity[0u]);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    entt::storage<entt::entity> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<void>());
    ASSERT_EQ(other.index(entity[0u]), 0u);

    entt::storage<entt::entity> extended{std::move(other), std::allocator<entt::entity>{}};

    test::is_initialized(other);

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(extended.info(), entt::type_id<void>());
    ASSERT_EQ(extended.index(entity[0u]), 0u);

    pool = std::move(extended);
    test::is_initialized(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(pool.info(), entt::type_id<void>());
    ASSERT_EQ(pool.index(entity[0u]), 0u);

    other = entt::storage<entt::entity>{};
    other.generate(entity[1u]);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<void>());
    ASSERT_EQ(other.index(entity[0u]), 0u);
}

TEST(StorageEntity, Swap) {
    entt::storage<entt::entity> pool;
    entt::storage<entt::entity> other;

    ASSERT_EQ(pool.info(), entt::type_id<void>());
    ASSERT_EQ(other.info(), entt::type_id<void>());

    pool.generate(entt::entity{4});

    other.generate(entt::entity{2});
    other.generate(entt::entity{1});
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 2u);

    pool.swap(other);

    ASSERT_EQ(pool.info(), entt::type_id<void>());
    ASSERT_EQ(other.info(), entt::type_id<void>());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entt::entity{1}), 0u);
    ASSERT_EQ(other.index(entt::entity{4}), 0u);
}

TEST(StorageEntity, Getters) {
    entt::storage<entt::entity> pool;
    const entt::entity entity{4};

    pool.generate(entity);

    testing::StaticAssertTypeEq<decltype(pool.get({})), void>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), void>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<>>();

    ASSERT_NO_THROW(pool.get(entity));
    ASSERT_NO_THROW(std::as_const(pool).get(entity));

    ASSERT_EQ(pool.get_as_tuple(entity), std::make_tuple());
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entity), std::make_tuple());
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, Getters) {
    entt::storage<entt::entity> pool;
    const entt::entity entity{4};

    ASSERT_DEATH(pool.get(entity), "");
    ASSERT_DEATH(std::as_const(pool).get(entity), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entity), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entity), "");
}

TEST(StorageEntity, Generate) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;
    std::array<entt::entity, 2u> entity{};

    ASSERT_EQ(pool.generate(), entt::entity{0});
    ASSERT_EQ(pool.generate(entt::null), entt::entity{1});
    ASSERT_EQ(pool.generate(entt::tombstone), entt::entity{2});
    ASSERT_EQ(pool.generate(entt::entity{0}), entt::entity{3});
    ASSERT_EQ(pool.generate(traits_type::construct(1, 1)), entt::entity{4});
    ASSERT_EQ(pool.generate(traits_type::construct(6, 3)), traits_type::construct(6, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{1}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{2}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{3}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{4}), pool.free_list());
    ASSERT_EQ(pool.current(entt::entity{5}), traits_type::to_version(entt::tombstone));
    ASSERT_LT(pool.index(traits_type::construct(6, 3)), pool.free_list());

    ASSERT_EQ(pool.generate(traits_type::construct(5, 2)), traits_type::construct(5, 2));
    ASSERT_EQ(pool.generate(traits_type::construct(5, 3)), entt::entity{7});

    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.generate(), traits_type::construct(2, 1));

    pool.erase(traits_type::construct(2, 1));
    pool.generate(entity.begin(), entity.end());

    ASSERT_EQ(entity[0u], traits_type::construct(2, 2));
    ASSERT_EQ(entity[1u], entt::entity{8});
}

TEST(StorageEntity, GenerateRange) {
    entt::storage<entt::entity> pool;
    std::array<entt::entity, 2u> entity{};

    pool.generate(entity.begin(), entity.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.free_list(), 2u);

    pool.erase(entity.begin(), entity.end());

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.free_list(), 0u);

    pool.generate(entity.begin(), entity.begin() + 1u);

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.free_list(), 1u);
}

TEST(StorageEntity, GenerateFrom) {
    entt::storage<entt::entity> pool;
    std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}};

    ASSERT_EQ(pool.generate(), entity[0u]);

    pool.start_from(entity[2u]);

    ASSERT_EQ(pool.generate(), entity[2u]);
    ASSERT_FALSE(pool.contains(entity[1u]));
}

TEST(StorageEntity, GenerateInUse) {
    entt::storage<entt::entity> pool;
    std::array<entt::entity, 2u> entity{};
    const entt::entity other{1};

    ASSERT_EQ(pool.generate(other), other);
    ASSERT_EQ(pool.generate(), entt::entity{0});
    ASSERT_EQ(pool.generate(), entt::entity{2});

    pool.clear();

    ASSERT_EQ(pool.generate(other), other);

    pool.generate(entity.begin(), entity.end());

    ASSERT_EQ(entity[0u], entt::entity{0});
    ASSERT_EQ(entity[1u], entt::entity{2});
}

TEST(StorageEntity, TryGenerate) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;

    ASSERT_EQ(*pool.push(entt::null), entt::entity{0});
    ASSERT_EQ(*pool.push(entt::tombstone), entt::entity{1});
    ASSERT_EQ(*pool.push(entt::entity{0}), entt::entity{2});
    ASSERT_EQ(*pool.push(traits_type::construct(1, 1)), entt::entity{3});
    ASSERT_EQ(*pool.push(traits_type::construct(5, 3)), traits_type::construct(5, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{1}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{2}), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{3}), pool.free_list());
    ASSERT_EQ(pool.current(entt::entity{4}), traits_type::to_version(entt::tombstone));
    ASSERT_LT(pool.index(traits_type::construct(5, 3)), pool.free_list());

    ASSERT_EQ(*pool.push(traits_type::construct(4, 2)), traits_type::construct(4, 2));
    ASSERT_EQ(*pool.push(traits_type::construct(4, 3)), entt::entity{6});

    const std::array entity{entt::entity{1}, traits_type::construct(5, 3)};

    pool.erase(entity.begin(), entity.end());
    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.current(entity[0u]), 1);
    ASSERT_EQ(pool.current(entity[1u]), 4);
    ASSERT_EQ(pool.current(entt::entity{2}), 1);

    ASSERT_LT(pool.index(entt::entity{0}), pool.free_list());
    ASSERT_GE(pool.index(traits_type::construct(1, 1)), pool.free_list());
    ASSERT_GE(pool.index(traits_type::construct(2, 1)), pool.free_list());
    ASSERT_LT(pool.index(entt::entity{3}), pool.free_list());
    ASSERT_LT(pool.index(traits_type::construct(4, 2)), pool.free_list());
    ASSERT_GE(pool.index(traits_type::construct(5, 4)), pool.free_list());

    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(2, 1));
    ASSERT_EQ(*pool.push(traits_type::construct(1, 3)), traits_type::construct(1, 3));
    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(5, 4));
    ASSERT_EQ(*pool.push(entt::null), entt::entity{7});
}

TEST(StorageEntity, TryGenerateInUse) {
    entt::storage<entt::entity> pool;
    std::array<entt::entity, 2u> entity{entt::entity{0}, entt::entity{0}};
    const entt::entity other{1};

    ASSERT_EQ(*pool.push(other), other);
    ASSERT_EQ(*pool.push(other), entt::entity{0});
    ASSERT_EQ(*pool.push(other), entt::entity{2});

    pool.clear();

    ASSERT_EQ(*pool.push(other), other);

    auto it = pool.push(entity.begin(), entity.end());

    ASSERT_EQ(*it, entt::entity{2});
    ASSERT_EQ(*(++it), entt::entity{0});
}

TEST(StorageEntity, Patch) {
    entt::storage<entt::entity> pool;
    const auto entity = pool.generate();

    int counter = 0;
    auto callback = [&counter]() { ++counter; };

    ASSERT_EQ(counter, 0);

    pool.patch(entity);
    pool.patch(entity, callback);
    pool.patch(entity, callback, callback);

    ASSERT_EQ(counter, 3);
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, Patch) {
    entt::storage<entt::entity> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TEST(StorageEntity, Pack) {
    entt::storage<entt::entity> pool;
    std::array entity{entt::entity{1}, entt::entity{3}, entt::entity{4}, entt::entity{2}};

    pool.push(entity.begin(), entity.end());
    pool.erase(entity[3u]);

    std::swap(entity[0u], entity[1u]);

    const auto to = pool.sort_as(entity.begin() + 1u, entity.end());
    auto from = pool.each().cbegin().base();

    ASSERT_NE(from, pool.cbegin());
    ASSERT_NE(from, pool.cend());

    ASSERT_NE(to, pool.cend());
    ASSERT_EQ(to + 1u, pool.cend());

    ASSERT_EQ(*from++, entity[1u]);
    ASSERT_EQ(*from++, entity[2u]);

    ASSERT_NE(from, pool.cend());
    ASSERT_EQ(*from++, entity[0u]);
    ASSERT_EQ(from, pool.cend());
}

TEST(StorageEntity, FreeList) {
    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{0});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.free_list(), 1u);

    pool.free_list(0u);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.free_list(), 0u);

    pool.free_list(1u);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.free_list(), 1u);
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, FreeList) {
    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{0});

    ASSERT_DEATH(pool.free_list(2u), "");
}

TEST(StorageEntity, Iterable) {
    using iterator = typename entt::storage<entt::entity>::iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{1});
    pool.generate(entt::entity{3});
    pool.generate(entt::entity{4});

    pool.erase(entt::entity{3});

    auto iterable = pool.each();

    iterator end{iterable.begin()};
    iterator begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_NE(begin.base(), pool.begin());
    ASSERT_EQ(begin.base(), pool.end() - static_cast<typename iterator::difference_type>(pool.free_list()));
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{4});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{4});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.end() - 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{1});
    pool.generate(entt::entity{3});
    pool.generate(entt::entity{4});

    pool.erase(entt::entity{3});

    auto iterable = std::as_const(pool).each();

    iterator end{iterable.cbegin()};
    iterator begin{};
    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_NE(begin.base(), pool.begin());
    ASSERT_EQ(begin.base(), pool.end() - static_cast<typename iterator::difference_type>(pool.free_list()));
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{4});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{4});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.end() - 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.end());

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, IterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.generate(entt::entity{3});

    const typename entt::storage<entt::entity>::iterable::iterator it = pool.each().begin();
    typename entt::storage<entt::entity>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, IterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.generate(entt::entity{3});

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST(StorageEntity, ReverseIterable) {
    using iterator = typename entt::storage<entt::entity>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{1});
    pool.generate(entt::entity{3});
    pool.generate(entt::entity{4});

    pool.erase(entt::entity{3});

    auto iterable = pool.reach();

    iterator end{iterable.begin()};
    iterator begin{};
    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), pool.rbegin());
    ASSERT_EQ(end.base(), pool.rbegin() + static_cast<typename iterator::difference_type>(pool.free_list()));
    ASSERT_NE(end.base(), pool.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.rbegin() + 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.rbegin() + 2);

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.generate(entt::entity{1});
    pool.generate(entt::entity{3});
    pool.generate(entt::entity{4});

    pool.erase(entt::entity{3});

    auto iterable = std::as_const(pool).reach();

    iterator end{iterable.cbegin()};
    iterator begin{};
    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), pool.rbegin());
    ASSERT_EQ(end.base(), pool.rbegin() + static_cast<typename iterator::difference_type>(pool.free_list()));
    ASSERT_NE(end.base(), pool.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), pool.rbegin() + 1);
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), pool.rbegin() + 2);

    for(auto [entity]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseIterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.generate(entt::entity{3});

    const typename entt::storage<entt::entity>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<entt::entity>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, ReverseIterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.generate(entt::entity{3});

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST(StorageEntity, SortOrdered) {
    entt::storage<entt::entity> pool;
    const std::array entity{entt::entity{16}, entt::entity{8}, entt::entity{4}, entt::entity{2}, entt::entity{1}};

    pool.push(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), pool.begin(), pool.end()));
}

TEST(StorageEntity, SortReverse) {
    entt::storage<entt::entity> pool;
    const std::array entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};

    pool.push(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_TRUE(std::equal(entity.begin(), entity.end(), pool.begin(), pool.end()));
}

TEST(StorageEntity, SortUnordered) {
    entt::storage<entt::entity> pool;
    const std::array entity{entt::entity{4}, entt::entity{2}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    pool.push(entity.begin(), entity.end());
    pool.sort(std::less{});

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[0u]);
    ASSERT_EQ(pool.data()[3u], entity[1u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TEST(StorageEntity, SortN) {
    entt::storage<entt::entity> pool;
    const std::array entity{entt::entity{2}, entt::entity{4}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    pool.push(entity.begin(), entity.end());
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

TEST(StorageEntity, SortAsDisjoint) {
    entt::storage<entt::entity> lhs;
    const entt::storage<entt::entity> rhs;
    const std::array entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};

    lhs.push(entity.begin(), entity.end());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));
}

TEST(StorageEntity, SortAsOverlap) {
    entt::storage<entt::entity> lhs;
    entt::storage<entt::entity> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};
    const std::array rhs_entity{entt::entity{2}};

    lhs.push(lhs_entity.begin(), lhs_entity.end());
    rhs.push(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs.begin(), rhs.end());

    ASSERT_EQ(lhs.data()[0u], lhs_entity[0u]);
    ASSERT_EQ(lhs.data()[1u], lhs_entity[2u]);
    ASSERT_EQ(lhs.data()[2u], lhs_entity[1u]);
}

TEST(StorageEntity, SortAsOrdered) {
    entt::storage<entt::entity> lhs;
    entt::storage<entt::entity> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{32}, entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};

    lhs.push(lhs_entity.begin(), lhs_entity.end());
    rhs.push(rhs_entity.begin(), rhs_entity.end());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.begin(), lhs.end());

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));
}

TEST(StorageEntity, SortAsReverse) {
    entt::storage<entt::entity> lhs;
    entt::storage<entt::entity> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{16}, entt::entity{8}, entt::entity{4}, entt::entity{2}, entt::entity{1}, entt::entity{32}};

    lhs.push(lhs_entity.begin(), lhs_entity.end());
    rhs.push(rhs_entity.begin(), rhs_entity.end());

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

TEST(StorageEntity, SortAsUnordered) {
    entt::storage<entt::entity> lhs;
    entt::storage<entt::entity> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_entity{entt::entity{4}, entt::entity{2}, entt::entity{32}, entt::entity{1}, entt::entity{8}, entt::entity{16}};

    lhs.push(lhs_entity.begin(), lhs_entity.end());
    rhs.push(rhs_entity.begin(), rhs_entity.end());

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
