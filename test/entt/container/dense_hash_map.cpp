#include <cmath>
#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/container/dense_hash_map.hpp>
#include <entt/core/memory.hpp>
#include <entt/core/utility.hpp>

struct transparent_equal_to {
    using is_transparent = void;

    template<typename Type, typename Other>
    constexpr std::enable_if_t<std::is_convertible_v<Other, Type>, bool>
    operator()(const Type &lhs, const Other &rhs) const {
        return lhs == static_cast<Type>(rhs);
    }
};

TEST(DenseHashMap, Functionalities) {
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity, transparent_equal_to> map;

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = map.get_allocator());

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.load_factor(), 0.f);
    ASSERT_EQ(map.max_load_factor(), .875f);

    map.max_load_factor(.9f);

    ASSERT_EQ(map.max_load_factor(), .9f);

    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());
    ASSERT_EQ(map.cbegin(), map.cend());

    ASSERT_NE(map.max_bucket_count(), 0u);
    ASSERT_EQ(map.bucket_count(), 8u);
    ASSERT_EQ(map.bucket_size(3u), 0u);

    ASSERT_EQ(map.bucket(0), 0u);
    ASSERT_EQ(map.bucket(3), 3u);
    ASSERT_EQ(map.bucket(8), 0u);
    ASSERT_EQ(map.bucket(10), 2u);

    ASSERT_EQ(map.begin(1u), map.end(1u));
    ASSERT_EQ(std::as_const(map).begin(1u), std::as_const(map).end(1u));
    ASSERT_EQ(map.cbegin(1u), map.cend(1u));

    ASSERT_FALSE(map.contains(42));
    ASSERT_FALSE(map.contains(4.2));

    ASSERT_EQ(map.find(42), map.end());
    ASSERT_EQ(map.find(4.2), map.end());
    ASSERT_EQ(std::as_const(map).find(42), map.cend());
    ASSERT_EQ(std::as_const(map).find(4.2), map.cend());

    ASSERT_EQ(map.hash_function()(42), 42);
    ASSERT_TRUE(map.key_eq()(42, 42));

    map.emplace(0u, 0u);

    ASSERT_FALSE(map.empty());
    ASSERT_EQ(map.size(), 1u);

    ASSERT_NE(map.begin(), map.end());
    ASSERT_NE(std::as_const(map).begin(), std::as_const(map).end());
    ASSERT_NE(map.cbegin(), map.cend());

    ASSERT_TRUE(map.contains(0u));
    ASSERT_EQ(map.bucket(0u), 0u);

    map.clear();

    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0u);

    ASSERT_EQ(map.begin(), map.end());
    ASSERT_EQ(std::as_const(map).begin(), std::as_const(map).end());
    ASSERT_EQ(map.cbegin(), map.cend());

    ASSERT_FALSE(map.contains(0u));
}

TEST(DenseHashMap, Contructors) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<int, int> map;

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    map = entt::dense_hash_map<int, int>{std::allocator<int>{}};
    map = entt::dense_hash_map<int, int>{2u * expected_bucket_count, std::allocator<float>{}};
    map = entt::dense_hash_map<int, int>{4u * expected_bucket_count, std::hash<int>(), std::allocator<double>{}};

    map.emplace(3u, 42u);

    entt::dense_hash_map<int, int> temp{map, map.get_allocator()};
    entt::dense_hash_map<int, int> other{std::move(temp), map.get_allocator()};

    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(map.bucket_count(), 4u * expected_bucket_count);
    ASSERT_EQ(other.bucket_count(), 4u * expected_bucket_count);
}

TEST(DenseHashMap, Copy) {
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;
    map.max_load_factor(map.max_load_factor() - .05f);
    map.emplace(3u, 42u);

    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> other{map};

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

TEST(DenseHashMap, Move) {
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;
    map.max_load_factor(map.max_load_factor() - .05f);
    map.emplace(3u, 42u);

    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> other{std::move(map)};

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

TEST(DenseHashMap, Iterator) {
    using iterator = typename entt::dense_hash_map<int, int>::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const int, int>>);
    static_assert(std::is_same_v<iterator::pointer, std::pair<const int, int> *>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const int, int> &>);

    entt::dense_hash_map<int, int> map;
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
}

TEST(DenseHashMap, ConstIterator) {
    using iterator = typename entt::dense_hash_map<int, int>::const_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const int, int>>);
    static_assert(std::is_same_v<iterator::pointer, const std::pair<const int, int> *>);
    static_assert(std::is_same_v<iterator::reference, const std::pair<const int, int> &>);

    entt::dense_hash_map<int, int> map;
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
    ASSERT_EQ(cbegin[0u].second, (*map.cbegin()).second);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, map.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, map.cend());
}

