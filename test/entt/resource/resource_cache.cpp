#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>
#include <entt/resource/loader.hpp>
#include "../common/throwing_allocator.hpp"

struct broken_tag {};
struct with_callback {};

template<typename Type>
struct loader {
    using result_type = std::shared_ptr<Type>;

    template<typename... Args>
    result_type operator()(Args &&...args) const {
        return std::make_shared<Type>(std::forward<Args>(args)...);
    }

    template<typename Func>
    result_type operator()(with_callback, Func &&func) const {
        return std::forward<Func>(func)();
    }

    template<typename... Args>
    result_type operator()(broken_tag, Args &&...) const {
        return {};
    }
};

TEST(ResourceCache, Functionalities) {
    using namespace entt::literals;

    entt::resource_cache<int> cache;

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = cache.get_allocator());

    ASSERT_TRUE(cache.empty());
    ASSERT_EQ(cache.size(), 0u);

    ASSERT_EQ(cache.begin(), cache.end());
    ASSERT_EQ(std::as_const(cache).begin(), std::as_const(cache).end());
    ASSERT_EQ(cache.cbegin(), cache.cend());

    ASSERT_FALSE(cache.contains("resource"_hs));

    cache.load("resource"_hs, 42);

    ASSERT_FALSE(cache.empty());
    ASSERT_EQ(cache.size(), 1u);

    ASSERT_NE(cache.begin(), cache.end());
    ASSERT_NE(std::as_const(cache).begin(), std::as_const(cache).end());
    ASSERT_NE(cache.cbegin(), cache.cend());

    ASSERT_TRUE(cache.contains("resource"_hs));

    cache.clear();

    ASSERT_TRUE(cache.empty());
    ASSERT_EQ(cache.size(), 0u);

    ASSERT_EQ(cache.begin(), cache.end());
    ASSERT_EQ(std::as_const(cache).begin(), std::as_const(cache).end());
    ASSERT_EQ(cache.cbegin(), cache.cend());

    ASSERT_FALSE(cache.contains("resource"_hs));
}

TEST(ResourceCache, Constructors) {
    using namespace entt::literals;

    entt::resource_cache<int> cache;

    cache = entt::resource_cache<int>{std::allocator<int>{}};
    cache = entt::resource_cache<int>{entt::resource_loader<int>{}, std::allocator<float>{}};

    cache.load("resource"_hs, 42u);

    entt::resource_cache<int> temp{cache, cache.get_allocator()};
    entt::resource_cache<int> other{std::move(temp), cache.get_allocator()};

    ASSERT_EQ(cache.size(), 1u);
    ASSERT_EQ(other.size(), 1u);
}

TEST(ResourceCache, Copy) {
    using namespace entt::literals;

    entt::resource_cache<std::size_t> cache;
    cache.load("resource"_hs, 42u);

    entt::resource_cache<std::size_t> other{cache};

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_TRUE(other.contains("resource"_hs));

    cache.load("foo"_hs, 99u);
    cache.load("bar"_hs, 77u);
    other.load("quux"_hs, 0u);
    other = cache;

    ASSERT_TRUE(other.contains("resource"_hs));
    ASSERT_TRUE(other.contains("foo"_hs));
    ASSERT_TRUE(other.contains("bar"_hs));
    ASSERT_FALSE(other.contains("quux"_hs));

    ASSERT_EQ(other["resource"_hs], 42u);
    ASSERT_EQ(other["foo"_hs], 99u);
    ASSERT_EQ(other["bar"_hs], 77u);
}

TEST(ResourceCache, Move) {
    using namespace entt::literals;

    entt::resource_cache<std::size_t> cache;
    cache.load("resource"_hs, 42u);

    entt::resource_cache<std::size_t> other{std::move(cache)};

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(other.contains("resource"_hs));

    cache = other;
    cache.load("foo"_hs, 99u);
    cache.load("bar"_hs, 77u);
    other.load("quux"_hs, 0u);
    other = std::move(cache);

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(other.contains("resource"_hs));
    ASSERT_TRUE(other.contains("foo"_hs));
    ASSERT_TRUE(other.contains("bar"_hs));
    ASSERT_FALSE(other.contains("quux"_hs));

    ASSERT_EQ(other["resource"_hs], 42u);
    ASSERT_EQ(other["foo"_hs], 99u);
    ASSERT_EQ(other["bar"_hs], 77u);
}

