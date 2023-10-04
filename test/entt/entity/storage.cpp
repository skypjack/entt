#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/component.hpp>
#include <entt/entity/storage.hpp>
#include "../common/config.h"
#include "../common/throwing_allocator.hpp"
#include "../common/throwing_type.hpp"
#include "../common/tracked_memory_resource.hpp"

struct empty_type {};

struct pointer_stability {
    static constexpr auto in_place_delete = true;
    int value;
};

TEST(BasicStorage, Constructors) {
    entt::storage<int> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<int>());

    pool = entt::storage<int>{std::allocator<int>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<int>());
}

TEST(BasicStorage, Move) {
    entt::storage<int> pool;

    pool.emplace(entt::entity{3}, 3);

    ASSERT_TRUE(std::is_move_constructible_v<decltype(pool)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(pool)>);

    entt::storage<int> other{std::move(pool)};

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());

    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});

    ASSERT_EQ(other.get(entt::entity{3}), 3);

    entt::storage<int> extended{std::move(other), std::allocator<int>{}};

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(extended.type(), entt::type_id<int>());

    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(extended.at(0u), entt::entity{3});

    ASSERT_EQ(extended.get(entt::entity{3}), 3);

    pool = std::move(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(extended.type(), entt::type_id<int>());

    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(extended.at(0u), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(pool.get(entt::entity{3}), 3);

    other = entt::storage<int>{};
    other.emplace(entt::entity{42}, 42);
    other = std::move(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());

    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{3});

    ASSERT_EQ(other.get(entt::entity{3}), 3);
}

TEST(BasicStorage, Swap) {
    entt::storage<int> pool;
    entt::storage<int> other;

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());

    pool.emplace(entt::entity{42}, 41);

    other.emplace(entt::entity{9}, 8);
    other.emplace(entt::entity{3}, 2);
    other.erase(entt::entity{9});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.at(0u), entt::entity{3});
    ASSERT_EQ(other.at(0u), entt::entity{42});

    ASSERT_EQ(pool.get(entt::entity{3}), 2);
    ASSERT_EQ(other.get(entt::entity{42}), 41);
}

TEST(BasicStorage, Capacity) {
    entt::storage<int> pool;
    constexpr auto page_size = decltype(pool)::traits_type::page_size;

    pool.reserve(42);

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_TRUE(pool.empty());

    pool.reserve(0);

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_TRUE(pool.empty());
}

TEST(BasicStorage, ShrinkToFit) {
    entt::storage<int> pool;
    constexpr auto page_size = decltype(pool)::traits_type::page_size;

    for(std::size_t next{}; next < page_size; ++next) {
        pool.emplace(entt::entity(next));
    }

    pool.emplace(entt::entity{page_size});
    pool.erase(entt::entity{page_size});

    ASSERT_EQ(pool.capacity(), 2 * page_size);
    ASSERT_EQ(pool.size(), page_size);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_EQ(pool.size(), page_size);

    pool.clear();

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_EQ(pool.size(), 0u);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TEST(BasicStorage, Raw) {
    entt::storage<int> pool;

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{12}, 6);
    pool.emplace(entt::entity{42}, 9);

    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{12}), 6);
    ASSERT_EQ(pool.get(entt::entity{42}), 9);

    ASSERT_EQ(pool.raw()[0u][0u], 3);
    ASSERT_EQ(std::as_const(pool).raw()[0u][1u], 6);
    ASSERT_EQ(pool.raw()[0u][2u], 9);
}

TEST(BasicStorage, Iterator) {
    using iterator = typename entt::storage<int>::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, int>();
    testing::StaticAssertTypeEq<iterator::pointer, int *>();
    testing::StaticAssertTypeEq<iterator::reference, int &>();

    entt::storage<int> pool;
    pool.emplace(entt::entity{3}, 42);

    iterator end{pool.begin()};
    iterator begin{};

    begin = pool.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.begin());
    ASSERT_EQ(end, pool.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin++, pool.begin());
    ASSERT_EQ(begin--, pool.end());

    ASSERT_EQ(begin + 1, pool.end());
    ASSERT_EQ(end - 1, pool.begin());

    ASSERT_EQ(++begin, pool.end());
    ASSERT_EQ(--begin, pool.begin());

    ASSERT_EQ(begin += 1, pool.end());
    ASSERT_EQ(begin -= 1, pool.begin());

    ASSERT_EQ(begin + (end - begin), pool.end());
    ASSERT_EQ(begin - (begin - end), pool.end());

    ASSERT_EQ(end - (end - begin), pool.begin());
    ASSERT_EQ(end + (begin - end), pool.begin());

    ASSERT_EQ(begin[0u], *pool.begin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.end());

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    pool.emplace(entt::entity{42}, 3);
    begin = pool.begin();

    ASSERT_EQ(begin.index(), 1);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin[0u], 3);
    ASSERT_EQ(begin[1u], 42);
}

TEST(BasicStorage, ConstIterator) {
    using iterator = typename entt::storage<int>::const_iterator;

    testing::StaticAssertTypeEq<iterator::value_type, int>();
    testing::StaticAssertTypeEq<iterator::pointer, const int *>();
    testing::StaticAssertTypeEq<iterator::reference, const int &>();

    entt::storage<int> pool;
    pool.emplace(entt::entity{3}, 42);

    iterator cend{pool.cbegin()};
    iterator cbegin{};
    cbegin = pool.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(pool).begin());
    ASSERT_EQ(cend, std::as_const(pool).end());
    ASSERT_EQ(cbegin, pool.cbegin());
    ASSERT_EQ(cend, pool.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin.index(), 0);
    ASSERT_EQ(cend.index(), -1);

    ASSERT_EQ(cbegin++, pool.cbegin());
    ASSERT_EQ(cbegin--, pool.cend());

    ASSERT_EQ(cbegin + 1, pool.cend());
    ASSERT_EQ(cend - 1, pool.cbegin());

    ASSERT_EQ(++cbegin, pool.cend());
    ASSERT_EQ(--cbegin, pool.cbegin());

    ASSERT_EQ(cbegin += 1, pool.cend());
    ASSERT_EQ(cbegin -= 1, pool.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.cend());

    ASSERT_EQ(cend - (cend - cbegin), pool.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.cbegin());

    ASSERT_EQ(cbegin[0u], *pool.cbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.cend());

    ASSERT_EQ(cbegin.index(), 0);
    ASSERT_EQ(cend.index(), -1);

    pool.emplace(entt::entity{42}, 3);
    cbegin = pool.cbegin();

    ASSERT_EQ(cbegin.index(), 1);
    ASSERT_EQ(cend.index(), -1);

    ASSERT_EQ(cbegin[0u], 3);
    ASSERT_EQ(cbegin[1u], 42);
}

TEST(BasicStorage, ReverseIterator) {
    using reverse_iterator = typename entt::storage<int>::reverse_iterator;

    testing::StaticAssertTypeEq<reverse_iterator::value_type, int>();
    testing::StaticAssertTypeEq<reverse_iterator::pointer, int *>();
    testing::StaticAssertTypeEq<reverse_iterator::reference, int &>();

    entt::storage<int> pool;
    pool.emplace(entt::entity{3}, 42);

    reverse_iterator end{pool.rbegin()};
    reverse_iterator begin{};
    begin = pool.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.rbegin());
    ASSERT_EQ(end, pool.rend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    ASSERT_EQ(begin++, pool.rbegin());
    ASSERT_EQ(begin--, pool.rend());

    ASSERT_EQ(begin + 1, pool.rend());
    ASSERT_EQ(end - 1, pool.rbegin());

    ASSERT_EQ(++begin, pool.rend());
    ASSERT_EQ(--begin, pool.rbegin());

    ASSERT_EQ(begin += 1, pool.rend());
    ASSERT_EQ(begin -= 1, pool.rbegin());

    ASSERT_EQ(begin + (end - begin), pool.rend());
    ASSERT_EQ(begin - (begin - end), pool.rend());

    ASSERT_EQ(end - (end - begin), pool.rbegin());
    ASSERT_EQ(end + (begin - end), pool.rbegin());

    ASSERT_EQ(begin[0u], *pool.rbegin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.rbegin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.rend());

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    pool.emplace(entt::entity{42}, 3);
    end = pool.rend();

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 1);

    ASSERT_EQ(begin[0u], 42);
    ASSERT_EQ(begin[1u], 3);
}