TEST(DenseHashMap, IteratorConversion) {
    entt::dense_hash_map<int, int> map;
    map.emplace(3, 42);

    typename entt::dense_hash_map<int, int>::iterator it = map.begin();
    typename entt::dense_hash_map<int, int>::const_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::pair<const int, int> &>);
    static_assert(std::is_same_v<decltype(*cit), const std::pair<const int, int> &>);

    ASSERT_EQ(it->first, 3);
    ASSERT_EQ((*it).second, 42);
    ASSERT_EQ(it->first, cit->first);
    ASSERT_EQ((*it).second, (*it).second);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseHashMap, Insert) {
    entt::dense_hash_map<int, int> map;
    typename entt::dense_hash_map<int, int>::iterator it;
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

TEST(DenseHashMap, InsertRehash) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
        ASSERT_TRUE(map.insert(std::make_pair(next, next)).second);
    }

    ASSERT_EQ(map.size(), expected_bucket_count);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_FALSE(map.contains(expected_bucket_count));

    ASSERT_TRUE(map.insert(std::make_pair(expected_bucket_count, expected_bucket_count)).second);

    ASSERT_EQ(map.size(), expected_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count * 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count));

    for(std::size_t next{}; next <= expected_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseHashMap, InsertSameBucket) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
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

TEST(DenseHashMap, InsertOrAssign) {
    entt::dense_hash_map<int, int> map;
    typename entt::dense_hash_map<int, int>::iterator it;
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

TEST(DenseHashMap, Emplace) {
    entt::dense_hash_map<int, int> map;
    typename entt::dense_hash_map<int, int>::iterator it;
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

TEST(DenseHashMap, EmplaceRehash) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
        ASSERT_TRUE(map.emplace(next, next).second);
    }

    ASSERT_EQ(map.size(), expected_bucket_count);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_FALSE(map.contains(expected_bucket_count));

    ASSERT_TRUE(map.emplace(expected_bucket_count, expected_bucket_count).second);

    ASSERT_EQ(map.size(), expected_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count * 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count));

    for(std::size_t next{}; next <= expected_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseHashMap, EmplaceSameBucket) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
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

TEST(DenseHashMap, TryEmplace) {
    entt::dense_hash_map<int, int> map;
    typename entt::dense_hash_map<int, int>::iterator it;
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

TEST(DenseHashMap, TryEmplaceRehash) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.size(), 0u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
        ASSERT_TRUE(map.try_emplace(next, next).second);
    }

    ASSERT_EQ(map.size(), expected_bucket_count);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_FALSE(map.contains(expected_bucket_count));

    ASSERT_TRUE(map.try_emplace(expected_bucket_count, expected_bucket_count).second);

    ASSERT_EQ(map.size(), expected_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), expected_bucket_count * 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count / 2u));
    ASSERT_EQ(map[expected_bucket_count - 1u], expected_bucket_count - 1u);
    ASSERT_EQ(map.bucket(expected_bucket_count / 2u), expected_bucket_count / 2u);
    ASSERT_TRUE(map.contains(expected_bucket_count));

    for(std::size_t next{}; next <= expected_bucket_count; ++next) {
        ASSERT_TRUE(map.contains(next));
        ASSERT_EQ(map.bucket(next), next);
        ASSERT_EQ(map[next], next);
    }
}

TEST(DenseHashMap, TryEmplaceSameBucket) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
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

TEST(DenseHashMap, Erase) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    for(std::size_t next{}, last = expected_bucket_count + 1u; next < last; ++next) {
        map.emplace(next, next);
    }

    ASSERT_EQ(map.bucket_count(), 2 * expected_bucket_count);
    ASSERT_EQ(map.size(), expected_bucket_count + 1u);

    for(std::size_t next{}, last = expected_bucket_count + 1u; next < last; ++next) {
        ASSERT_TRUE(map.contains(next));
    }

    auto it = map.erase(++map.begin());
    it = map.erase(it, it + 1);

    ASSERT_EQ((--map.end())->first, 6u);
    ASSERT_EQ(map.erase(6u), 1u);
    ASSERT_EQ(map.erase(6u), 0u);

    ASSERT_EQ(map.bucket_count(), 2 * expected_bucket_count);
    ASSERT_EQ(map.size(), expected_bucket_count + 1u - 3u);

    ASSERT_EQ(it, ++map.begin());
    ASSERT_EQ(it->first, 7u);
    ASSERT_EQ((--map.end())->first, 5u);

    for(std::size_t next{}, last = expected_bucket_count + 1u; next < last; ++next) {
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

    for(std::size_t next{}, last = expected_bucket_count + 1u; next < last; ++next) {
        ASSERT_FALSE(map.contains(next));
    }

    ASSERT_EQ(map.bucket_count(), 2 * expected_bucket_count);
    ASSERT_EQ(map.size(), 0u);
}

