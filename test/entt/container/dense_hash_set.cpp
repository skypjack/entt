#include <cmath>
#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/container/dense_hash_set.hpp>
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

TEST(DenseHashSet, Functionalities) {
    entt::dense_hash_set<std::size_t, entt::identity, transparent_equal_to> set;

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = set.get_allocator());

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.load_factor(), 0.f);
    ASSERT_EQ(set.max_load_factor(), .875f);

    set.max_load_factor(.9f);

    ASSERT_EQ(set.max_load_factor(), .9f);

    ASSERT_EQ(set.begin(), set.end());
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.cbegin(), set.cend());

    ASSERT_NE(set.max_bucket_count(), 0u);
    ASSERT_EQ(set.bucket_count(), 8u);
    ASSERT_EQ(set.bucket_size(3u), 0u);

    ASSERT_EQ(set.bucket(0), 0u);
    ASSERT_EQ(set.bucket(3), 3u);
    ASSERT_EQ(set.bucket(8), 0u);
    ASSERT_EQ(set.bucket(10), 2u);

    ASSERT_EQ(set.begin(1u), set.end(1u));
    ASSERT_EQ(std::as_const(set).begin(1u), std::as_const(set).end(1u));
    ASSERT_EQ(set.cbegin(1u), set.cend(1u));

    ASSERT_FALSE(set.contains(42));
    ASSERT_FALSE(set.contains(4.2));

    ASSERT_EQ(set.find(42), set.end());
    ASSERT_EQ(set.find(4.2), set.end());
    ASSERT_EQ(std::as_const(set).find(42), set.cend());
    ASSERT_EQ(std::as_const(set).find(4.2), set.cend());

    ASSERT_EQ(set.hash_function()(42), 42);
    ASSERT_TRUE(set.key_eq()(42, 42));

    set.emplace(0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);

    ASSERT_NE(set.begin(), set.end());
    ASSERT_NE(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_NE(set.cbegin(), set.cend());

    ASSERT_TRUE(set.contains(0u));
    ASSERT_EQ(set.bucket(0u), 0u);

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    ASSERT_EQ(set.begin(), set.end());
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.cbegin(), set.cend());

    ASSERT_FALSE(set.contains(0u));
}

TEST(DenseHashSet, Contructors) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<int> set;

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);

    set = entt::dense_hash_set<int>{std::allocator<int>{}};
    set = entt::dense_hash_set<int>{2u * minimum_bucket_count, std::allocator<float>{}};
    set = entt::dense_hash_set<int>{4u * minimum_bucket_count, std::hash<int>(), std::allocator<double>{}};

    set.emplace(3);

    entt::dense_hash_set<int> temp{set, set.get_allocator()};
    entt::dense_hash_set<int> other{std::move(temp), set.get_allocator()};

    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(set.bucket_count(), 4u * minimum_bucket_count);
    ASSERT_EQ(other.bucket_count(), 4u * minimum_bucket_count);
}

TEST(DenseHashSet, Copy) {
    entt::dense_hash_set<std::size_t, entt::identity> set;
    set.max_load_factor(set.max_load_factor() - .05f);
    set.emplace(3u);

    entt::dense_hash_set<std::size_t, entt::identity> other{set};

    ASSERT_TRUE(set.contains(3u));
    ASSERT_TRUE(other.contains(3u));
    ASSERT_EQ(set.max_load_factor(), other.max_load_factor());

    set.emplace(1u);
    set.emplace(11u);
    other.emplace(0u);
    other = set;

    ASSERT_TRUE(other.contains(3u));
    ASSERT_TRUE(other.contains(1u));
    ASSERT_TRUE(other.contains(11u));
    ASSERT_FALSE(other.contains(0u));

    ASSERT_EQ(other.bucket(3u), set.bucket(11u));
    ASSERT_EQ(other.bucket(3u), other.bucket(11u));
    ASSERT_EQ(*other.begin(3u), *set.begin(3u));
    ASSERT_EQ(*other.begin(3u), 11u);
    ASSERT_EQ((*++other.begin(3u)), 3u);
}