TEST(ResourceCache, Iterator) {
    using namespace entt::literals;

    using iterator = typename entt::resource_cache<int>::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<entt::id_type, entt::resource<int>>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<entt::id_type, entt::resource<int>>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<entt::id_type, entt::resource<int>>>);

    entt::resource_cache<int> cache;
    cache.load("resource"_hs, 42);

    iterator end{cache.begin()};
    iterator begin{};
    begin = cache.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, cache.begin());
    ASSERT_EQ(end, cache.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, cache.begin());
    ASSERT_EQ(begin--, cache.end());

    ASSERT_EQ(begin + 1, cache.end());
    ASSERT_EQ(end - 1, cache.begin());

    ASSERT_EQ(++begin, cache.end());
    ASSERT_EQ(--begin, cache.begin());

    ASSERT_EQ(begin += 1, cache.end());
    ASSERT_EQ(begin -= 1, cache.begin());

    ASSERT_EQ(begin + (end - begin), cache.end());
    ASSERT_EQ(begin - (begin - end), cache.end());

    ASSERT_EQ(end - (end - begin), cache.begin());
    ASSERT_EQ(end + (begin - end), cache.begin());

    ASSERT_EQ(begin[0u].first, cache.begin()->first);
    ASSERT_EQ(begin[0u].second, (*cache.begin()).second);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, cache.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, cache.end());

    cache.load("other"_hs, 3);
    begin = cache.begin();

    ASSERT_EQ(begin[0u].first, "resource"_hs);
    ASSERT_EQ(begin[1u].second, 3);
}

TEST(ResourceCache, ConstIterator) {
    using namespace entt::literals;

    using iterator = typename entt::resource_cache<int>::const_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<entt::id_type, entt::resource<const int>>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<entt::id_type, entt::resource<const int>>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<entt::id_type, entt::resource<const int>>>);

    entt::resource_cache<int> cache;
    cache.load("resource"_hs, 42);

    iterator cend{cache.cbegin()};
    iterator cbegin{};
    cbegin = cache.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, cache.cbegin());
    ASSERT_EQ(cend, cache.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, cache.cbegin());
    ASSERT_EQ(cbegin--, cache.cend());

    ASSERT_EQ(cbegin + 1, cache.cend());
    ASSERT_EQ(cend - 1, cache.cbegin());

    ASSERT_EQ(++cbegin, cache.cend());
    ASSERT_EQ(--cbegin, cache.cbegin());

    ASSERT_EQ(cbegin += 1, cache.cend());
    ASSERT_EQ(cbegin -= 1, cache.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), cache.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), cache.cend());

    ASSERT_EQ(cend - (cend - cbegin), cache.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), cache.cbegin());

    ASSERT_EQ(cbegin[0u].first, cache.cbegin()->first);
    ASSERT_EQ(cbegin[0u].second, (*cache.cbegin()).second);

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, cache.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, cache.cend());

    cache.load("other"_hs, 3);
    cbegin = cache.cbegin();

    ASSERT_EQ(cbegin[0u].first, "resource"_hs);
    ASSERT_EQ(cbegin[1u].second, 3);
}