TEST(DenseHashMap, EraseFromBucket) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_EQ(map.size(), 0u);

    for(std::size_t next{}; next < 4u; ++next) {
        ASSERT_TRUE(map.emplace(2u * expected_bucket_count * next, 2u * 2u * expected_bucket_count * next).second);
        ASSERT_TRUE(map.emplace(2u * expected_bucket_count * next + 2u, 2u * expected_bucket_count * next + 2u).second);
        ASSERT_TRUE(map.emplace(2u * expected_bucket_count * (next + 1u) - 1u, 2u * expected_bucket_count * (next + 1u) - 1u).second);
    }

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_EQ(map.size(), 12u);

    ASSERT_EQ(map.bucket_size(0u), 4u);
    ASSERT_EQ(map.bucket_size(2u), 4u);
    ASSERT_EQ(map.bucket_size(15u), 4u);

    map.erase(map.end() - 3, map.end());

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_EQ(map.size(), 9u);

    ASSERT_EQ(map.bucket_size(0u), 3u);
    ASSERT_EQ(map.bucket_size(2u), 3u);
    ASSERT_EQ(map.bucket_size(15u), 3u);

    for(std::size_t next{}; next < 3u; ++next) {
        ASSERT_TRUE(map.contains(2u * expected_bucket_count * next));
        ASSERT_EQ(map.bucket(2u * expected_bucket_count * next), 0u);

        ASSERT_TRUE(map.contains(2u * expected_bucket_count * next + 2u));
        ASSERT_EQ(map.bucket(2u * expected_bucket_count * next + 2u), 2u);

        ASSERT_TRUE(map.contains(2u * expected_bucket_count * (next + 1u) - 1u));
        ASSERT_EQ(map.bucket(2u * expected_bucket_count * (next + 1u) - 1u), 15u);
    }

    ASSERT_FALSE(map.contains(2u * expected_bucket_count * 3u));
    ASSERT_FALSE(map.contains(2u * expected_bucket_count * 3u + 2u));
    ASSERT_FALSE(map.contains(2u * expected_bucket_count * (3u + 1u) - 1u));

    map.erase((++map.begin(0u))->first);
    map.erase((++map.begin(2u))->first);
    map.erase((++map.begin(15u))->first);

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_EQ(map.size(), 6u);

    ASSERT_EQ(map.bucket_size(0u), 2u);
    ASSERT_EQ(map.bucket_size(2u), 2u);
    ASSERT_EQ(map.bucket_size(15u), 2u);

    ASSERT_FALSE(map.contains(2u * expected_bucket_count * 1u));
    ASSERT_FALSE(map.contains(2u * expected_bucket_count * 1u + 2u));
    ASSERT_FALSE(map.contains(2u * expected_bucket_count * (1u + 1u) - 1u));

    while(map.begin(15) != map.end(15u)) {
        map.erase(map.begin(15)->first);
    }

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_EQ(map.size(), 4u);

    ASSERT_EQ(map.bucket_size(0u), 2u);
    ASSERT_EQ(map.bucket_size(2u), 2u);
    ASSERT_EQ(map.bucket_size(15u), 0u);

    ASSERT_TRUE(map.contains(0u * expected_bucket_count));
    ASSERT_TRUE(map.contains(0u * expected_bucket_count + 2u));
    ASSERT_TRUE(map.contains(4u * expected_bucket_count));
    ASSERT_TRUE(map.contains(4u * expected_bucket_count + 2u));

    map.erase(4u * expected_bucket_count + 2u);
    map.erase(0u * expected_bucket_count);

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_EQ(map.size(), 2u);

    ASSERT_EQ(map.bucket_size(0u), 1u);
    ASSERT_EQ(map.bucket_size(2u), 1u);
    ASSERT_EQ(map.bucket_size(15u), 0u);

    ASSERT_FALSE(map.contains(0u * expected_bucket_count));
    ASSERT_TRUE(map.contains(0u * expected_bucket_count + 2u));
    ASSERT_TRUE(map.contains(4u * expected_bucket_count));
    ASSERT_FALSE(map.contains(4u * expected_bucket_count + 2u));
}