TEST(DenseHashSet, Move) {
    entt::dense_hash_set<std::size_t, entt::identity> set;
    set.max_load_factor(set.max_load_factor() - .05f);
    set.emplace(3u);

    entt::dense_hash_set<std::size_t, entt::identity> other{std::move(set)};

    ASSERT_EQ(set.size(), 0u);
    ASSERT_TRUE(other.contains(3u));
    ASSERT_EQ(set.max_load_factor(), other.max_load_factor());

    set = other;
    set.emplace(1u);
    set.emplace(11u);
    other.emplace(0u);
    other = std::move(set);

    ASSERT_EQ(set.size(), 0u);
    ASSERT_TRUE(other.contains(3u));
    ASSERT_TRUE(other.contains(1u));
    ASSERT_TRUE(other.contains(11u));
    ASSERT_FALSE(other.contains(0u));

    ASSERT_EQ(other.bucket(3u), other.bucket(11u));
    ASSERT_EQ(*other.begin(3u), 11u);
    ASSERT_EQ(*++other.begin(3u), 3u);
}

TEST(DenseHashSet, Iterator) {
    using iterator = typename entt::dense_hash_set<int>::iterator;

    static_assert(std::is_same_v<iterator::value_type, int>);
    static_assert(std::is_same_v<iterator::pointer, const int *>);
    static_assert(std::is_same_v<iterator::reference, const int &>);

    entt::dense_hash_set<int> set;
    set.emplace(3);

    iterator end{set.begin()};
    iterator begin{};
    begin = set.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, set.begin());
    ASSERT_EQ(end, set.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, set.begin());
    ASSERT_EQ(begin--, set.end());

    ASSERT_EQ(begin + 1, set.end());
    ASSERT_EQ(end - 1, set.begin());

    ASSERT_EQ(++begin, set.end());
    ASSERT_EQ(--begin, set.begin());

    ASSERT_EQ(begin += 1, set.end());
    ASSERT_EQ(begin -= 1, set.begin());

    ASSERT_EQ(begin + (end - begin), set.end());
    ASSERT_EQ(begin - (begin - end), set.end());

    ASSERT_EQ(end - (end - begin), set.begin());
    ASSERT_EQ(end + (begin - end), set.begin());

    ASSERT_EQ(begin[0u], *set.begin().operator->());
    ASSERT_EQ(begin[0u], *set.begin());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.end());

    set.emplace(42);
    begin = set.begin();

    ASSERT_EQ(begin[0u], 3);
    ASSERT_EQ(begin[1u], 42);
}

TEST(DenseHashSet, ConstIterator) {
    using iterator = typename entt::dense_hash_set<int>::const_iterator;

    static_assert(std::is_same_v<iterator::value_type, int>);
    static_assert(std::is_same_v<iterator::pointer, const int *>);
    static_assert(std::is_same_v<iterator::reference, const int &>);

    entt::dense_hash_set<int> set;
    set.emplace(3);

    iterator cend{set.cbegin()};
    iterator cbegin{};
    cbegin = set.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, set.cbegin());
    ASSERT_EQ(cend, set.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, set.cbegin());
    ASSERT_EQ(cbegin--, set.cend());

    ASSERT_EQ(cbegin + 1, set.cend());
    ASSERT_EQ(cend - 1, set.cbegin());

    ASSERT_EQ(++cbegin, set.cend());
    ASSERT_EQ(--cbegin, set.cbegin());

    ASSERT_EQ(cbegin += 1, set.cend());
    ASSERT_EQ(cbegin -= 1, set.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), set.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), set.cend());

    ASSERT_EQ(cend - (cend - cbegin), set.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), set.cbegin());

    ASSERT_EQ(cbegin[0u], *set.cbegin().operator->());
    ASSERT_EQ(cbegin[0u], *set.cbegin());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, set.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, set.cend());

    set.emplace(42);
    cbegin = set.cbegin();

    ASSERT_EQ(cbegin[0u], 3);
    ASSERT_EQ(cbegin[1u], 42);
}

