#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/container/dense_map.hpp>
#include <entt/core/iterator.hpp>
#include <entt/core/memory.hpp>
#include <entt/core/utility.hpp>
#include "../common/config.h"
#include "../common/throwing_allocator.hpp"
#include "../common/tracked_memory_resource.hpp"

struct transparent_equal_to {
    using is_transparent = void;

    template<typename Type, typename Other>
    constexpr std::enable_if_t<std::is_convertible_v<Other, Type>, bool>
    operator()(const Type &lhs, const Other &rhs) const {
        return lhs == static_cast<Type>(rhs);
    }
};

TEST(DenseMap, Functionalities) {
    entt::dense_map<int, int, entt::identity, transparent_equal_to> map;
    const auto &cmap = map;

    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = map.get_allocator());

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.load_factor(), 0.f);
    ASSERT_EQ(map.max_load_factor(), .875f);
    ASSERT_EQ(map.max_size(), (std::vector<entt::internal::dense_map_node<int, int>>{}.max_size()));

    map.max_load_factor(.9f);

    ASSERT_EQ(map.max_load_factor(), .9f);

    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(cmap.begin(), cmap.end());
    ASSERT_EQ(map.cbegin(), map.cend());

    ASSERT_NE(map.max_bucket_count(), 0u);
    ASSERT_EQ(map.bucket_count(), 8u);
    ASSERT_EQ(map.bucket_size(3u), 0u);

    ASSERT_EQ(map.bucket(0), 0u);
    ASSERT_EQ(map.bucket(3), 3u);
    ASSERT_EQ(map.bucket(8), 0u);
    ASSERT_EQ(map.bucket(10), 2u);

    ASSERT_EQ(map.begin(1u), map.end(1u));
    ASSERT_EQ(cmap.begin(1u), cmap.end(1u));
    ASSERT_EQ(map.cbegin(1u), map.cend(1u));

    ASSERT_FALSE(map.contains(42));
    ASSERT_FALSE(map.contains(4.2));

    ASSERT_EQ(map.find(42), map.end());
    ASSERT_EQ(map.find(4.2), map.end());
    ASSERT_EQ(cmap.find(42), map.cend());
    ASSERT_EQ(cmap.find(4.2), map.cend());

    ASSERT_EQ(map.hash_function()(42), 42);
    ASSERT_TRUE(map.key_eq()(42, 42));

    map.emplace(0, 0);

    ASSERT_EQ(map.count(0), 1u);
    ASSERT_EQ(map.count(4.2), 0u);
    ASSERT_EQ(cmap.count(0.0), 1u);
    ASSERT_EQ(cmap.count(42), 0u);

    ASSERT_FALSE(map.empty());
    ASSERT_EQ(map.size(), 1u);

    ASSERT_NE(map.begin(), map.end());
    ASSERT_NE(cmap.begin(), cmap.end());
    ASSERT_NE(map.cbegin(), map.cend());

    ASSERT_TRUE(map.contains(0));
    ASSERT_EQ(map.bucket(0), 0u);

    map.clear();

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);

    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(cmap.begin(), cmap.end());
    ASSERT_EQ(map.cbegin(), map.cend());

    ASSERT_FALSE(map.contains(0));
}

TEST(DenseMap, Constructors) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<int, int> map;

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    map = entt::dense_map<int, int>{std::allocator<int>{}};
    map = entt::dense_map<int, int>{2u * minimum_bucket_count, std::allocator<float>{}};
    map = entt::dense_map<int, int>{4u * minimum_bucket_count, std::hash<int>(), std::allocator<double>{}};

    map.emplace(3u, 42u);

    entt::dense_map<int, int> temp{map, map.get_allocator()};
    entt::dense_map<int, int> other{std::move(temp), map.get_allocator()};

    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(map.bucket_count(), 4u * minimum_bucket_count);
    ASSERT_EQ(other.bucket_count(), 4u * minimum_bucket_count);
}