TEST(ResourceCache, IteratorConversion) {
    using namespace entt::literals;

    entt::resource_cache<int> cache;
    cache.load("resource"_hs, 42);

    typename entt::resource_cache<int>::iterator it = cache.begin();
    typename entt::resource_cache<int>::const_iterator cit = it;

    static_assert(std::is_same_v<decltype(*it), std::pair<entt::id_type, entt::resource<int>>>);
    static_assert(std::is_same_v<decltype(*cit), std::pair<entt::id_type, entt::resource<const int>>>);

    ASSERT_EQ(it->first, "resource"_hs);
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

TEST(ResourceCache, Load) {
    using namespace entt::literals;

    entt::resource_cache<int> cache;
    typename entt::resource_cache<int>::iterator it;
    bool result;

    ASSERT_TRUE(cache.empty());
    ASSERT_EQ(cache.size(), 0u);
    ASSERT_EQ(cache["resource"_hs], entt::resource<int>{});
    ASSERT_EQ(std::as_const(cache)["resource"_hs], entt::resource<const int>{});
    ASSERT_FALSE(cache.contains("resource"_hs));

    std::tie(it, result) = cache.load("resource"_hs, 1);

    ASSERT_TRUE(result);
    ASSERT_EQ(cache.size(), 1u);
    ASSERT_EQ(it, --cache.end());
    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_NE(cache["resource"_hs], entt::resource<int>{});
    ASSERT_NE(std::as_const(cache)["resource"_hs], entt::resource<const int>{});
    ASSERT_EQ(it->first, "resource"_hs);
    ASSERT_EQ(it->second, 1);

    std::tie(it, result) = cache.load("resource"_hs, 2);

    ASSERT_FALSE(result);
    ASSERT_EQ(cache.size(), 1u);
    ASSERT_EQ(it, --cache.end());
    ASSERT_EQ(it->second, 1);

    std::tie(it, result) = cache.force_load("resource"_hs, 3);

    ASSERT_TRUE(result);
    ASSERT_EQ(cache.size(), 1u);
    ASSERT_EQ(it, --cache.end());
    ASSERT_EQ(it->second, 3);
}

TEST(ResourceCache, Erase) {
    static constexpr std::size_t resource_count = 8u;
    entt::resource_cache<std::size_t> cache;

    for(std::size_t next{}, last = resource_count + 1u; next < last; ++next) {
        cache.load(next, next);
    }

    ASSERT_EQ(cache.size(), resource_count + 1u);

    for(std::size_t next{}, last = resource_count + 1u; next < last; ++next) {
        ASSERT_TRUE(cache.contains(next));
    }

    auto it = cache.erase(++cache.begin());
    it = cache.erase(it, it + 1);

    ASSERT_EQ((--cache.end())->first, 6u);
    ASSERT_EQ(cache.erase(6u), 1u);
    ASSERT_EQ(cache.erase(6u), 0u);

    ASSERT_EQ(cache.size(), resource_count + 1u - 3u);

    ASSERT_EQ(it, ++cache.begin());
    ASSERT_EQ(it->first, 7u);
    ASSERT_EQ((--cache.end())->first, 5u);

    for(std::size_t next{}, last = resource_count + 1u; next < last; ++next) {
        if(next == 1u || next == 8u || next == 6u) {
            ASSERT_FALSE(cache.contains(next));
        } else {
            ASSERT_TRUE(cache.contains(next));
        }
    }

    cache.erase(cache.begin(), cache.end());

    for(std::size_t next{}, last = resource_count + 1u; next < last; ++next) {
        ASSERT_FALSE(cache.contains(next));
    }

    ASSERT_EQ(cache.size(), 0u);
}

TEST(ResourceCache, Indexing) {
    using namespace entt::literals;

    entt::resource_cache<int> cache;

    ASSERT_FALSE(cache.contains("resource"_hs));
    ASSERT_FALSE(cache["resource"_hs]);
    ASSERT_FALSE(std::as_const(cache)["resource"_hs]);

    cache.load("resource"_hs, 99);

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_EQ(cache[std::move("resource"_hs)], 99);
    ASSERT_EQ(std::as_const(cache)["resource"_hs], 99);
}

TEST(ResourceCache, LoaderDispatching) {
    using namespace entt::literals;

    entt::resource_cache<int, loader<int>> cache;
    cache.force_load("resource"_hs, 99);

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_EQ(cache["resource"_hs], 99);

    cache.force_load("resource"_hs, with_callback{}, []() { return std::make_shared<int>(42); });

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_EQ(cache["resource"_hs], 42);
}

TEST(ResourceCache, BrokenLoader) {
    using namespace entt::literals;

    entt::resource_cache<int, loader<int>> cache;
    cache.load("resource"_hs, broken_tag{});

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_FALSE(cache["resource"_hs]);

    cache.force_load("resource"_hs, 42);

    ASSERT_TRUE(cache.contains("resource"_hs));
    ASSERT_TRUE(cache["resource"_hs]);
}

TEST(ResourceCache, ThrowingAllocator) {
    using namespace entt::literals;

    using allocator = test::throwing_allocator<std::size_t>;
    using packed_allocator = test::throwing_allocator<entt::internal::dense_map_node<entt::id_type, std::shared_ptr<std::size_t>>>;
    using packed_exception = typename packed_allocator::exception_type;

    entt::resource_cache<std::size_t, entt::resource_loader<std::size_t>, allocator> cache{};

    packed_allocator::trigger_on_allocate = true;

    ASSERT_THROW(cache.load("resource"_hs), packed_exception);
    ASSERT_FALSE(cache.contains("resource"_hs));

    packed_allocator::trigger_on_allocate = true;

    ASSERT_THROW(cache.force_load("resource"_hs), packed_exception);
    ASSERT_FALSE(cache.contains("resource"_hs));
}