TEST(BasicStorage, ConstReverseIterator) {
    using const_reverse_iterator = typename entt::storage<int>::const_reverse_iterator;

    testing::StaticAssertTypeEq<const_reverse_iterator::value_type, int>();
    testing::StaticAssertTypeEq<const_reverse_iterator::pointer, const int *>();
    testing::StaticAssertTypeEq<const_reverse_iterator::reference, const int &>();

    entt::storage<int> pool;
    pool.emplace(entt::entity{3}, 42);

    const_reverse_iterator cend{pool.crbegin()};
    const_reverse_iterator cbegin{};
    cbegin = pool.crend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(pool).rbegin());
    ASSERT_EQ(cend, std::as_const(pool).rend());
    ASSERT_EQ(cbegin, pool.crbegin());
    ASSERT_EQ(cend, pool.crend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 0);

    ASSERT_EQ(cbegin++, pool.crbegin());
    ASSERT_EQ(cbegin--, pool.crend());

    ASSERT_EQ(cbegin + 1, pool.crend());
    ASSERT_EQ(cend - 1, pool.crbegin());

    ASSERT_EQ(++cbegin, pool.crend());
    ASSERT_EQ(--cbegin, pool.crbegin());

    ASSERT_EQ(cbegin += 1, pool.crend());
    ASSERT_EQ(cbegin -= 1, pool.crbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.crend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.crend());

    ASSERT_EQ(cend - (cend - cbegin), pool.crbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.crbegin());

    ASSERT_EQ(cbegin[0u], *pool.crbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.crbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.crend());

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 0);

    pool.emplace(entt::entity{42}, 3);
    cend = pool.crend();

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 1);

    ASSERT_EQ(cbegin[0u], 42);
    ASSERT_EQ(cbegin[1u], 3);
}

TEST(BasicStorage, IteratorConversion) {
    entt::storage<int> pool;
    pool.emplace(entt::entity{3}, 42);

    typename entt::storage<int>::iterator it = pool.begin();
    typename entt::storage<int>::const_iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), int &>();
    testing::StaticAssertTypeEq<decltype(*cit), const int &>();

    ASSERT_EQ(*it.operator->(), 42);
    ASSERT_EQ(*it.operator->(), *cit);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(BasicStorage, IteratorPageSizeAwareness) {
    entt::storage<int> pool;
    constexpr auto page_size = decltype(pool)::traits_type::page_size;
    const int check{42};

    for(unsigned int next{}; next < page_size; ++next) {
        pool.emplace(entt::entity{next});
    }

    pool.emplace(entt::entity{page_size}, check);

    // test the proper use of component traits by the storage iterator
    ASSERT_EQ(*pool.begin(), check);
}

TEST(BasicStorage, Getters) {
    entt::storage<int> pool;

    pool.emplace(entt::entity{41}, 3);

    testing::StaticAssertTypeEq<decltype(pool.get({})), int &>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), const int &>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<int &>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<const int &>>();

    ASSERT_EQ(pool.get(entt::entity{41}), 3);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{41}), 3);

    ASSERT_EQ(pool.get_as_tuple(entt::entity{41}), std::make_tuple(3));
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entt::entity{41}), std::make_tuple(3));
}

ENTT_DEBUG_TEST(BasicStorageDeathTest, Getters) {
    entt::storage<int> pool;

    ASSERT_DEATH([[maybe_unused]] const auto &value = pool.get(entt::entity{41}), "");
    ASSERT_DEATH([[maybe_unused]] const auto &value = std::as_const(pool).get(entt::entity{41}), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entt::entity{41}), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entt::entity{41}), "");
}

// <<<<<<<<<<<< REWORK IN PROGRESS [basic] <<<<<<<<<<<<

TEST(EmptyType, Constructors) {
    entt::storage<empty_type> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<empty_type>());

    pool = entt::storage<empty_type>{std::allocator<empty_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<empty_type>());
}

TEST(EmptyType, Getters) {
    entt::storage<empty_type> pool;

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

ENTT_DEBUG_TEST(EmptyTypeDeathTest, Getters) {
    entt::storage<empty_type> pool;

    ASSERT_DEATH(pool.get(entt::entity{41}), "");
    ASSERT_DEATH(std::as_const(pool).get(entt::entity{41}), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entt::entity{41}), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entt::entity{41}), "");
}

TEST(EmptyType, Emplace) {
    entt::storage<empty_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    empty_type instance{};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_FATAL_FAILURE(pool.emplace(entity[0u]));
    ASSERT_NO_FATAL_FAILURE(pool.emplace(entity[1u], instance));
}

ENTT_DEBUG_TEST(EmptyTypeDeathTest, Emplace) {
    entt::storage<empty_type> pool;

    const entt::entity entity{42};
    empty_type instance{};

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), void>();

    ASSERT_NO_FATAL_FAILURE(pool.emplace(entity));

    ASSERT_DEATH(pool.emplace(entity), "");
    ASSERT_DEATH(pool.emplace(entity, instance), "");
}

TEST(EmptyType, TryEmplace) {
    entt::storage<empty_type> pool;
    entt::sparse_set &base = pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    empty_type instance{};

    ASSERT_NE(base.push(entity[0u], &instance), base.end());

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

TEST(EmptyType, Patch) {
    entt::storage<empty_type> pool;
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

ENTT_DEBUG_TEST(EmptyTypeDeathTest, Patch) {
    entt::storage<empty_type> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TEST(EmptyType, Insert) {
    entt::storage<empty_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    const empty_type values[2u]{};

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.index(entity[1u]), 1u);

    pool.erase(std::begin(entity), std::end(entity));
    pool.insert(std::rbegin(entity), std::rend(entity), std::begin(values));

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
}

ENTT_DEBUG_TEST(EmptyTypeDeathTest, Insert) {
    entt::storage<empty_type> pool;

    const entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    const empty_type values[2u]{};

    ASSERT_NO_FATAL_FAILURE(pool.insert(std::begin(entity), std::end(entity)));

    ASSERT_DEATH(pool.insert(std::begin(entity), std::end(entity)), "");
    ASSERT_DEATH(pool.insert(std::begin(entity), std::end(entity), std::begin(values)), "");
}

TEST(EmptyType, Iterable) {
    using iterator = typename entt::storage<empty_type>::iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<empty_type> pool;
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

TEST(EmptyType, ConstIterable) {
    using iterator = typename entt::storage<empty_type>::const_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<empty_type> pool;
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

TEST(EmptyType, ReverseIterable) {
    using iterator = typename entt::storage<empty_type>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<empty_type> pool;
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

TEST(EmptyType, ConstReverseIterable) {
    using iterator = typename entt::storage<empty_type>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<empty_type> pool;
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

TEST(EmptyType, IterableIteratorConversion) {
    entt::storage<empty_type> pool;
    pool.emplace(entt::entity{3});

    typename entt::storage<empty_type>::iterable::iterator it = pool.each().begin();
    typename entt::storage<empty_type>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

// <<<<<<<<<<<< REWORK IN PROGRESS [empty] <<<<<<<<<<<<

TEST(Void, Constructors) {
    entt::storage<void> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<void>());

    pool = entt::storage<void>{std::allocator<void>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<void>());
}

// <<<<<<<<<<<< REWORK IN PROGRESS [void] <<<<<<<<<<<<

TEST(PointerStability, Constructors) {
    entt::storage<pointer_stability> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::in_place);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<pointer_stability>());

    pool = entt::storage<pointer_stability>{std::allocator<pointer_stability>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::in_place);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<pointer_stability>());
}

// <<<<<<<<<<<< REWORK IN PROGRESS [stable] <<<<<<<<<<<<

TEST(Entity, Constructors) {
    entt::storage<entt::entity> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());

    pool = entt::storage<entt::entity>{std::allocator<entt::entity>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
}

// <<<<<<<<<<<< REWORK IN PROGRESS [entity] <<<<<<<<<<<<

struct pinned_type {
    const int value{42};
};

struct empty_stable_type {
    static constexpr auto in_place_delete = true;
};

struct boxed_int {
    int value;
};

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value;
};

struct aggregate_tracking_type {
    ~aggregate_tracking_type() {
        ++counter;
    }

    static inline int counter;
    int value;
};

struct non_default_constructible {
    non_default_constructible() = delete;

    non_default_constructible(int v)
        : value{v} {}

    int value;
};

struct update_from_destructor {
    update_from_destructor(entt::storage<update_from_destructor> &ref, entt::entity other)
        : storage{&ref},
          target{other} {}

    update_from_destructor(update_from_destructor &&other) noexcept
        : storage{std::exchange(other.storage, nullptr)},
          target{std::exchange(other.target, entt::null)} {}

    update_from_destructor &operator=(update_from_destructor &&other) noexcept {
        storage = std::exchange(other.storage, nullptr);
        target = std::exchange(other.target, entt::null);
        return *this;
    }

    ~update_from_destructor() {
        if(target != entt::null && storage->contains(target)) {
            storage->erase(target);
        }
    }

private:
    entt::storage<update_from_destructor> *storage{};
    entt::entity target{entt::null};
};

struct create_from_constructor {
    create_from_constructor(entt::storage<create_from_constructor> &ref, entt::entity other)
        : child{other} {
        if(child != entt::null) {
            ref.emplace(child, ref, entt::null);
        }
    }

    entt::entity child;
};

template<>
struct entt::component_traits<std::unordered_set<char>> {
    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 4u;
};

inline bool operator==(const boxed_int &lhs, const boxed_int &rhs) {
    return lhs.value == rhs.value;
}

struct StorageOld: ::testing::Test {
    void SetUp() override {
        aggregate_tracking_type::counter = 0;
    }
};

using StorageOldDeathTest = StorageOld;

TEST_F(StorageOld, Functionalities) {
    entt::storage<int> pool;
    constexpr auto page_size = decltype(pool)::traits_type::page_size;

    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<int>());

    pool.reserve(42);

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    pool.reserve(0);

    ASSERT_EQ(pool.capacity(), page_size);
    ASSERT_TRUE(pool.empty());

    pool.emplace(entt::entity{41}, 3);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_NE(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_NE(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.contains(entt::entity{41}));

    ASSERT_EQ(pool.get(entt::entity{41}), 3);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{41}), 3);
    ASSERT_EQ(pool.get_as_tuple(entt::entity{41}), std::make_tuple(3));
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entt::entity{41}), std::make_tuple(3));

    pool.erase(entt::entity{41});

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    pool.emplace(entt::entity{41}, 12);

    ASSERT_EQ(pool.get(entt::entity{41}), 12);
    ASSERT_EQ(std::as_const(pool).get(entt::entity{41}), 12);
    ASSERT_EQ(pool.get_as_tuple(entt::entity{41}), std::make_tuple(12));
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entt::entity{41}), std::make_tuple(12));

    pool.clear();

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(std::as_const(pool).begin(), std::as_const(pool).end());
    ASSERT_EQ(pool.begin(), pool.end());
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_FALSE(pool.contains(entt::entity{41}));

    ASSERT_EQ(pool.capacity(), page_size);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);
}