TEST(DenseMap, Copy) {
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;
    map.max_load_factor(map.max_load_factor() - .05f);
    map.emplace(3u, 42u);

    entt::dense_map<std::size_t, std::size_t, entt::identity> other{map};

    ASSERT_TRUE(map.contains(3u));
    ASSERT_TRUE(other.contains(3u));
    ASSERT_EQ(map.max_load_factor(), other.max_load_factor());

    map.emplace(1u, 99u);
    map.emplace(11u, 77u);
    other.emplace(0u, 0u);
    other = map;

    ASSERT_TRUE(other.contains(3u));
    ASSERT_TRUE(other.contains(1u));
    ASSERT_TRUE(other.contains(11u));
    ASSERT_FALSE(other.contains(0u));

    ASSERT_EQ(other[3u], 42u);
    ASSERT_EQ(other[1u], 99u);
    ASSERT_EQ(other[11u], 77u);

    ASSERT_EQ(other.bucket(3u), map.bucket(11u));
    ASSERT_EQ(other.bucket(3u), other.bucket(11u));
    ASSERT_EQ(*other.begin(3u), *map.begin(3u));
    ASSERT_EQ(other.begin(3u)->first, 11u);
    ASSERT_EQ((++other.begin(3u))->first, 3u);
}

TEST(DenseMap, Move) {
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;
    map.max_load_factor(map.max_load_factor() - .05f);
    map.emplace(3u, 42u);

    entt::dense_map<std::size_t, std::size_t, entt::identity> other{std::move(map)};

    ASSERT_EQ(map.size(), 0u);
    ASSERT_TRUE(other.contains(3u));
    ASSERT_EQ(map.max_load_factor(), other.max_load_factor());

    map = other;
    map.emplace(1u, 99u);
    map.emplace(11u, 77u);
    other.emplace(0u, 0u);
    other = std::move(map);

    ASSERT_EQ(map.size(), 0u);
    ASSERT_TRUE(other.contains(3u));
    ASSERT_TRUE(other.contains(1u));
    ASSERT_TRUE(other.contains(11u));
    ASSERT_FALSE(other.contains(0u));

    ASSERT_EQ(other[3u], 42u);
    ASSERT_EQ(other[1u], 99u);
    ASSERT_EQ(other[11u], 77u);

    ASSERT_EQ(other.bucket(3u), other.bucket(11u));
    ASSERT_EQ(other.begin(3u)->first, 11u);
    ASSERT_EQ((++other.begin(3u))->first, 3u);
}

TEST(DenseMap, Iterator) {
    using iterator = typename entt::dense_map<int, int>::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const int &, int &>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<const int &, int &>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const int &, int &>>);

    entt::dense_map<int, int> map;
    map.emplace(3, 42);

    iterator end{map.begin()};
    iterator begin{};
    begin = map.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, map.begin());
    ASSERT_EQ(end, map.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, map.begin());
    ASSERT_EQ(begin--, map.end());

    ASSERT_EQ(begin + 1, map.end());
    ASSERT_EQ(end - 1, map.begin());

    ASSERT_EQ(++begin, map.end());
    ASSERT_EQ(--begin, map.begin());

    ASSERT_EQ(begin += 1, map.end());
    ASSERT_EQ(begin -= 1, map.begin());

    ASSERT_EQ(begin + (end - begin), map.end());
    ASSERT_EQ(begin - (begin - end), map.end());

    ASSERT_EQ(end - (end - begin), map.begin());
    ASSERT_EQ(end + (begin - end), map.begin());

    ASSERT_EQ(begin[0u].first, map.begin()->first);
    ASSERT_EQ(begin[0u].second, (*map.begin()).second);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, map.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, map.end());

    map.emplace(42, 3);
    begin = map.begin();

    ASSERT_EQ(begin[0u].first, 3);
    ASSERT_EQ(begin[1u].second, 3);
}

TEST(DenseMap, ConstIterator) {
    using iterator = typename entt::dense_map<int, int>::const_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const int &, const int &>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<const int &, const int &>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const int &, const int &>>);

    entt::dense_map<int, int> map;
    map.emplace(3, 42);

    iterator cend{map.cbegin()};
    iterator cbegin{};
    cbegin = map.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, map.cbegin());
    ASSERT_EQ(cend, map.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, map.cbegin());
    ASSERT_EQ(cbegin--, map.cend());

    ASSERT_EQ(cbegin + 1, map.cend());
    ASSERT_EQ(cend - 1, map.cbegin());

    ASSERT_EQ(++cbegin, map.cend());
    ASSERT_EQ(--cbegin, map.cbegin());

    ASSERT_EQ(cbegin += 1, map.cend());
    ASSERT_EQ(cbegin -= 1, map.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), map.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), map.cend());

    ASSERT_EQ(cend - (cend - cbegin), map.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), map.cbegin());

    ASSERT_EQ(cbegin[0u].first, map.cbegin()->first);
    ASSERT_EQ(cbegin[0u].second, (*map.cbegin()).second);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, map.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, map.cend());

    map.emplace(42, 3);
    cbegin = map.cbegin();

    ASSERT_EQ(cbegin[0u].first, 3);
    ASSERT_EQ(cbegin[1u].second, 3);
}

