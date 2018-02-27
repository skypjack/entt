#include <gtest/gtest.h>
#include <entt/resource/cache.hpp>

struct Resource { const int value; };

struct Loader: entt::ResourceLoader<Loader, Resource> {
    std::shared_ptr<Resource> load(int value) const {
        return std::shared_ptr<Resource>(new Resource{ value });
    }
};

struct BrokenLoader: entt::ResourceLoader<BrokenLoader, Resource> {
    std::shared_ptr<Resource> load(int) const {
        return nullptr;
    }
};

TEST(ResourceCache, Functionalities) {
    entt::ResourceCache<Resource> cache;

    constexpr auto hs1 = entt::HashedString{"res1"};
    constexpr auto hs2 = entt::HashedString{"res2"};

    ASSERT_EQ(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_FALSE(cache.load<BrokenLoader>(hs1, 42));
    ASSERT_FALSE(cache.reload<BrokenLoader>(hs1, 42));

    ASSERT_EQ(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<Loader>(hs1, 42));
    ASSERT_TRUE(cache.reload<Loader>(hs1, 42));

    ASSERT_NE(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));
    ASSERT_EQ((*cache.handle(hs1)).value, 42);

    ASSERT_TRUE(cache.load<Loader>(hs2, 42));

    ASSERT_NE(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ((*cache.handle(hs1)).value, 42);
    ASSERT_EQ(cache.handle(hs2)->value, 42);

    ASSERT_NO_THROW(cache.discard(hs1));

    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ(cache.handle(hs2)->value, 42);

    ASSERT_TRUE(cache.load<Loader>(hs1, 42));
    ASSERT_NO_THROW(cache.clear());

    ASSERT_EQ(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<Loader>(hs1, 42));

    ASSERT_NE(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.handle(hs1));
    ASSERT_FALSE(cache.handle(hs2));

    ASSERT_TRUE(cache.handle(hs1));
    ASSERT_EQ(&cache.handle(hs1).get(), &static_cast<const Resource &>(cache.handle(hs1)));
    ASSERT_NO_THROW(cache.clear());

    ASSERT_EQ(cache.size(), entt::ResourceCache<Resource>::size_type{});
    ASSERT_TRUE(cache.empty());

    ASSERT_TRUE(cache.temp<Loader>(42));
    ASSERT_TRUE(cache.empty());
}