TEST_F(StorageOld, StableSwap) {
    entt::storage<stable_type> pool;
    entt::storage<stable_type> other;

    pool.emplace(entt::entity{42}, 41);

    other.emplace(entt::entity{9}, 8);
    other.emplace(entt::entity{3}, 2);
    other.erase(entt::entity{9});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 2u);

    pool.swap(other);

    ASSERT_EQ(pool.type(), entt::type_id<stable_type>());
    ASSERT_EQ(other.type(), entt::type_id<stable_type>());

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.at(1u), entt::entity{3});
    ASSERT_EQ(pool.get(entt::entity{3}).value, 2);

    ASSERT_EQ(other.at(0u), entt::entity{42});
    ASSERT_EQ(other.get(entt::entity{42}).value, 41);
}

TEST_F(StorageOld, VoidType) {
    entt::storage<void> pool;
    pool.emplace(entt::entity{99});

    ASSERT_EQ(pool.type(), entt::type_id<void>());
    ASSERT_TRUE(pool.contains(entt::entity{99}));

    entt::storage<void> other{std::move(pool)};

    ASSERT_FALSE(pool.contains(entt::entity{99}));
    ASSERT_TRUE(other.contains(entt::entity{99}));

    pool = std::move(other);

    ASSERT_TRUE(pool.contains(entt::entity{99}));
    ASSERT_FALSE(other.contains(entt::entity{99}));
}

TEST_F(StorageOld, Patch) {
    entt::storage<int> pool;
    entt::entity entity{42};
    auto callback = [](int &value) { ++value; };

    pool.emplace(entity, 0);

    ASSERT_EQ(pool.get(entity), 0);

    pool.patch(entity);
    pool.patch(entity, callback);
    pool.patch(entity, callback, callback);

    ASSERT_EQ(pool.get(entity), 3);
}

ENTT_DEBUG_TEST_F(StorageOldDeathTest, Patch) {
    entt::storage<int> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TEST_F(StorageOld, Insert) {
    entt::storage<stable_type> pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    entt::storage<stable_type>::iterator it{};

    it = pool.insert(std::begin(entity), std::end(entity), stable_type{99});

    ASSERT_EQ(it, pool.cbegin());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.get(entity[0u]).value, 99);
    ASSERT_EQ(pool.get(entity[1u]).value, 99);
    ASSERT_EQ(it++->value, 99);
    ASSERT_EQ(it->value, 99);

    pool.erase(std::begin(entity), std::end(entity));
    const stable_type values[2u] = {stable_type{42}, stable_type{3}};
    it = pool.insert(std::rbegin(entity), std::rend(entity), std::begin(values));

    ASSERT_EQ(it, pool.cbegin());

    ASSERT_EQ(pool.size(), 4u);
    ASSERT_EQ(pool.at(2u), entity[1u]);
    ASSERT_EQ(pool.at(3u), entity[0u]);
    ASSERT_EQ(pool.index(entity[0u]), 3u);
    ASSERT_EQ(pool.index(entity[1u]), 2u);
    ASSERT_EQ(pool.get(entity[0u]).value, 3);
    ASSERT_EQ(pool.get(entity[1u]).value, 42);
    ASSERT_EQ(it++->value, 3);
    ASSERT_EQ(it->value, 42);
}

TEST_F(StorageOld, Erase) {
    entt::storage<int> pool;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, entt::entity{9}};

    pool.emplace(entity[0u]);
    pool.emplace(entity[1u]);
    pool.emplace(entity[2u]);
    pool.erase(std::begin(entity), std::end(entity));

    ASSERT_TRUE(pool.empty());

    pool.emplace(entity[0u], 0);
    pool.emplace(entity[1u], 1);
    pool.emplace(entity[2u], 2);
    pool.erase(entity, entity + 2u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 2);

    pool.erase(entity[2u]);

    ASSERT_TRUE(pool.empty());

    pool.emplace(entity[0u], 0);
    pool.emplace(entity[1u], 1);
    pool.emplace(entity[2u], 2);
    std::swap(entity[1u], entity[2u]);
    pool.erase(entity, entity + 2u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 1);
}

TEST_F(StorageOld, CrossErase) {
    entt::sparse_set set;
    entt::storage<int> pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    pool.emplace(entity[0u], 3);
    pool.emplace(entity[1u], 42);
    set.push(entity[1u]);
    pool.erase(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u], 3);
}

TEST_F(StorageOld, StableErase) {
    entt::storage<stable_type> pool;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, entt::entity{9}};

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});
    pool.emplace(entity[2u], stable_type{2});

    pool.erase(std::begin(entity), std::end(entity));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.at(2u) == entt::tombstone);

    pool.emplace(entity[2u], stable_type{2});
    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});

    ASSERT_EQ(pool.get(entity[0u]).value, 0);
    ASSERT_EQ(pool.get(entity[1u]).value, 1);
    ASSERT_EQ(pool.get(entity[2u]).value, 2);

    ASSERT_EQ(pool.begin()->value, 2);
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
    ASSERT_EQ(pool.index(entity[2u]), 2u);

    pool.erase(entity, entity + 2u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.begin()->value, 2);
    ASSERT_EQ(pool.index(entity[2u]), 2u);

    pool.erase(entity[2u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_FALSE(pool.contains(entity[2u]));

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});
    pool.emplace(entity[2u], stable_type{2});
    std::swap(entity[1u], entity[2u]);
    pool.erase(entity, entity + 2u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.contains(entity[2u]));
    ASSERT_EQ(pool.index(entity[2u]), 0u);
    ASSERT_EQ(pool.get(entity[2u]).value, 1);

    pool.compact();

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.begin()->value, 1);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{2});
    pool.emplace(entity[2u], stable_type{1});
    pool.erase(entity[2u]);

    pool.erase(entity[0u]);
    pool.erase(entity[1u]);

    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.at(2u) == entt::tombstone);

    pool.emplace(entity[0u], stable_type{99});

    ASSERT_EQ((++pool.begin())->value, 99);

    pool.emplace(entity[1u], stable_type{2});
    pool.emplace(entity[2u], stable_type{1});
    pool.emplace(entt::entity{0}, stable_type{7});

    ASSERT_EQ(pool.size(), 4u);
    ASSERT_EQ(pool.begin()->value, 7);
    ASSERT_EQ(pool.at(0u), entity[1u]);
    ASSERT_EQ(pool.at(1u), entity[0u]);
    ASSERT_EQ(pool.at(2u), entity[2u]);

    ASSERT_EQ(pool.get(entity[0u]).value, 99);
    ASSERT_EQ(pool.get(entity[1u]).value, 2);
    ASSERT_EQ(pool.get(entity[2u]).value, 1);
}