TEST(DenseMap, IteratorConversion) {
    entt::dense_map<int, int> map;
    map.emplace(3, 42);

    typename entt::dense_map<int, int>::iterator it = map.begin();
    typename entt::dense_map<int, int>::const_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::pair<const int &, int &>>);
    static_assert(std::is_same_v<decltype(*cit), std::pair<const int &, const int &>>);

    ASSERT_EQ(it->first, 3);
    ASSERT_EQ((*it).second, 42);
    ASSERT_EQ(it->first, cit->first);
    ASSERT_EQ((*it).second, (*cit).second);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseMap, Insert) {
    entt::dense_map<int, int> map;
    typename entt::dense_map<int, int>::iterator it;
    bool result;

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.find(0), map.end());
    ASSERT_FALSE(map.contains(0));

    std::pair<const int, int> value{1, 2};
    std::tie(it, result) = map.insert(std::as_const(value));

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(1));
    ASSERT_NE(map.find(1), map.end());
    ASSERT_EQ(it->first, 1);
    ASSERT_EQ(it->second, 2);

    value.second = 99;
    std::tie(it, result) = map.insert(value);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 2);

    std::tie(it, result) = map.insert(std::pair<const int, int>{3, 4});

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(3));
    ASSERT_NE(map.find(3), map.end());
    ASSERT_EQ(it->first, 3);
    ASSERT_EQ(it->second, 4);

    std::tie(it, result) = map.insert(std::pair<const int, int>{3, 99});

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 4);

    std::tie(it, result) = map.insert(std::pair<int, unsigned int>{5, 6u});

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(5));
    ASSERT_NE(map.find(5), map.end());
    ASSERT_EQ(it->first, 5);
    ASSERT_EQ(it->second, 6);

    std::tie(it, result) = map.insert(std::pair<int, unsigned int>{5, 99u});

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 6);

    std::pair<const int, int> range[2u]{std::make_pair(7, 8), std::make_pair(9, 10)};
    map.insert(std::begin(range), std::end(range));

    ASSERT_EQ(map.size(), 5u);
    ASSERT_TRUE(map.contains(7));
    ASSERT_NE(map.find(9), map.end());

    range[0u].second = 99;
    range[1u].second = 99;
    map.insert(std::begin(range), std::end(range));

    ASSERT_EQ(map.size(), 5u);
    ASSERT_EQ(map.find(7)->second, 8);
    ASSERT_EQ(map.find(9)->second, 10);
}

TEST(DenseMap, InsertRehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.insert(std::make_pair(next, next)).second);
    }

    ASSERT_EQ(map.size(), minimum_bucket_count);
    ASSERT_GT(map.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_FALSE(map.contains(minimum_bucket_count));

    ASSERT_TRUE(map.insert(std::make_pair(minimum_bucket_count, minimum_bucket_count)).second);

    ASSERT_EQ(map.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count * 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count));

    for(std::size_t next{}; next <= minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseMap, InsertSameBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_EQ(map.cbegin(next), map.cend(next));
    }

    ASSERT_TRUE(map.insert(std::make_pair(1u, 1u)).second);
    ASSERT_TRUE(map.insert(std::make_pair(9u, 9u)).second);

    ASSERT_EQ(map.size(), 2u);
    ASSERT_TRUE(map.contains(1u));
    ASSERT_NE(map.find(9u), map.end());
    ASSERT_EQ(map.bucket(1u), 1u);
    ASSERT_EQ(map.bucket(9u), 1u);
    ASSERT_EQ(map.bucket_size(1u), 2u);
    ASSERT_EQ(map.cbegin(6u), map.cend(6u));
}