TEST(DenseHashMap, Swap) {
    entt::dense_hash_map<int, int> map;
    entt::dense_hash_map<int, int> other;

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

TEST(DenseHashMap, Indexing) {
    entt::dense_hash_map<int, int> map;
    const auto key = 1;

    ASSERT_FALSE(map.contains(key));
    ASSERT_DEATH([[maybe_unused]] auto value = std::as_const(map).at(key), "");
    ASSERT_DEATH([[maybe_unused]] auto value = map.at(key), "");

    map[key] = 99;

    ASSERT_TRUE(map.contains(key));
    ASSERT_EQ(map[std::move(key)], 99);
    ASSERT_EQ(std::as_const(map).at(key), 99);
    ASSERT_EQ(map.at(key), 99);
}

TEST(DenseHashMap, LocalIterator) {
    using iterator = typename entt::dense_hash_map<std::size_t, std::size_t, entt::identity>::local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const std::size_t, std::size_t>>);
    static_assert(std::is_same_v<iterator::pointer, std::pair<const std::size_t, std::size_t> *>);
    static_assert(std::is_same_v<iterator::reference, std::pair<const std::size_t, std::size_t> &>);

    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;
    map.emplace(3u, 42u);
    map.emplace(3u + expected_bucket_count, 99u);

    iterator end{map.begin(3u)};
    iterator begin{};
    begin = map.end(3u);
    std::swap(begin, end);

    ASSERT_EQ(begin, map.begin(3u));
    ASSERT_EQ(end, map.end(3u));
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin->first, 3u + expected_bucket_count);
    ASSERT_EQ((*begin).second, 99u);

    ASSERT_EQ(begin.base(), map.begin().base() + 1u);
    ASSERT_EQ(begin++, map.begin(3u));
    ASSERT_EQ(begin.base(), map.begin().base());
    ASSERT_EQ(++begin, map.end(3u));
    ASSERT_NE(begin.base(), map.end().base());
}

TEST(DenseHashMap, ConstLocalIterator) {
    using iterator = typename entt::dense_hash_map<std::size_t, std::size_t, entt::identity>::const_local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<const std::size_t, std::size_t>>);
    static_assert(std::is_same_v<iterator::pointer, const std::pair<const std::size_t, std::size_t> *>);
    static_assert(std::is_same_v<iterator::reference, const std::pair<const std::size_t, std::size_t> &>);

    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;
    map.emplace(3u, 42u);
    map.emplace(3u + expected_bucket_count, 99u);

    iterator cend{map.begin(3u)};
    iterator cbegin{};
    cbegin = map.end(3u);
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, map.begin(3u));
    ASSERT_EQ(cend, map.end(3u));
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin->first, 3u + expected_bucket_count);
    ASSERT_EQ((*cbegin).second, 99u);

    ASSERT_EQ(cbegin.base(), map.cbegin().base() + 1u);
    ASSERT_EQ(cbegin++, map.begin(3u));
    ASSERT_EQ(cbegin.base(), map.cbegin().base());
    ASSERT_EQ(++cbegin, map.end(3u));
    ASSERT_NE(cbegin.base(), map.cend().base());
}

TEST(DenseHashMap, LocalIteratorConversion) {
    entt::dense_hash_map<int, int> map;
    map.emplace(3, 42);

    typename entt::dense_hash_map<int, int>::local_iterator it = map.begin(map.bucket(3));
    typename entt::dense_hash_map<int, int>::const_local_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::pair<const int, int> &>);
    static_assert(std::is_same_v<decltype(*cit), const std::pair<const int, int> &>);

    ASSERT_EQ(it->first, 3);
    ASSERT_EQ((*it).second, 42);
    ASSERT_EQ(it->first, cit->first);
    ASSERT_EQ((*it).second, (*it).second);

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseHashMap, Rehash) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<std::size_t, std::size_t, entt::identity> map;
    map[32u] = 99u;

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(12u);

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(44u);

    ASSERT_EQ(map.bucket_count(), 8u * expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 32u);
    ASSERT_EQ(map[32u], 99u);

    map.rehash(0u);

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
        map.emplace(next, next);
    }

    ASSERT_EQ(map.size(), expected_bucket_count + 1u);
    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);

    map.rehash(0u);

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));

    map.rehash(55u);

    ASSERT_EQ(map.bucket_count(), 8u * expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));

    map.rehash(2u);

    ASSERT_EQ(map.bucket_count(), 2u * expected_bucket_count);
    ASSERT_TRUE(map.contains(32u));
    ASSERT_EQ(map.bucket(32u), 0u);
    ASSERT_EQ(map[32u], 99u);

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
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

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);
    ASSERT_FALSE(map.contains(32u));

    for(std::size_t next{}; next < expected_bucket_count; ++next) {
        ASSERT_FALSE(map.contains(next));
    }

    ASSERT_EQ(map.bucket_size(0u), 0u);
    ASSERT_EQ(map.bucket_size(3u), 0u);
}

TEST(DenseHashMap, Reserve) {
    static constexpr std::size_t expected_bucket_count = 8u;
    entt::dense_hash_map<int, int> map;

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    map.reserve(0u);

    ASSERT_EQ(map.bucket_count(), expected_bucket_count);

    map.reserve(expected_bucket_count);

    ASSERT_EQ(map.bucket_count(), 2 * expected_bucket_count);
    ASSERT_EQ(map.bucket_count(), entt::next_power_of_two(std::ceil(expected_bucket_count / map.max_load_factor())));
}