TEST_F(StorageOld, CrossStableErase) {
    entt::sparse_set set;
    entt::storage<stable_type> pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    pool.emplace(entity[0u], 3);
    pool.emplace(entity[1u], 42);
    set.push(entity[1u]);
    pool.erase(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u].value, 3);
}

TEST_F(StorageOld, Remove) {
    entt::storage<int> pool;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, entt::entity{9}};

    pool.emplace(entity[0u]);
    pool.emplace(entity[1u]);
    pool.emplace(entity[2u]);

    ASSERT_EQ(pool.remove(std::begin(entity), std::end(entity)), 3u);
    ASSERT_EQ(pool.remove(std::begin(entity), std::end(entity)), 0u);
    ASSERT_TRUE(pool.empty());

    pool.emplace(entity[0u], 0);
    pool.emplace(entity[1u], 1);
    pool.emplace(entity[2u], 2);

    ASSERT_EQ(pool.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 2);

    ASSERT_TRUE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.remove(entity[2u]));
    ASSERT_TRUE(pool.empty());

    pool.emplace(entity[0u], 0);
    pool.emplace(entity[1u], 1);
    pool.emplace(entity[2u], 2);
    std::swap(entity[1u], entity[2u]);

    ASSERT_EQ(pool.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(*pool.begin(), 1);
}

TEST_F(StorageOld, CrossRemove) {
    entt::sparse_set set;
    entt::storage<int> pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    pool.emplace(entity[0u], 3);
    pool.emplace(entity[1u], 42);
    set.push(entity[1u]);
    pool.remove(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u], 3);
}

TEST_F(StorageOld, StableRemove) {
    entt::storage<stable_type> pool;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, entt::entity{9}};

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});
    pool.emplace(entity[2u], stable_type{2});

    ASSERT_EQ(pool.remove(std::begin(entity), std::end(entity)), 3u);
    ASSERT_EQ(pool.remove(std::begin(entity), std::end(entity)), 0u);
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.at(2u) == entt::tombstone);

    pool.emplace(entity[2u], stable_type{2});
    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});

    ASSERT_EQ(pool.get(entity[0u]).value, 0);
    ASSERT_EQ(pool.get(entity[1u]).value, 1);
    ASSERT_EQ(pool.get(entity[2u]).value, 2);

    ASSERT_EQ(pool.begin()->value, 2);
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
    ASSERT_EQ(pool.index(entity[2u]), 2u);

    ASSERT_EQ(pool.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(pool.begin()->value, 2);
    ASSERT_EQ(pool.index(entity[2u]), 2u);

    ASSERT_TRUE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_FALSE(pool.contains(entity[2u]));

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{1});
    pool.emplace(entity[2u], stable_type{2});
    std::swap(entity[1u], entity[2u]);

    ASSERT_EQ(pool.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.contains(entity[2u]));
    ASSERT_EQ(pool.index(entity[2u]), 0u);
    ASSERT_EQ(pool.get(entity[2u]).value, 1);

    pool.compact();

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.begin()->value, 1);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);

    pool.emplace(entity[0u], stable_type{0});
    pool.emplace(entity[1u], stable_type{2});
    pool.emplace(entity[2u], stable_type{1});

    ASSERT_TRUE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.remove(entity[2u]));

    ASSERT_TRUE(pool.remove(entity[0u]));
    ASSERT_TRUE(pool.remove(entity[1u]));
    ASSERT_EQ(pool.remove(entity, entity + 2u), 0u);

    ASSERT_EQ(pool.size(), 3u);
    ASSERT_TRUE(pool.at(2u) == entt::tombstone);

    pool.emplace(entity[0u], stable_type{99});

    ASSERT_EQ((++pool.begin())->value, 99);

    pool.emplace(entity[1u], stable_type{2});
    pool.emplace(entity[2u], stable_type{1});
    pool.emplace(entt::entity{0}, stable_type{7});

    ASSERT_EQ(pool.size(), 4u);
    ASSERT_EQ(pool.begin()->value, 7);
    ASSERT_EQ(pool.at(0u), entity[1u]);
    ASSERT_EQ(pool.at(1u), entity[0u]);
    ASSERT_EQ(pool.at(2u), entity[2u]);

    ASSERT_EQ(pool.get(entity[0u]).value, 99);
    ASSERT_EQ(pool.get(entity[1u]).value, 2);
    ASSERT_EQ(pool.get(entity[2u]).value, 1);
}

TEST_F(StorageOld, CrossStableRemove) {
    entt::sparse_set set;
    entt::storage<stable_type> pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    pool.emplace(entity[0u], 3);
    pool.emplace(entity[1u], 42);
    set.push(entity[1u]);
    pool.remove(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u].value, 3);
}

TEST_F(StorageOld, TypeFromBase) {
    entt::storage<int> pool;
    entt::sparse_set &base = pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    ASSERT_EQ(pool.type(), entt::type_id<int>());
    ASSERT_EQ(pool.type(), base.type());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    int instance = 42;

    ASSERT_NE(base.push(entity[0u], &instance), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(base.value(entity[0u]), &pool.get(entity[0u]));
    ASSERT_EQ(pool.get(entity[0u]), 42);

    base.erase(entity[0u]);

    ASSERT_NE(base.push(std::begin(entity), std::end(entity)), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.get(entity[0u]), 0);
    ASSERT_EQ(pool.get(entity[1u]), 0);

    base.erase(std::begin(entity), std::end(entity));

    ASSERT_TRUE(pool.empty());
}

TEST_F(StorageOld, NonDefaultConstructibleTypeFromBase) {
    entt::storage<non_default_constructible> pool;
    entt::sparse_set &base = pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    ASSERT_EQ(pool.type(), entt::type_id<non_default_constructible>());
    ASSERT_EQ(pool.type(), base.type());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_EQ(base.push(entity[0u]), base.end());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(base.find(entity[0u]), base.end());
    ASSERT_TRUE(pool.empty());

    non_default_constructible instance{3};

    ASSERT_NE(base.push(entity[0u], &instance), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    base.erase(entity[0u]);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.contains(entity[0u]));

    ASSERT_EQ(base.push(std::begin(entity), std::end(entity)), base.end());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(base.find(entity[0u]), base.end());
    ASSERT_EQ(base.find(entity[1u]), base.end());
    ASSERT_TRUE(pool.empty());
}

TEST_F(StorageOld, NonCopyConstructibleTypeFromBase) {
    entt::storage<std::unique_ptr<int>> pool;
    entt::sparse_set &base = pool;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    ASSERT_EQ(pool.type(), entt::type_id<std::unique_ptr<int>>());
    ASSERT_EQ(pool.type(), base.type());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_NE(base.push(entity[0u]), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_NE(base.find(entity[0u]), base.end());
    ASSERT_FALSE(pool.empty());

    std::unique_ptr<int> instance = std::make_unique<int>(3);

    ASSERT_EQ(base.push(entity[1u], &instance), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    base.erase(entity[0u]);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.contains(entity[0u]));

    ASSERT_NE(base.push(std::begin(entity), std::end(entity)), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_NE(base.find(entity[0u]), base.end());
    ASSERT_NE(base.find(entity[1u]), base.end());
    ASSERT_FALSE(pool.empty());
}

TEST_F(StorageOld, Compact) {
    entt::storage<stable_type> pool;

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);

    pool.compact();

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);

    pool.emplace(entt::entity{0}, stable_type{0});
    pool.compact();

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 1u);

    pool.emplace(entt::entity{42}, stable_type{42});
    pool.erase(entt::entity{0});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entt::entity{42}), 1u);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 42);

    pool.compact();

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.index(entt::entity{42}), 0u);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 42);

    pool.emplace(entt::entity{0}, stable_type{0});
    pool.compact();

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entt::entity{42}), 0u);
    ASSERT_EQ(pool.index(entt::entity{0}), 1u);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 42);
    ASSERT_EQ(pool.get(entt::entity{0}).value, 0);

    pool.erase(entt::entity{0});
    pool.erase(entt::entity{42});
    pool.compact();

    ASSERT_TRUE(pool.empty());
}

