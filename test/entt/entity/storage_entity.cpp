#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/storage.hpp>
#include "../common/config.h"
#include "../common/throwing_allocator.hpp"

TEST(StorageEntity, TypeAndPolicy) {
    entt::storage<entt::entity> pool;

    ASSERT_EQ(pool.type(), entt::type_id<entt::entity>());
    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);
}

TEST(StorageEntity, Functionalities) {
    entt::entity entities[2u]{entt::entity{0}, entt::entity{1}};
    entt::storage<entt::entity> pool;

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.in_use(), 0u);

    ASSERT_EQ(*pool.push(entt::null), entities[0u]);
    ASSERT_EQ(*pool.push(entt::tombstone), entities[1u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.in_use(1u);

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);

    ASSERT_NO_THROW(pool.get(entities[0u]));
    ASSERT_EQ(pool.get_as_tuple(entities[0u]), std::tuple<>{});

    pool.erase(entities[0u]);

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

    entt::entity entities[2u]{entt::entity{1}, traits_type::construct(5, 3)};

    pool.erase(entities, entities + 2u);
    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.current(entities[0u]), 1);
    ASSERT_EQ(pool.current(entities[1u]), 4);
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
    entt::entity entities[2u]{};

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
    pool.insert(entities, entities + 2u);

    ASSERT_EQ(entities[0u], traits_type::construct(2, 2));
    ASSERT_EQ(entities[1u], entt::entity{8});
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
    entt::entity entities[2u]{};

    pool.insert(std::begin(entities), std::end(entities));

    ASSERT_TRUE(pool.contains(entities[0u]));
    ASSERT_TRUE(pool.contains(entities[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 2u);

    pool.erase(std::begin(entities), std::end(entities));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 0u);

    pool.insert(entities, entities + 1u);

    ASSERT_TRUE(pool.contains(entities[0u]));
    ASSERT_FALSE(pool.contains(entities[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.in_use(), 1u);
}

TEST(StorageEntity, Pack) {
    entt::storage<entt::entity> pool;
    entt::entity entities[3u]{entt::entity{1}, entt::entity{3}, entt::entity{42}};

    pool.push(entities, entities + 3u);
    std::swap(entities[0u], entities[1u]);

    const auto len = pool.pack(entities + 1u, entities + 3u);
    auto it = pool.each().cbegin().base();

    ASSERT_NE(it, pool.cbegin());
    ASSERT_NE(it, pool.cend());

    ASSERT_EQ(len, 2u);
    ASSERT_NE(it + len, pool.cend());
    ASSERT_EQ(it + len + 1u, pool.cend());

    ASSERT_EQ(*it++, entities[1u]);
    ASSERT_EQ(*it++, entities[2u]);

    ASSERT_NE(it, pool.cend());
    ASSERT_EQ(*it++, entities[0u]);
    ASSERT_EQ(it, pool.cend());
}

TEST(StorageEntity, Iterable) {
    using iterator = typename entt::storage<entt::entity>::iterable::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::tuple<entt::entity>>);
    static_assert(std::is_same_v<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>);
    static_assert(std::is_same_v<typename iterator::reference, typename iterator::value_type>);

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
        static_assert(std::is_same_v<decltype(entity), entt::entity>);
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_iterable::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::tuple<entt::entity>>);
    static_assert(std::is_same_v<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>);
    static_assert(std::is_same_v<typename iterator::reference, typename iterator::value_type>);

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
        static_assert(std::is_same_v<decltype(entity), entt::entity>);
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, IterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::iterable::iterator it = pool.each().begin();
    typename entt::storage<entt::entity>::const_iterable::iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::tuple<entt::entity>>);
    static_assert(std::is_same_v<decltype(*cit), std::tuple<entt::entity>>);

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

    static_assert(std::is_same_v<iterator::value_type, std::tuple<entt::entity>>);
    static_assert(std::is_same_v<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>);
    static_assert(std::is_same_v<typename iterator::reference, typename iterator::value_type>);

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
        static_assert(std::is_same_v<decltype(entity), entt::entity>);
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseConstIterable) {
    using iterator = typename entt::storage<entt::entity>::const_reverse_iterable::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::tuple<entt::entity>>);
    static_assert(std::is_same_v<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity>>>);
    static_assert(std::is_same_v<typename iterator::reference, typename iterator::value_type>);

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
        static_assert(std::is_same_v<decltype(entity), entt::entity>);
        ASSERT_TRUE(entity != entt::entity{3});
    }
}

TEST(StorageEntity, ReverseIterableIteratorConversion) {
    entt::storage<entt::entity> pool;
    pool.emplace(entt::entity{3});

    typename entt::storage<entt::entity>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<entt::entity>::const_reverse_iterable::iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::tuple<entt::entity>>);
    static_assert(std::is_same_v<decltype(*cit), std::tuple<entt::entity>>);

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