TEST(DenseMap, InsertOrAssign) {
    entt::dense_map<int, int> map;
    typename entt::dense_map<int, int>::iterator it;
    bool result;

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.find(0), map.end());
    ASSERT_FALSE(map.contains(0));

    const auto key = 1;
    std::tie(it, result) = map.insert_or_assign(key, 2);

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(1));
    ASSERT_NE(map.find(1), map.end());
    ASSERT_EQ(it->first, 1);
    ASSERT_EQ(it->second, 2);

    std::tie(it, result) = map.insert_or_assign(key, 99);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 99);

    std::tie(it, result) = map.insert_or_assign(3, 4);

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(3));
    ASSERT_NE(map.find(3), map.end());
    ASSERT_EQ(it->first, 3);
    ASSERT_EQ(it->second, 4);

    std::tie(it, result) = map.insert_or_assign(3, 99);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 99);

    std::tie(it, result) = map.insert_or_assign(5, 6u);

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(5));
    ASSERT_NE(map.find(5), map.end());
    ASSERT_EQ(it->first, 5);
    ASSERT_EQ(it->second, 6);

    std::tie(it, result) = map.insert_or_assign(5, 99u);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 99);
}

TEST(DenseMap, Emplace) {
    entt::dense_map<int, int> map;
    typename entt::dense_map<int, int>::iterator it;
    bool result;

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.find(0), map.end());
    ASSERT_FALSE(map.contains(0));

    std::tie(it, result) = map.emplace();

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(0));
    ASSERT_NE(map.find(0), map.end());
    ASSERT_EQ(it->first, 0);
    ASSERT_EQ(it->second, 0);

    std::tie(it, result) = map.emplace();

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 0);

    std::tie(it, result) = map.emplace(std::make_pair(1, 2));

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(1));
    ASSERT_NE(map.find(1), map.end());
    ASSERT_EQ(it->first, 1);
    ASSERT_EQ(it->second, 2);

    std::tie(it, result) = map.emplace(std::make_pair(1, 99));

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 2u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 2);

    std::tie(it, result) = map.emplace(3, 4);

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(3));
    ASSERT_NE(map.find(3), map.end());
    ASSERT_EQ(it->first, 3);
    ASSERT_EQ(it->second, 4);

    std::tie(it, result) = map.emplace(3, 99);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 4);

    std::tie(it, result) = map.emplace(std::piecewise_construct, std::make_tuple(5), std::make_tuple(6u));

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 4u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(5));
    ASSERT_NE(map.find(5), map.end());
    ASSERT_EQ(it->first, 5);
    ASSERT_EQ(it->second, 6);

    std::tie(it, result) = map.emplace(std::piecewise_construct, std::make_tuple(5), std::make_tuple(99u));

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 4u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 6);

    std::tie(it, result) = map.emplace(std::make_pair(1, 99));

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 4u);
    ASSERT_EQ(it, ++map.begin());
    ASSERT_EQ(it->second, 2);
}

TEST(DenseMap, EmplaceRehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.emplace(next, next).second);
        ASSERT_LE(map.load_factor(), map.max_load_factor());
    }

    ASSERT_EQ(map.size(), minimum_bucket_count);
    ASSERT_GT(map.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_FALSE(map.contains(minimum_bucket_count));

    ASSERT_TRUE(map.emplace(minimum_bucket_count, minimum_bucket_count).second);

    ASSERT_EQ(map.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count * 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count));

    for(std::size_t next{}; next <= minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseMap, EmplaceSameBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_EQ(map.cbegin(next), map.cend(next));
    }

    ASSERT_TRUE(map.emplace(1u, 1u).second);
    ASSERT_TRUE(map.emplace(9u, 9u).second);

    ASSERT_EQ(map.size(), 2u);
    ASSERT_TRUE(map.contains(1u));
    ASSERT_NE(map.find(9u), map.end());
    ASSERT_EQ(map.bucket(1u), 1u);
    ASSERT_EQ(map.bucket(9u), 1u);
    ASSERT_EQ(map.bucket_size(1u), 2u);
    ASSERT_EQ(map.cbegin(6u), map.cend(6u));
}

TEST(DenseMap, TryEmplace) {
    entt::dense_map<int, int> map;
    typename entt::dense_map<int, int>::iterator it;
    bool result;

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.find(1), map.end());
    ASSERT_FALSE(map.contains(1));

    std::tie(it, result) = map.try_emplace(1, 2);

    ASSERT_TRUE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_TRUE(map.contains(1));
    ASSERT_NE(map.find(1), map.end());
    ASSERT_EQ(it->first, 1);
    ASSERT_EQ(it->second, 2);

    std::tie(it, result) = map.try_emplace(1, 99);

    ASSERT_FALSE(result);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_EQ(it, --map.end());
    ASSERT_EQ(it->second, 2);
}