TEST_F(StorageOld, AggregatesMustWork) {
    ASSERT_TRUE(std::is_aggregate_v<aggregate_tracking_type>);
    entt::storage<aggregate_tracking_type> storage{};

    // aggregate types with no args enter the non-aggregate path
    storage.emplace(entt::entity{0});

    ASSERT_EQ(aggregate_tracking_type::counter, 0);

    // aggregate types with args work despite the lack of support in the standard library
    storage.emplace(entt::entity{1}, 42);

    ASSERT_EQ(aggregate_tracking_type::counter, 1);
}

TEST_F(StorageOld, SelfMoveSupport) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<std::unordered_set<int>> pool;
    entt::entity entity{};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);

    pool.emplace(entity).insert(42);
    pool.erase(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TEST_F(StorageOld, SelfMoveSupportInPlaceDelete) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<std::unordered_set<char>> pool;
    entt::entity entity{};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::in_place);

    pool.emplace(entity).insert(42);
    pool.erase(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TEST_F(StorageOld, Iterable) {
    using iterator = typename entt::storage<boxed_int>::iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity, boxed_int &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, boxed_int &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<boxed_int> pool;
    entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 99);
    pool.emplace(entt::entity{3}, 42);

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
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), boxed_int{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin), boxed_int{42});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), boxed_int &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == boxed_int{99});
        ASSERT_TRUE(entity != entt::entity{3} || element == boxed_int{42});
    }
}

TEST_F(StorageOld, ConstIterable) {
    using iterator = typename entt::storage<boxed_int>::const_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity, const boxed_int &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, const boxed_int &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<boxed_int> pool;
    entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 99);
    pool.emplace(entt::entity{3}, 42);

    auto iterable = std::as_const(pool).each();

    iterator end{iterable.cbegin()};
    iterator begin{};

    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.begin());
    ASSERT_EQ(end.base(), base.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), boxed_int{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin), boxed_int{42});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), const boxed_int &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == boxed_int{99});
        ASSERT_TRUE(entity != entt::entity{3} || element == boxed_int{42});
    }
}

TEST_F(StorageOld, IterableIteratorConversion) {
    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    typename entt::storage<boxed_int>::iterable::iterator it = pool.each().begin();
    typename entt::storage<boxed_int>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity, boxed_int &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity, const boxed_int &>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST_F(StorageOld, IterableAlgorithmCompatibility) {
    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST_F(StorageOld, ReverseIterable) {
    using iterator = typename entt::storage<boxed_int>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity, boxed_int &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, boxed_int &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<boxed_int> pool;
    entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 99);
    pool.emplace(entt::entity{3}, 42);

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
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), boxed_int{99});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin), boxed_int{99});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), boxed_int &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == boxed_int{99});
        ASSERT_TRUE(entity != entt::entity{3} || element == boxed_int{42});
    }
}

TEST_F(StorageOld, ConstReverseIterable) {
    using iterator = typename entt::storage<boxed_int>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity, const boxed_int &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, const boxed_int &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<boxed_int> pool;
    entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 99);
    pool.emplace(entt::entity{3}, 42);

    auto iterable = std::as_const(pool).reach();

    iterator end{iterable.cbegin()};
    iterator begin{};

    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.rbegin());
    ASSERT_EQ(end.base(), base.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), boxed_int{99});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin), boxed_int{99});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), const boxed_int &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == boxed_int{99});
        ASSERT_TRUE(entity != entt::entity{3} || element == boxed_int{42});
    }
}

TEST_F(StorageOld, ReverseIterableIteratorConversion) {
    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    typename entt::storage<boxed_int>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<boxed_int>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity, boxed_int &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity, const boxed_int &>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST_F(StorageOld, ReverseIterableAlgorithmCompatibility) {
    entt::storage<boxed_int> pool;
    pool.emplace(entt::entity{3}, 42);

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST_F(StorageOld, SwapElements) {
    entt::storage<int> pool;

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{12}, 6);
    pool.emplace(entt::entity{42}, 9);

    pool.erase(entt::entity{12});

    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(pool.get(entt::entity{42}), 9);
    ASSERT_EQ(pool.index(entt::entity{3}), 0u);
    ASSERT_EQ(pool.index(entt::entity{42}), 1u);

    pool.swap_elements(entt::entity{3}, entt::entity{42});

    ASSERT_EQ(pool.get(entt::entity{3}), 3);
    ASSERT_EQ(pool.get(entt::entity{42}), 9);
    ASSERT_EQ(pool.index(entt::entity{3}), 1u);
    ASSERT_EQ(pool.index(entt::entity{42}), 0u);
}

TEST_F(StorageOld, StableSwapElements) {
    entt::storage<stable_type> pool;

    pool.emplace(entt::entity{3}, 3);
    pool.emplace(entt::entity{12}, 6);
    pool.emplace(entt::entity{42}, 9);

    pool.erase(entt::entity{12});

    ASSERT_EQ(pool.get(entt::entity{3}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 9);
    ASSERT_EQ(pool.index(entt::entity{3}), 0u);
    ASSERT_EQ(pool.index(entt::entity{42}), 2u);

    pool.swap_elements(entt::entity{3}, entt::entity{42});

    ASSERT_EQ(pool.get(entt::entity{3}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{42}).value, 9);
    ASSERT_EQ(pool.index(entt::entity{3}), 2u);
    ASSERT_EQ(pool.index(entt::entity{42}), 0u);
}

TEST_F(StorageOld, SortOrdered) {
    entt::storage<boxed_int> pool;
    entt::entity entity[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{12}, {9}, {6}, {3}, {1}};

    pool.insert(std::begin(entity), std::end(entity), values);
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(values), std::rend(values), pool.begin(), pool.end()));
}

TEST_F(StorageOld, SortReverse) {
    entt::storage<boxed_int> pool;
    entt::entity entity[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{1}, {3}, {6}, {9}, {12}};

    pool.insert(std::begin(entity), std::end(entity), values);
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    ASSERT_TRUE(std::equal(std::begin(entity), std::end(entity), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::begin(values), std::end(values), pool.begin(), pool.end()));
}

TEST_F(StorageOld, SortUnordered) {
    entt::storage<boxed_int> pool;
    entt::entity entity[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{6}, {3}, {1}, {9}, {12}};

    pool.insert(std::begin(entity), std::end(entity), values);
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), values[2u]);
    ASSERT_EQ(*(begin++), values[1u]);
    ASSERT_EQ(*(begin++), values[0u]);
    ASSERT_EQ(*(begin++), values[3u]);
    ASSERT_EQ(*(begin++), values[4u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[0u]);
    ASSERT_EQ(pool.data()[3u], entity[1u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TEST_F(StorageOld, SortRange) {
    entt::storage<boxed_int> pool;
    entt::entity entity[5u]{entt::entity{12}, entt::entity{42}, entt::entity{7}, entt::entity{3}, entt::entity{9}};
    boxed_int values[5u]{{3}, {6}, {1}, {9}, {12}};

    pool.insert(std::begin(entity), std::end(entity), values);
    pool.sort_n(0u, [&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(values), std::rend(values), pool.begin(), pool.end()));

    pool.sort_n(2u, [&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    ASSERT_EQ(pool.raw()[0u][0u], values[1u]);
    ASSERT_EQ(pool.raw()[0u][1u], values[0u]);
    ASSERT_EQ(pool.raw()[0u][2u], values[2u]);

    ASSERT_EQ(pool.data()[0u], entity[1u]);
    ASSERT_EQ(pool.data()[1u], entity[0u]);
    ASSERT_EQ(pool.data()[2u], entity[2u]);

    pool.sort_n(5u, [&pool](auto lhs, auto rhs) { return pool.get(lhs).value < pool.get(rhs).value; });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), values[2u]);
    ASSERT_EQ(*(begin++), values[0u]);
    ASSERT_EQ(*(begin++), values[1u]);
    ASSERT_EQ(*(begin++), values[3u]);
    ASSERT_EQ(*(begin++), values[4u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[1u]);
    ASSERT_EQ(pool.data()[3u], entity[0u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TEST_F(StorageOld, RespectDisjoint) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    int lhs_values[3u]{3, 6, 9};
    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity), lhs_values);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));
}

TEST_F(StorageOld, RespectOverlap) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    int lhs_values[3u]{3, 6, 9};
    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity), lhs_values);

    entt::entity rhs_entity[1u]{entt::entity{12}};
    int rhs_values[1u]{6};
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity), rhs_values);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), lhs_values[1u]);
    ASSERT_EQ(*(begin++), lhs_values[2u]);
    ASSERT_EQ(*(begin++), lhs_values[0u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(lhs.data()[0u], lhs_entity[0u]);
    ASSERT_EQ(lhs.data()[1u], lhs_entity[2u]);
    ASSERT_EQ(lhs.data()[2u], lhs_entity[1u]);
}

TEST_F(StorageOld, RespectOrdered) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity), lhs_values);

    entt::entity rhs_entity[6u]{entt::entity{6}, entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int rhs_values[6u]{6, 1, 2, 3, 4, 5};
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity), rhs_values);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));
}