TEST(DenseHashSet, IteratorConversion) {
    entt::dense_hash_set<int> set;
    set.emplace(3);

    typename entt::dense_hash_set<int, int>::iterator it = set.begin();
    typename entt::dense_hash_set<int, int>::const_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), const int &>);
    static_assert(std::is_same_v<decltype(*cit), const int &>);

    ASSERT_EQ(*it, 3);
    ASSERT_EQ(*it.operator->(), 3);
    ASSERT_EQ(it.operator->(), cit.operator->());
    ASSERT_EQ(*it, *cit);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseHashSet, Insert) {
    entt::dense_hash_set<int> set;
    typename entt::dense_hash_set<int>::iterator it;
    bool result;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.find(0), set.end());
    ASSERT_FALSE(set.contains(0));

    int value{1};
    std::tie(it, result) = set.insert(std::as_const(value));

    ASSERT_TRUE(result);
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(it, --set.end());
    ASSERT_TRUE(set.contains(1));
    ASSERT_NE(set.find(1), set.end());
    ASSERT_EQ(*it, 1);

    std::tie(it, result) = set.insert(value);

    ASSERT_FALSE(result);
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(it, --set.end());
    ASSERT_EQ(*it, 1);

    std::tie(it, result) = set.insert(3);

    ASSERT_TRUE(result);
    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(it, --set.end());
    ASSERT_TRUE(set.contains(3));
    ASSERT_NE(set.find(3), set.end());
    ASSERT_EQ(*it, 3);

    std::tie(it, result) = set.insert(3);

    ASSERT_FALSE(result);
    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(it, --set.end());
    ASSERT_EQ(*it, 3);

    int range[2u]{7, 9};
    set.insert(std::begin(range), std::end(range));

    ASSERT_EQ(set.size(), 4u);
    ASSERT_TRUE(set.contains(7));
    ASSERT_NE(set.find(9), set.end());
}