TEST(DenseMap, TryEmplaceRehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.try_emplace(next, next).second);
    }

    ASSERT_EQ(map.size(), minimum_bucket_count);
    ASSERT_GT(map.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_FALSE(map.contains(minimum_bucket_count));

    ASSERT_TRUE(map.try_emplace(minimum_bucket_count, minimum_bucket_count).second);

    ASSERT_EQ(map.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count * 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(map[minimum_bucket_count - 1u], minimum_bucket_count - 1u);
    ASSERT_EQ(map.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_TRUE(map.contains(minimum_bucket_count));

    for(std::size_t next{}; next <= minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseMap, TryEmplaceSameBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_EQ(map.cbegin(next), map.cend(next));
    }

    ASSERT_TRUE(map.try_emplace(1u, 1u).second);
    ASSERT_TRUE(map.try_emplace(9u, 9u).second);

    ASSERT_EQ(map.size(), 2u);
    ASSERT_TRUE(map.contains(1u));
    ASSERT_NE(map.find(9u), map.end());
    ASSERT_EQ(map.bucket(1u), 1u);
    ASSERT_EQ(map.bucket(9u), 1u);
    ASSERT_EQ(map.bucket_size(1u), 2u);
    ASSERT_EQ(map.cbegin(6u), map.cend(6u));
}

TEST(DenseMap, TryEmplaceMovableType) {
    entt::dense_map<int, std::unique_ptr<int>> map;
    std::unique_ptr<int> value = std::make_unique<int>(42);

    ASSERT_TRUE(map.try_emplace(*value, std::move(value)).second);
    ASSERT_FALSE(map.empty());
    ASSERT_FALSE(value);

    value = std::make_unique<int>(42);

    ASSERT_FALSE(map.try_emplace(*value, std::move(value)).second);
    ASSERT_TRUE(value);
}

TEST(DenseMap, Erase) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        map.emplace(next, next);
    }

    ASSERT_EQ(map.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(map.size(), minimum_bucket_count + 1u);

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        ASSERT_TRUE(map.contains(next));
    }

    auto it = map.erase(++map.begin());
    it = map.erase(it, it + 1);

    ASSERT_EQ((--map.end())->first, 6u);
    ASSERT_EQ(map.erase(6u), 1u);
    ASSERT_EQ(map.erase(6u), 0u);

    ASSERT_EQ(map.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(map.size(), minimum_bucket_count + 1u - 3u);

    ASSERT_EQ(it, ++map.begin());
    ASSERT_EQ(it->first, 7u);
    ASSERT_EQ((--map.end())->first, 5u);

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        if(next == 1u || next == 8u || next == 6u) {
            ASSERT_FALSE(map.contains(next));
            ASSERT_EQ(map.bucket_size(next), 0u);
        } else {
            ASSERT_TRUE(map.contains(next));
            ASSERT_EQ(map.bucket(next), next);
            ASSERT_EQ(map.bucket_size(next), 1u);
        }
    }

    map.erase(map.begin(), map.end());

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        ASSERT_FALSE(map.contains(next));
    }

    ASSERT_EQ(map.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(map.size(), 0u);
}

TEST(DenseMap, EraseWithMovableKeyValue) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::string, std::size_t> map;

    map.emplace("0", 0u);
    map.emplace("1", 1u);

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_EQ(map.size(), 2u);

    auto it = map.erase(map.find("0"));

    ASSERT_EQ(it->first, "1");
    ASSERT_EQ(it->second, 1u);
    ASSERT_EQ(map.size(), 1u);
    ASSERT_FALSE(map.contains("0"));
}