TEST_F(StorageOld, RespectReverse) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity), lhs_values);

    entt::entity rhs_entity[6u]{entt::entity{5}, entt::entity{4}, entt::entity{3}, entt::entity{2}, entt::entity{1}, entt::entity{6}};
    int rhs_values[6u]{5, 4, 3, 2, 1, 6};
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity), rhs_values);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_values[0u]);
    ASSERT_EQ(*(begin++), rhs_values[1u]);
    ASSERT_EQ(*(begin++), rhs_values[2u]);
    ASSERT_EQ(*(begin++), rhs_values[3u]);
    ASSERT_EQ(*(begin++), rhs_values[4u]);
    ASSERT_EQ(*(begin++), rhs_values[5u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], rhs_entity[5u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[0u]);
}

TEST_F(StorageOld, RespectUnordered) {
    entt::storage<int> lhs;
    entt::storage<int> rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    int lhs_values[5u]{1, 2, 3, 4, 5};
    lhs.insert(std::begin(lhs_entity), std::end(lhs_entity), lhs_values);

    entt::entity rhs_entity[6u]{entt::entity{3}, entt::entity{2}, entt::entity{6}, entt::entity{1}, entt::entity{4}, entt::entity{5}};
    int rhs_values[6u]{3, 2, 6, 1, 4, 5};
    rhs.insert(std::begin(rhs_entity), std::end(rhs_entity), rhs_values);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(lhs_values), std::rend(lhs_values), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_values), std::rend(rhs_values), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_values[5u]);
    ASSERT_EQ(*(begin++), rhs_values[4u]);
    ASSERT_EQ(*(begin++), rhs_values[0u]);
    ASSERT_EQ(*(begin++), rhs_values[1u]);
    ASSERT_EQ(*(begin++), rhs_values[3u]);
    ASSERT_EQ(*(begin++), rhs_values[2u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[0u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[5u]);
}

TEST_F(StorageOld, CanModifyDuringIteration) {
    entt::storage<int> pool;
    auto *ptr = &pool.emplace(entt::entity{0}, 42);
    constexpr auto page_size = decltype(pool)::traits_type::page_size;

    ASSERT_EQ(pool.capacity(), page_size);

    const auto it = pool.cbegin();
    pool.reserve(page_size + 1u);

    ASSERT_EQ(pool.capacity(), 2 * page_size);
    ASSERT_EQ(&pool.get(entt::entity{0}), ptr);

    // this should crash with asan enabled if we break the constraint
    [[maybe_unused]] const int &value = *it;
}

TEST_F(StorageOld, ReferencesGuaranteed) {
    entt::storage<boxed_int> pool;

    pool.emplace(entt::entity{0}, 0);
    pool.emplace(entt::entity{1}, 1);

    ASSERT_EQ(pool.get(entt::entity{0}).value, 0);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 1);

    for(auto &&type: pool) {
        if(type.value) {
            type.value = 42;
        }
    }

    ASSERT_EQ(pool.get(entt::entity{0}).value, 0);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 42);

    auto begin = pool.begin();

    while(begin != pool.end()) {
        (begin++)->value = 3;
    }

    ASSERT_EQ(pool.get(entt::entity{0}).value, 3);
    ASSERT_EQ(pool.get(entt::entity{1}).value, 3);
}

TEST_F(StorageOld, MoveOnlyComponent) {
    // the purpose is to ensure that move only components are always accepted
    [[maybe_unused]] entt::storage<std::unique_ptr<int>> pool;
}

TEST_F(StorageOld, PinnedComponent) {
    // the purpose is to ensure that non-movable components are always accepted
    [[maybe_unused]] entt::storage<pinned_type> pool;
}

ENTT_DEBUG_TEST_F(StorageOldDeathTest, PinnedComponent) {
    entt::storage<pinned_type> pool;
    const entt::entity entity{0};
    const entt::entity destroy{1};
    const entt::entity other{2};

    pool.emplace(entity);
    pool.emplace(destroy);
    pool.emplace(other);

    pool.erase(destroy);

    ASSERT_DEATH(pool.swap_elements(entity, other), "");
    ASSERT_DEATH(pool.compact(), "");
    ASSERT_DEATH(pool.sort([](auto &&lhs, auto &&rhs) { return lhs < rhs; }), "");
}

TEST_F(StorageOld, UpdateFromDestructor) {
    auto test = [](const auto target) {
        constexpr auto size = 10u;

        entt::storage<update_from_destructor> pool;

        for(std::size_t next{}; next < size; ++next) {
            const auto entity = entt::entity(next);
            pool.emplace(entity, pool, entity == entt::entity(size / 2) ? target : entity);
        }

        pool.erase(entt::entity(size / 2));

        ASSERT_EQ(pool.size(), size - 1u - (target != entt::null));
        ASSERT_FALSE(pool.contains(entt::entity(size / 2)));
        ASSERT_FALSE(pool.contains(target));

        pool.clear();

        ASSERT_TRUE(pool.empty());

        for(std::size_t next{}; next < size; ++next) {
            ASSERT_FALSE(pool.contains(entt::entity(next)));
        }
    };

    test(entt::entity{9u});
    test(entt::entity{8u});
    test(entt::entity{0u});
}

TEST_F(StorageOld, CreateFromConstructor) {
    entt::storage<create_from_constructor> pool;
    const entt::entity entity{0u};
    const entt::entity other{1u};

    pool.emplace(entity, pool, other);

    ASSERT_EQ(pool.get(entity).child, other);
    ASSERT_EQ(pool.get(other).child, static_cast<entt::entity>(entt::null));
}

TEST_F(StorageOld, CustomAllocator) {
    auto test = [](auto pool, auto alloc) {
        pool.reserve(1u);

        ASSERT_NE(pool.capacity(), 0u);

        pool.emplace(entt::entity{0});
        pool.emplace(entt::entity{1});

        decltype(pool) other{std::move(pool), alloc};

        ASSERT_TRUE(pool.empty());
        ASSERT_FALSE(other.empty());
        ASSERT_EQ(pool.capacity(), 0u);
        ASSERT_NE(other.capacity(), 0u);
        ASSERT_EQ(other.size(), 2u);

        pool = std::move(other);

        ASSERT_FALSE(pool.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(other.capacity(), 0u);
        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 2u);

        pool.swap(other);
        pool = std::move(other);

        ASSERT_FALSE(pool.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(other.capacity(), 0u);
        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 2u);

        pool.clear();

        ASSERT_NE(pool.capacity(), 0u);
        ASSERT_EQ(pool.size(), 0u);
    };

    test::throwing_allocator<entt::entity> allocator{};

    test(entt::basic_storage<int, entt::entity, test::throwing_allocator<int>>{allocator}, allocator);
    test(entt::basic_storage<std::true_type, entt::entity, test::throwing_allocator<std::true_type>>{allocator}, allocator);
    test(entt::basic_storage<stable_type, entt::entity, test::throwing_allocator<stable_type>>{allocator}, allocator);
}