TEST(DenseHashSet, InsertRehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(set.insert(next).second);
    }

    ASSERT_EQ(set.size(), minimum_bucket_count);
    ASSERT_GT(set.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(set.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(set.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_FALSE(set.contains(minimum_bucket_count));

    ASSERT_TRUE(set.insert(minimum_bucket_count).second);

    ASSERT_EQ(set.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(set.bucket_count(), minimum_bucket_count * 2u);
    ASSERT_TRUE(set.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(set.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_TRUE(set.contains(minimum_bucket_count));

    for(std::size_t next{}; next <= minimum_bucket_count; ++next) {
        ASSERT_TRUE(set.contains(next));
        ASSERT_EQ(set.bucket(next), next);
    }
}

TEST(DenseHashSet, InsertSameBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_EQ(set.cbegin(next), set.cend(next));
    }

    ASSERT_TRUE(set.insert(1u).second);
    ASSERT_TRUE(set.insert(9u).second);

    ASSERT_EQ(set.size(), 2u);
    ASSERT_TRUE(set.contains(1u));
    ASSERT_NE(set.find(9u), set.end());
    ASSERT_EQ(set.bucket(1u), 1u);
    ASSERT_EQ(set.bucket(9u), 1u);
    ASSERT_EQ(set.bucket_size(1u), 2u);
    ASSERT_EQ(set.cbegin(6u), set.cend(6u));
}

TEST(DenseHashSet, Emplace) {
    entt::dense_hash_set<int> set;
    typename entt::dense_hash_set<int>::iterator it;
    bool result;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.find(0), set.end());
    ASSERT_FALSE(set.contains(0));

    std::tie(it, result) = set.emplace();

    ASSERT_TRUE(result);
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(it, --set.end());
    ASSERT_TRUE(set.contains(0));
    ASSERT_NE(set.find(0), set.end());
    ASSERT_EQ(*it, 0);

    std::tie(it, result) = set.emplace();

    ASSERT_FALSE(result);
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(it, --set.end());
    ASSERT_EQ(*it, 0);

    std::tie(it, result) = set.emplace(1);

    ASSERT_TRUE(result);
    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(it, --set.end());
    ASSERT_TRUE(set.contains(1));
    ASSERT_NE(set.find(1), set.end());
    ASSERT_EQ(*it, 1);

    std::tie(it, result) = set.emplace(1);

    ASSERT_FALSE(result);
    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(it, --set.end());
    ASSERT_EQ(*it, 1);
}

TEST(DenseHashSet, EmplaceRehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(set.emplace(next).second);
        ASSERT_LE(set.load_factor(), set.max_load_factor());
    }

    ASSERT_EQ(set.size(), minimum_bucket_count);
    ASSERT_GT(set.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(set.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(set.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_FALSE(set.contains(minimum_bucket_count));

    ASSERT_TRUE(set.emplace(minimum_bucket_count).second);

    ASSERT_EQ(set.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(set.bucket_count(), minimum_bucket_count * 2u);
    ASSERT_TRUE(set.contains(minimum_bucket_count / 2u));
    ASSERT_EQ(set.bucket(minimum_bucket_count / 2u), minimum_bucket_count / 2u);
    ASSERT_TRUE(set.contains(minimum_bucket_count));

    for(std::size_t next{}; next <= minimum_bucket_count; ++next) {
        ASSERT_TRUE(set.contains(next));
        ASSERT_EQ(set.bucket(next), next);
    }
}

TEST(DenseHashSet, EmplaceSameBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_EQ(set.cbegin(next), set.cend(next));
    }

    ASSERT_TRUE(set.emplace(1u).second);
    ASSERT_TRUE(set.emplace(9u).second);

    ASSERT_EQ(set.size(), 2u);
    ASSERT_TRUE(set.contains(1u));
    ASSERT_NE(set.find(9u), set.end());
    ASSERT_EQ(set.bucket(1u), 1u);
    ASSERT_EQ(set.bucket(9u), 1u);
    ASSERT_EQ(set.bucket_size(1u), 2u);
    ASSERT_EQ(set.cbegin(6u), set.cend(6u));
}

TEST(DenseHashSet, Erase) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        set.emplace(next);
    }

    ASSERT_EQ(set.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(set.size(), minimum_bucket_count + 1u);

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        ASSERT_TRUE(set.contains(next));
    }

    auto it = set.erase(++set.begin());
    it = set.erase(it, it + 1);

    ASSERT_EQ(*--set.end(), 6u);
    ASSERT_EQ(set.erase(6u), 1u);
    ASSERT_EQ(set.erase(6u), 0u);

    ASSERT_EQ(set.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(set.size(), minimum_bucket_count + 1u - 3u);

    ASSERT_EQ(it, ++set.begin());
    ASSERT_EQ(*it, 7u);
    ASSERT_EQ(*--set.end(), 5u);

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        if(next == 1u || next == 8u || next == 6u) {
            ASSERT_FALSE(set.contains(next));
            ASSERT_EQ(set.bucket_size(next), 0u);
        } else {
            ASSERT_TRUE(set.contains(next));
            ASSERT_EQ(set.bucket(next), next);
            ASSERT_EQ(set.bucket_size(next), 1u);
        }
    }

    set.erase(set.begin(), set.end());

    for(std::size_t next{}, last = minimum_bucket_count + 1u; next < last; ++next) {
        ASSERT_FALSE(set.contains(next));
    }

    ASSERT_EQ(set.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(set.size(), 0u);
}

TEST(DenseHashSet, EraseFromBucket) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);
    ASSERT_EQ(set.size(), 0u);

    for(std::size_t next{}; next < 4u; ++next) {
        ASSERT_TRUE(set.emplace(2u * minimum_bucket_count * next).second);
        ASSERT_TRUE(set.emplace(2u * minimum_bucket_count * next + 2u).second);
        ASSERT_TRUE(set.emplace(2u * minimum_bucket_count * (next + 1u) - 1u).second);
    }

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(set.size(), 12u);

    ASSERT_EQ(set.bucket_size(0u), 4u);
    ASSERT_EQ(set.bucket_size(2u), 4u);
    ASSERT_EQ(set.bucket_size(15u), 4u);

    set.erase(set.end() - 3, set.end());

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(set.size(), 9u);

    ASSERT_EQ(set.bucket_size(0u), 3u);
    ASSERT_EQ(set.bucket_size(2u), 3u);
    ASSERT_EQ(set.bucket_size(15u), 3u);

    for(std::size_t next{}; next < 3u; ++next) {
        ASSERT_TRUE(set.contains(2u * minimum_bucket_count * next));
        ASSERT_EQ(set.bucket(2u * minimum_bucket_count * next), 0u);

        ASSERT_TRUE(set.contains(2u * minimum_bucket_count * next + 2u));
        ASSERT_EQ(set.bucket(2u * minimum_bucket_count * next + 2u), 2u);

        ASSERT_TRUE(set.contains(2u * minimum_bucket_count * (next + 1u) - 1u));
        ASSERT_EQ(set.bucket(2u * minimum_bucket_count * (next + 1u) - 1u), 15u);
    }

    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * 3u));
    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * 3u + 2u));
    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * (3u + 1u) - 1u));

    set.erase(*++set.begin(0u));
    set.erase(*++set.begin(2u));
    set.erase(*++set.begin(15u));

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(set.size(), 6u);

    ASSERT_EQ(set.bucket_size(0u), 2u);
    ASSERT_EQ(set.bucket_size(2u), 2u);
    ASSERT_EQ(set.bucket_size(15u), 2u);

    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * 1u));
    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * 1u + 2u));
    ASSERT_FALSE(set.contains(2u * minimum_bucket_count * (1u + 1u) - 1u));

    while(set.begin(15) != set.end(15u)) {
        set.erase(*set.begin(15));
    }

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(set.size(), 4u);

    ASSERT_EQ(set.bucket_size(0u), 2u);
    ASSERT_EQ(set.bucket_size(2u), 2u);
    ASSERT_EQ(set.bucket_size(15u), 0u);

    ASSERT_TRUE(set.contains(0u * minimum_bucket_count));
    ASSERT_TRUE(set.contains(0u * minimum_bucket_count + 2u));
    ASSERT_TRUE(set.contains(4u * minimum_bucket_count));
    ASSERT_TRUE(set.contains(4u * minimum_bucket_count + 2u));

    set.erase(4u * minimum_bucket_count + 2u);
    set.erase(0u * minimum_bucket_count);

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_EQ(set.size(), 2u);

    ASSERT_EQ(set.bucket_size(0u), 1u);
    ASSERT_EQ(set.bucket_size(2u), 1u);
    ASSERT_EQ(set.bucket_size(15u), 0u);

    ASSERT_FALSE(set.contains(0u * minimum_bucket_count));
    ASSERT_TRUE(set.contains(0u * minimum_bucket_count + 2u));
    ASSERT_TRUE(set.contains(4u * minimum_bucket_count));
    ASSERT_FALSE(set.contains(4u * minimum_bucket_count + 2u));
}