TEST(DenseMap, EraseFromBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_EQ(map.size(), 0u);

    for(std::size_t next{}; next < 4u; ++next) {
        ASSERT_TRUE(map.emplace(2u * minimum_bucket_count * next, 2u * minimum_bucket_count * next).second);
        ASSERT_TRUE(map.emplace(2u * minimum_bucket_count * next + 2u, 2u * minimum_bucket_count * next + 2u).second);
        ASSERT_TRUE(map.emplace(2u * minimum_bucket_count * (next + 1u) - 1u, 2u * minimum_bucket_count * (next + 1u) - 1u).second);
    }

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(map.size(), 12u);

    ASSERT_EQ(map.bucket_size(0u), 4u);
    ASSERT_EQ(map.bucket_size(2u), 4u);
    ASSERT_EQ(map.bucket_size(15u), 4u);

    map.erase(map.end() - 3, map.end());

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(map.size(), 9u);

    ASSERT_EQ(map.bucket_size(0u), 3u);
    ASSERT_EQ(map.bucket_size(2u), 3u);
    ASSERT_EQ(map.bucket_size(15u), 3u);

    for(std::size_t next{}; next < 3u; ++next) {
        ASSERT_TRUE(map.contains(2u * minimum_bucket_count * next));
        ASSERT_EQ(map.bucket(2u * minimum_bucket_count * next), 0u);

        ASSERT_TRUE(map.contains(2u * minimum_bucket_count * next + 2u));
        ASSERT_EQ(map.bucket(2u * minimum_bucket_count * next + 2u), 2u);

        ASSERT_TRUE(map.contains(2u * minimum_bucket_count * (next + 1u) - 1u));
        ASSERT_EQ(map.bucket(2u * minimum_bucket_count * (next + 1u) - 1u), 15u);
    }

    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * 3u));
    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * 3u + 2u));
    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * (3u + 1u) - 1u));

    map.erase((++map.begin(0u))->first);
    map.erase((++map.begin(2u))->first);
    map.erase((++map.begin(15u))->first);

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(map.size(), 6u);

    ASSERT_EQ(map.bucket_size(0u), 2u);
    ASSERT_EQ(map.bucket_size(2u), 2u);
    ASSERT_EQ(map.bucket_size(15u), 2u);

    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * 1u));
    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * 1u + 2u));
    ASSERT_FALSE(map.contains(2u * minimum_bucket_count * (1u + 1u) - 1u));

    while(map.begin(15) != map.end(15u)) {
        map.erase(map.begin(15)->first);
    }

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(map.size(), 4u);

    ASSERT_EQ(map.bucket_size(0u), 2u);
    ASSERT_EQ(map.bucket_size(2u), 2u);
    ASSERT_EQ(map.bucket_size(15u), 0u);

    ASSERT_TRUE(map.contains(0u * minimum_bucket_count));
    ASSERT_TRUE(map.contains(0u * minimum_bucket_count + 2u));
    ASSERT_TRUE(map.contains(4u * minimum_bucket_count));
    ASSERT_TRUE(map.contains(4u * minimum_bucket_count + 2u));

    map.erase(4u * minimum_bucket_count + 2u);
    map.erase(0u * minimum_bucket_count);

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(map.size(), 2u);

    ASSERT_EQ(map.bucket_size(0u), 1u);
    ASSERT_EQ(map.bucket_size(2u), 1u);
    ASSERT_EQ(map.bucket_size(15u), 0u);

    ASSERT_FALSE(map.contains(0u * minimum_bucket_count));
    ASSERT_TRUE(map.contains(0u * minimum_bucket_count + 2u));
    ASSERT_TRUE(map.contains(4u * minimum_bucket_count));
    ASSERT_FALSE(map.contains(4u * minimum_bucket_count + 2u));
}