TEST_F(StorageOld, ThrowingAllocator) {
    auto test = [](auto pool) {
        using pool_allocator_type = typename decltype(pool)::allocator_type;
        using value_type = typename decltype(pool)::value_type;

        typename std::decay_t<decltype(pool)>::base_type &base = pool;
        constexpr auto packed_page_size = decltype(pool)::traits_type::page_size;
        constexpr auto sparse_page_size = std::remove_reference_t<decltype(base)>::traits_type::page_size;

        pool_allocator_type::trigger_on_allocate = true;

        ASSERT_THROW(pool.reserve(1u), typename pool_allocator_type::exception_type);
        ASSERT_EQ(pool.capacity(), 0u);

        pool_allocator_type::trigger_after_allocate = true;

        ASSERT_THROW(pool.reserve(2 * packed_page_size), typename pool_allocator_type::exception_type);
        ASSERT_EQ(pool.capacity(), packed_page_size);

        pool.shrink_to_fit();

        ASSERT_EQ(pool.capacity(), 0u);

        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

        ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_FALSE(pool.contains(entt::entity{0}));
        ASSERT_TRUE(pool.empty());

        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

        ASSERT_THROW(base.push(entt::entity{0}), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_FALSE(base.contains(entt::entity{0}));
        ASSERT_TRUE(base.empty());

        pool_allocator_type::trigger_on_allocate = true;

        ASSERT_THROW(pool.emplace(entt::entity{0}, 0), typename pool_allocator_type::exception_type);
        ASSERT_FALSE(pool.contains(entt::entity{0}));
        ASSERT_NO_FATAL_FAILURE(pool.compact());
        ASSERT_TRUE(pool.empty());

        pool.emplace(entt::entity{0}, 0);
        const entt::entity entity[2u]{entt::entity{1}, entt::entity{sparse_page_size}};
        test::throwing_allocator<entt::entity>::trigger_after_allocate = true;

        ASSERT_THROW(pool.insert(std::begin(entity), std::end(entity), value_type{0}), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_TRUE(pool.contains(entt::entity{1}));
        ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));

        pool.erase(entt::entity{1});
        const value_type components[2u]{value_type{1}, value_type{sparse_page_size}};
        test::throwing_allocator<entt::entity>::trigger_on_allocate = true;
        pool.compact();

        ASSERT_THROW(pool.insert(std::begin(entity), std::end(entity), std::begin(components)), test::throwing_allocator<entt::entity>::exception_type);
        ASSERT_TRUE(pool.contains(entt::entity{1}));
        ASSERT_FALSE(pool.contains(entt::entity{sparse_page_size}));
    };

    test(entt::basic_storage<int, entt::entity, test::throwing_allocator<int>>{});
    test(entt::basic_storage<stable_type, entt::entity, test::throwing_allocator<stable_type>>{});
}

TEST_F(StorageOld, ThrowingComponent) {
    entt::storage<test::throwing_type> pool;
    test::throwing_type::trigger_on_value = 42;

    // strong exception safety
    ASSERT_THROW(pool.emplace(entt::entity{0}, test::throwing_type{42}), typename test::throwing_type::exception_type);
    ASSERT_TRUE(pool.empty());

    const entt::entity entity[2u]{entt::entity{42}, entt::entity{1}};
    const test::throwing_type components[2u]{42, 1};

    // basic exception safety
    ASSERT_THROW(pool.insert(std::begin(entity), std::end(entity), test::throwing_type{42}), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entt::entity{1}));

    // basic exception safety
    ASSERT_THROW(pool.insert(std::begin(entity), std::end(entity), std::begin(components)), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entt::entity{1}));

    // basic exception safety
    ASSERT_THROW(pool.insert(std::rbegin(entity), std::rend(entity), std::rbegin(components)), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.get(entt::entity{1}), 1);

    pool.clear();
    pool.emplace(entt::entity{1}, 1);
    pool.emplace(entt::entity{42}, 42);

    // basic exception safety
    ASSERT_THROW(pool.erase(entt::entity{1}), typename test::throwing_type::exception_type);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entt::entity{42}));
    ASSERT_TRUE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.at(0u), entt::entity{1});
    ASSERT_EQ(pool.at(1u), entt::entity{42});
    ASSERT_EQ(pool.get(entt::entity{42}), 42);
    // the element may have been moved but it's still there
    ASSERT_EQ(pool.get(entt::entity{1}), test::throwing_type::moved_from_value);

    test::throwing_type::trigger_on_value = 99;
    pool.erase(entt::entity{1});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{42}));
    ASSERT_FALSE(pool.contains(entt::entity{1}));
    ASSERT_EQ(pool.at(0u), entt::entity{42});
    ASSERT_EQ(pool.get(entt::entity{42}), 42);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TEST_F(StorageOld, NoUsesAllocatorConstruction) {
    test::tracked_memory_resource memory_resource{};
    entt::basic_storage<int, entt::entity, std::pmr::polymorphic_allocator<int>> pool{&memory_resource};
    const entt::entity entity{};

    pool.emplace(entity);
    pool.erase(entity);
    memory_resource.reset();
    pool.emplace(entity, 0);

    ASSERT_TRUE(pool.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_EQ(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST_F(StorageOld, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    entt::basic_storage<string_type, entt::entity, std::pmr::polymorphic_allocator<string_type>> pool{&memory_resource};
    const entt::entity entity{};

    pool.emplace(entity);
    pool.erase(entity);
    memory_resource.reset();
    pool.emplace(entity, test::tracked_memory_resource::default_value);

    ASSERT_TRUE(pool.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif

TEST_F(StorageOld, StorageType) {
    // just a bunch of static asserts to avoid regressions
    testing::StaticAssertTypeEq<entt::storage_type_t<char, entt::entity>, entt::sigh_mixin<entt::basic_storage<char, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_type_t<int>, entt::sigh_mixin<entt::storage<int>>>();
}

TEST_F(StorageOld, StorageFor) {
    // just a bunch of static asserts to avoid regressions
    testing::StaticAssertTypeEq<entt::storage_for_t<const double, entt::entity>, const entt::sigh_mixin<entt::basic_storage<double, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<char, entt::entity>, entt::sigh_mixin<entt::basic_storage<char, entt::entity>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<const bool>, const entt::sigh_mixin<entt::storage<bool>>>();
    testing::StaticAssertTypeEq<entt::storage_for_t<int>, entt::sigh_mixin<entt::storage<int>>>();
}

TEST(StorageEntity, TypeAndPolicy) {
    entt::storage<entt::entity> pool;

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_only);
}

TEST(StorageEntity, Functionalities) {
    entt::entity entity[2u]{entt::entity{0}, entt::entity{1}};
    entt::storage<entt::entity> pool;

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);

    ASSERT_EQ(*pool.push(entt::null), entity[0u]);
    ASSERT_EQ(*pool.push(entt::tombstone), entity[1u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.in_use(1u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    ASSERT_NO_THROW(pool.get(entity[0u]));
    ASSERT_EQ(pool.get_as_tuple(entity[0u]), std::tuple<>{});

    pool.erase(entity[0u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, Get) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{99});

    ASSERT_DEATH(pool.get(entt::entity{3}), "");
    ASSERT_DEATH([[maybe_unused]] auto tup = pool.get_as_tuple(entt::entity{3}), "");

    ASSERT_NO_THROW(pool.get(entt::entity{99}));
    ASSERT_NO_THROW([[maybe_unused]] auto tup = pool.get_as_tuple(entt::entity{99}));

    pool.erase(entt::entity{99});

    ASSERT_DEATH(pool.get(entt::entity{99}), "");
    ASSERT_DEATH([[maybe_unused]] auto tup = pool.get_as_tuple(entt::entity{99}), "");
}

TEST(StorageEntity, Move) {
    entt::storage<entt::entity> pool;

    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    ASSERT_TRUE(std::is_move_constructible_v<decltype(pool)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(pool)>);

    entt::storage<entt::entity> other{std::move(pool)};

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 1u);
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{1});

    pool = std::move(other);

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 0u);
    ASSERT_EQ(pool.at(0u), entt::entity{1});
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));

    other = entt::storage<entt::entity>{};

    other.push(entt::entity{3});
    other = std::move(pool);

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 1u);
    ASSERT_EQ(pool.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{1});

    other.clear();

    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(other.in_use(), 0u);

    ASSERT_EQ(*other.push(entt::null), entt::entity{0});
}