TEST(DenseHashSet, Swap) {
    entt::dense_hash_set<int> set;
    entt::dense_hash_set<int> other;

    set.emplace(0);

    ASSERT_FALSE(set.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(set.contains(0));
    ASSERT_FALSE(other.contains(0));

    set.swap(other);

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_FALSE(set.contains(0));
    ASSERT_TRUE(other.contains(0));
}

TEST(DenseHashSet, LocalIterator) {
    using iterator = typename entt::dense_hash_set<std::size_t, entt::identity>::local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::size_t>);
    static_assert(std::is_same_v<iterator::pointer, const std::size_t *>);
    static_assert(std::is_same_v<iterator::reference, const std::size_t &>);

    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;
    set.emplace(3u);
    set.emplace(3u + minimum_bucket_count);

    iterator end{set.begin(3u)};
    iterator begin{};
    begin = set.end(3u);
    std::swap(begin, end);

    ASSERT_EQ(begin, set.begin(3u));
    ASSERT_EQ(end, set.end(3u));
    ASSERT_NE(begin, end);

    ASSERT_EQ(*begin.operator->(), 3u + minimum_bucket_count);
    ASSERT_EQ(*begin, 3u + minimum_bucket_count);

    ASSERT_EQ(begin++, set.begin(3u));
    ASSERT_EQ(++begin, set.end(3u));
}