TEST(DenseMap, Swap) {
    entt::dense_map<int, int> map;
    entt::dense_map<int, int> other;

    map.emplace(0, 1);

    ASSERT_FALSE(map.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(map.contains(0));
    ASSERT_FALSE(other.contains(0));

    map.swap(other);

    ASSERT_TRUE(map.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_FALSE(map.contains(0));
    ASSERT_TRUE(other.contains(0));
}

TEST(DenseMap, EqualRange) {
    entt::dense_map<int, int, entt::identity, transparent_equal_to> map;
    const auto &cmap = map;

    map.emplace(42, 3);

    ASSERT_EQ(map.equal_range(0).first, map.end());
    ASSERT_EQ(map.equal_range(0).second, map.end());

    ASSERT_EQ(cmap.equal_range(0).first, cmap.cend());
    ASSERT_EQ(cmap.equal_range(0).second, cmap.cend());

    ASSERT_EQ(map.equal_range(0.0).first, map.end());
    ASSERT_EQ(map.equal_range(0.0).second, map.end());

    ASSERT_EQ(cmap.equal_range(0.0).first, cmap.cend());
    ASSERT_EQ(cmap.equal_range(0.0).second, cmap.cend());

    ASSERT_NE(map.equal_range(42).first, map.end());
    ASSERT_EQ(map.equal_range(42).first->first, 42);
    ASSERT_EQ(map.equal_range(42).first->second, 3);
    ASSERT_EQ(map.equal_range(42).second, map.end());

    ASSERT_NE(cmap.equal_range(42).first, cmap.cend());
    ASSERT_EQ(cmap.equal_range(42).first->first, 42);
    ASSERT_EQ(cmap.equal_range(42).first->second, 3);
    ASSERT_EQ(cmap.equal_range(42).second, cmap.cend());

    ASSERT_NE(map.equal_range(42.0).first, map.end());
    ASSERT_EQ(map.equal_range(42.0).first->first, 42);
    ASSERT_EQ(map.equal_range(42.0).first->second, 3);
    ASSERT_EQ(map.equal_range(42.0).second, map.end());

    ASSERT_NE(cmap.equal_range(42.0).first, cmap.cend());
    ASSERT_EQ(cmap.equal_range(42.0).first->first, 42);
    ASSERT_EQ(cmap.equal_range(42.0).first->second, 3);
    ASSERT_EQ(cmap.equal_range(42.0).second, cmap.cend());
}

TEST(DenseMap, Indexing) {
    entt::dense_map<int, int> map;
    const auto &cmap = map;
    const auto key = 1;

    ASSERT_FALSE(map.contains(key));

    map[key] = 99;

    ASSERT_TRUE(map.contains(key));
    ASSERT_EQ(map[std::move(key)], 99);
    ASSERT_EQ(cmap.at(key), 99);
    ASSERT_EQ(map.at(key), 99);
}

ENTT_DEBUG_TEST(DenseMapDeathTest, Indexing) {
    entt::dense_map<int, int> map;
    const auto &cmap = map;

    ASSERT_DEATH([[maybe_unused]] auto value = cmap.at(0), "");
    ASSERT_DEATH([[maybe_unused]] auto value = map.at(42), "");
}

TEST(DenseMap, LocalIterator) {
    using iterator = typename entt::dense_map<std::size_t, std::size_t, entt::identity>::local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const std::size_t &, std::size_t &>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<const std::size_t &, std::size_t &>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const std::size_t &, std::size_t &>>);

    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;
    map.emplace(3u, 42u);
    map.emplace(3u + minimum_bucket_count, 99u);

    iterator end{map.begin(3u)};
    iterator begin{};
    begin = map.end(3u);
    std::swap(begin, end);

    ASSERT_EQ(begin, map.begin(3u));
    ASSERT_EQ(end, map.end(3u));
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin->first, 3u + minimum_bucket_count);
    ASSERT_EQ((*begin).second, 99u);

    ASSERT_EQ(begin++, map.begin(3u));
    ASSERT_EQ(++begin, map.end(3u));
}

TEST(DenseMap, ConstLocalIterator) {
    using iterator = typename entt::dense_map<std::size_t, std::size_t, entt::identity>::const_local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const std::size_t &, const std::size_t &>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<const std::size_t &, const std::size_t &>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const std::size_t &, const std::size_t &>>);

    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;
    map.emplace(3u, 42u);
    map.emplace(3u + minimum_bucket_count, 99u);

    iterator cend{map.begin(3u)};
    iterator cbegin{};
    cbegin = map.end(3u);
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, map.begin(3u));
    ASSERT_EQ(cend, map.end(3u));
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin->first, 3u + minimum_bucket_count);
    ASSERT_EQ((*cbegin).second, 99u);

    ASSERT_EQ(cbegin++, map.begin(3u));
    ASSERT_EQ(++cbegin, map.end(3u));
}