TEST(StorageEntity, Swap) {
    entt::storage<entt::entity> pool;
    entt::storage<entt::entity> other;

    pool.push(entt::entity{1});

    other.push(entt::entity{2});
    other.push(entt::entity{0});
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 3u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 1u);

    pool.swap(other);

    ASSERT_EQ(pool.size(), 3u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_EQ(other.in_use(), 1u);

    ASSERT_EQ(pool.at(0u), entt::entity{0});
    ASSERT_EQ(other.at(0u), entt::entity{1});

    pool.clear();
    other.clear();

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 0u);

    ASSERT_EQ(*other.push(entt::null), entt::entity{0});
}

TEST(StorageEntity, Push) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;

    ASSERT_EQ(*pool.push(entt::null), entt::entity{0});
    ASSERT_EQ(*pool.push(entt::tombstone), entt::entity{1});
    ASSERT_EQ(*pool.push(entt::entity{0}), entt::entity{2});
    ASSERT_EQ(*pool.push(traits_type::construct(1, 1)), entt::entity{3});
    ASSERT_EQ(*pool.push(traits_type::construct(5, 3)), traits_type::construct(5, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{1}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{2}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_GE(pool.index(entt::entity{4}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(5, 3)), pool.in_use());

    ASSERT_EQ(*pool.push(traits_type::construct(4, 42)), traits_type::construct(4, 42));
    ASSERT_EQ(*pool.push(traits_type::construct(4, 43)), entt::entity{6});

    entt::entity entity[2u]{entt::entity{1}, traits_type::construct(5, 3)};

    pool.erase(entity, entity + 2u);
    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.current(entity[0u]), 1);
    ASSERT_EQ(pool.current(entity[1u]), 4);
    ASSERT_EQ(pool.current(entt::entity{2}), 1);

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(1, 1)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(2, 1)), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(4, 42)), pool.in_use());
    ASSERT_GE(pool.index(traits_type::construct(5, 4)), pool.in_use());

    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(2, 1));
    ASSERT_EQ(*pool.push(traits_type::construct(1, 3)), traits_type::construct(1, 3));
    ASSERT_EQ(*pool.push(entt::null), traits_type::construct(5, 4));
    ASSERT_EQ(*pool.push(entt::null), entt::entity{7});
}

TEST(StorageEntity, Emplace) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::storage<entt::entity> pool;
    entt::entity entity[2u]{};

    ASSERT_EQ(pool.emplace(), entt::entity{0});
    ASSERT_EQ(pool.emplace(entt::null), entt::entity{1});
    ASSERT_EQ(pool.emplace(entt::tombstone), entt::entity{2});
    ASSERT_EQ(pool.emplace(entt::entity{0}), entt::entity{3});
    ASSERT_EQ(pool.emplace(traits_type::construct(1, 1)), entt::entity{4});
    ASSERT_EQ(pool.emplace(traits_type::construct(6, 3)), traits_type::construct(6, 3));

    ASSERT_LT(pool.index(entt::entity{0}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{1}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{2}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{3}), pool.in_use());
    ASSERT_LT(pool.index(entt::entity{4}), pool.in_use());
    ASSERT_GE(pool.index(entt::entity{5}), pool.in_use());
    ASSERT_LT(pool.index(traits_type::construct(6, 3)), pool.in_use());

    ASSERT_EQ(pool.emplace(traits_type::construct(5, 42)), traits_type::construct(5, 42));
    ASSERT_EQ(pool.emplace(traits_type::construct(5, 43)), entt::entity{7});

    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.emplace(), traits_type::construct(2, 1));

    pool.erase(traits_type::construct(2, 1));
    pool.insert(entity, entity + 2u);

    ASSERT_EQ(entity[0u], traits_type::construct(2, 2));
    ASSERT_EQ(entity[1u], entt::entity{8});
}

TEST(StorageEntity, Patch) {
    entt::storage<entt::entity> pool;
    const auto entity = pool.emplace();

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

TEST(StorageEntity, Insert) {
    entt::storage<entt::entity> pool;
    entt::entity entity[2u]{};

    pool.insert(std::begin(entity), std::end(entity));

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.erase(std::begin(entity), std::end(entity));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);

    pool.insert(entity, entity + 1u);

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
}

TEST(StorageEntity, Pack) {
    entt::storage<entt::entity> pool;
    entt::entity entity[3u]{entt::entity{1}, entt::entity{3}, entt::entity{42}};

    pool.push(entity, entity + 3u);
    std::swap(entity[0u], entity[1u]);

    const auto len = pool.pack(entity + 1u, entity + 3u);
    auto it = pool.each().cbegin().base();

    ASSERT_NE(it, pool.cbegin());
    ASSERT_NE(it, pool.cend());

    ASSERT_EQ(len, 2u);
    ASSERT_NE(it + len, pool.cend());
    ASSERT_EQ(it + len + 1u, pool.cend());

    ASSERT_EQ(*it++, entity[1u]);
    ASSERT_EQ(*it++, entity[2u]);

    ASSERT_NE(it, pool.cend());
    ASSERT_EQ(*it++, entity[0u]);
    ASSERT_EQ(it, pool.cend());
}

TEST(StorageEntity, Iterable) {
    using iterator = typename entt::storage<entt::entity>::iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

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
    ASSERT_EQ(begin.base(), pool.end() - pool.in_use());
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{42});

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

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

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
    ASSERT_EQ(begin.base(), pool.end() - pool.in_use());
    ASSERT_EQ(end.base(), pool.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{42});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{42});

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
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::iterable::iterator it = pool.each().begin();
    typename entt::storage<entt::entity>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, IterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST(StorageEntity, ReverseIterable) {
    using iterator = typename entt::storage<entt::entity>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

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
    ASSERT_EQ(end.base(), pool.rbegin() + pool.in_use());
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

    testing::StaticAssertTypeEq<iterator::value_type, std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<entt::entity> pool;

    pool.emplace(entt::entity{1});
    pool.emplace(entt::entity{3});
    pool.emplace(entt::entity{42});

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
    ASSERT_EQ(end.base(), pool.rbegin() + pool.in_use());
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
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<entt::entity>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(StorageEntity, ReverseIterableAlgorithmCompatibility) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TEST(StorageEntity, SwapElements) {
    entt::storage<entt::entity> pool;

    pool.push(entt::entity{0});
    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);
    ASSERT_TRUE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.contains(entt::entity{1}));

    ASSERT_EQ(*pool.begin(), entt::entity{1});
    ASSERT_EQ(*++pool.begin(), entt::entity{0});

    pool.swap_elements(entt::entity{0}, entt::entity{1});

    ASSERT_EQ(*pool.begin(), entt::entity{0});
    ASSERT_EQ(*++pool.begin(), entt::entity{1});
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, SwapElements) {
    entt::storage<entt::entity> pool;

    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
    ASSERT_TRUE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.contains(entt::entity{1}));

    ASSERT_DEATH(pool.swap_elements(entt::entity{0}, entt::entity{1}), "");
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, InUse) {
    entt::storage<entt::entity> pool;

    pool.push(entt::entity{0});
    pool.push(entt::entity{1});

    ASSERT_DEATH(pool.in_use(3u), "");
}

ENTT_DEBUG_TEST(StorageEntityDeathTest, SortAndRespect) {
    entt::storage<entt::entity> pool;
    entt::storage<entt::entity> other;

    pool.push(entt::entity{1});
    pool.push(entt::entity{2});
    pool.erase(entt::entity{2});

    other.push(entt::entity{2});

    ASSERT_DEATH(pool.sort([](auto...) { return true; }), "");
    ASSERT_DEATH(pool.sort_as(other), "");
}

TEST(StorageEntity, CustomAllocator) {
    test::throwing_allocator<entt::entity> allocator{};
    entt::basic_storage<entt::entity, entt::entity, test::throwing_allocator<entt::entity>> pool{allocator};

    pool.reserve(1u);

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);

    pool.push(entt::entity{0});
    pool.push(entt::entity{1});

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    decltype(pool) other{std::move(pool), allocator};

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(other.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);
    ASSERT_EQ(other.in_use(), 2u);

    pool = std::move(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(pool.in_use(), 2u);
    ASSERT_EQ(other.in_use(), 0u);

    pool.swap(other);
    pool = std::move(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(pool.in_use(), 2u);
    ASSERT_EQ(other.in_use(), 0u);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);
}