TEST(DenseHashSet, ConstLocalIterator) {
    using iterator = typename entt::dense_hash_set<std::size_t, entt::identity>::const_local_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::size_t>);
    static_assert(std::is_same_v<iterator::pointer, const std::size_t *>);
    static_assert(std::is_same_v<iterator::reference, const std::size_t &>);

    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;
    set.emplace(3u);
    set.emplace(3u + minimum_bucket_count);

    iterator cend{set.begin(3u)};
    iterator cbegin{};
    cbegin = set.end(3u);
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, set.begin(3u));
    ASSERT_EQ(cend, set.end(3u));
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(*cbegin.operator->(), 3u + minimum_bucket_count);
    ASSERT_EQ(*cbegin, 3u + minimum_bucket_count);

    ASSERT_EQ(cbegin++, set.begin(3u));
    ASSERT_EQ(++cbegin, set.end(3u));
}

TEST(DenseHashSet, LocalIteratorConversion) {
    entt::dense_hash_set<int> set;
    set.emplace(3);

    typename entt::dense_hash_set<int>::local_iterator it = set.begin(set.bucket(3));
    typename entt::dense_hash_set<int>::const_local_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), const int &>);
    static_assert(std::is_same_v<decltype(*cit), const int &>);

    ASSERT_EQ(*it, 3);
    ASSERT_EQ(*it.operator->(), 3);
    ASSERT_EQ(it.operator->(), cit.operator->());
    ASSERT_EQ(*it, *cit);

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TEST(DenseHashSet, Rehash) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<std::size_t, entt::identity> set;
    set.emplace(32u);

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));
    ASSERT_EQ(set.bucket(32u), 0u);

    set.rehash(12u);

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));
    ASSERT_EQ(set.bucket(32u), 0u);

    set.rehash(44u);

    ASSERT_EQ(set.bucket_count(), 8u * minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));
    ASSERT_EQ(set.bucket(32u), 32u);

    set.rehash(0u);

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));
    ASSERT_EQ(set.bucket(32u), 0u);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        set.emplace(next);
    }

    ASSERT_EQ(set.size(), minimum_bucket_count + 1u);
    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);

    set.rehash(0u);

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));

    set.rehash(55u);

    ASSERT_EQ(set.bucket_count(), 8u * minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));

    set.rehash(2u);

    ASSERT_EQ(set.bucket_count(), 2u * minimum_bucket_count);
    ASSERT_TRUE(set.contains(32u));
    ASSERT_EQ(set.bucket(32u), 0u);

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_TRUE(set.contains(next));
        ASSERT_EQ(set.bucket(next), next);
    }

    ASSERT_EQ(set.bucket_size(0u), 2u);
    ASSERT_EQ(set.bucket_size(3u), 1u);

    ASSERT_EQ(*set.begin(0u), 0u);
    ASSERT_EQ(*++set.begin(0u), 32u);

    set.clear();
    set.rehash(2u);

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);
    ASSERT_FALSE(set.contains(32u));

    for(std::size_t next{}; next < minimum_bucket_count; ++next) {
        ASSERT_FALSE(set.contains(next));
    }

    ASSERT_EQ(set.bucket_size(0u), 0u);
    ASSERT_EQ(set.bucket_size(3u), 0u);
}

TEST(DenseHashSet, Reserve) {
    static constexpr std::size_t minimum_bucket_count = 8u;
    entt::dense_hash_set<int> set;

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);

    set.reserve(0u);

    ASSERT_EQ(set.bucket_count(), minimum_bucket_count);

    set.reserve(minimum_bucket_count);

    ASSERT_EQ(set.bucket_count(), 2 * minimum_bucket_count);
    ASSERT_EQ(set.bucket_count(), entt::next_power_of_two(std::ceil(minimum_bucket_count / set.max_load_factor())));
}