TEST(DenseMap, LocalIteratorConversion) {
    entt::dense_map<int, int> map;
    map.emplace(3, 42);

    typename entt::dense_map<int, int>::local_iterator it = map.begin(map.bucket(3));
    typename entt::dense_map<int, int>::const_local_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::pair<const int &, int &>>);
    static_assert(std::is_same_v<decltype(*cit), std::pair<const int &, const int &>>);

    ASSERT_EQ(it->first, 3);
    ASSERT_EQ((*it).second, 42);
    ASSERT_EQ(it->first, cit->first);
    ASSERT_EQ((*it).second, (*cit).second);

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseMap, Rehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, entt::identity> map;
    map[32u] = 99u;

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(12u);

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(44u);

    ASSERT_EQ(map.bucket_count(), 8u * minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 32u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(0u);

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        map.emplace(next, next);
    }

    ASSERT_EQ(map.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);

    map.rehash(0u);

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));

    map.rehash(55u);

    ASSERT_EQ(map.bucket_count(), 8u * minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));

    map.rehash(2u);

    ASSERT_EQ(map.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map[next], next);
        ASSERT_EQ(map.bucket(next), next);
    }

    ASSERT_EQ(map.bucket_size(0u), 2u);
    ASSERT_EQ(map.bucket_size(3u), 1u);

    ASSERT_EQ(map.begin(0u)->first, 0u);
    ASSERT_EQ(map.begin(0u)->second, 0u);
    ASSERT_EQ((++map.begin(0u))->first, 32u);
    ASSERT_EQ((++map.begin(0u))->second, 99u);

    map.clear();
    map.rehash(2u);

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_FALSE(map.contains(32u));

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_FALSE(map.contains(next));
    }

    ASSERT_EQ(map.bucket_size(0u), 0u);
    ASSERT_EQ(map.bucket_size(3u), 0u);
}

TEST(DenseMap, Reserve) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<int, int> map;

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    map.reserve(0u);

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    map.reserve(minimum_bucket_count);

    ASSERT_EQ(map.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(map.bucket_count(), entt::next_power_of_two(std::ceil(minimum_bucket_count / map.max_load_factor())));
}

TEST(DenseMap, ThrowingAllocator) {
    using allocator = test::throwing_allocator<std::pair<const std::size_t, std::size_t>>;
    using packed_allocator = test::throwing_allocator<entt::internal::dense_map_node<std::size_t, std::size_t>>;
    using packed_exception = typename packed_allocator::exception_type;

    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_map<std::size_t, std::size_t, std::hash<std::size_t>, std::equal_to<std::size_t>, allocator> map{};

    packed_allocator::trigger_on_allocate = true;

    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);
    ASSERT_THROW(map.reserve(2u * map.bucket_count()), packed_exception);
    ASSERT_EQ(map.bucket_count(), minimum_bucket_count);

    packed_allocator::trigger_on_allocate = true;

    ASSERT_THROW(map.emplace(0u, 0u), packed_exception);
    ASSERT_FALSE(map.contains(0u));

    packed_allocator::trigger_on_allocate = true;

    ASSERT_THROW(map.emplace(std::piecewise_construct, std::make_tuple(0u), std::make_tuple(0u)), packed_exception);
    ASSERT_FALSE(map.contains(0u));

    packed_allocator::trigger_on_allocate = true;

    ASSERT_THROW(map.insert_or_assign(0u, 0u), packed_exception);
    ASSERT_FALSE(map.contains(0u));
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TEST(DenseMap, NoUsesAllocatorConstruction) {
    using allocator = std::pmr::polymorphic_allocator<std::pair<const int, int>>;

    test::tracked_memory_resource memory_resource{};
    entt::dense_map<int, int, std::hash<int>, std::equal_to<int>, allocator> map{&memory_resource};

    map.reserve(1u);
    memory_resource.reset();
    map.emplace(0, 0);

    ASSERT_TRUE(map.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_EQ(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST(DenseMap, KeyUsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;
    using allocator = std::pmr::polymorphic_allocator<std::pair<const string_type, int>>;

    test::tracked_memory_resource memory_resource{};
    entt::dense_map<string_type, int, std::hash<string_type>, std::equal_to<string_type>, allocator> map{&memory_resource};

    map.reserve(1u);
    memory_resource.reset();
    map.emplace(test::tracked_memory_resource::default_value, 0);

    ASSERT_TRUE(map.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);

    memory_resource.reset();
    decltype(map) other{map, &memory_resource};

    ASSERT_TRUE(memory_resource.is_equal(*other.get_allocator().resource()));
    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST(DenseMap, ValueUsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;
    using allocator = std::pmr::polymorphic_allocator<std::pair<const int, string_type>>;

    test::tracked_memory_resource memory_resource{};
    entt::dense_map<int, string_type, std::hash<int>, std::equal_to<int>, allocator> map{std::pmr::get_default_resource()};

    map.reserve(1u);
    memory_resource.reset();
    map.emplace(0, test::tracked_memory_resource::default_value);

    ASSERT_FALSE(map.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_EQ(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);

    decltype(map) other{std::move(map), &memory_resource};

    ASSERT_TRUE(other.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif
